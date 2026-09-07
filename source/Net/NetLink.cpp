#include "NetLink.hpp"

#include <dswifi9.h>
#include <dswifi_dlplay.h>

#include <cstdio>
#include <cstring>

// All dswifi access is confined to this file. Received frames are copied raw in
// the IRQ-context packet handlers; yas (de)serialization happens on the main
// thread in the Poll/Send helpers (guarded by REG_IME for the shared buffers).

namespace
{
    constexpr int kMsgCap = 512; // max framed payload (tag + yas blob)
    constexpr int kIntentCap = 64;

    // Consecutive bad frames before a link is declared lost. Half a second at
    // 60 fps: long enough to ride out a blip, short enough that a console that
    // really left is noticed promptly.
    constexpr int kLinkLossFrames = 30;
    int g_hostLostFrames = 0;
    int g_clientLostFrames = 0;

    // Beacon name the host advertises, and what a direct connect asks for.
    constexpr const char *kHostSsid = "SKYJO";

    bool g_wifiInited = false;
    bool g_isHost = false;
    bool g_isClient = false;
    bool g_isDlPlay = false;

    // Scratch used only inside the IRQ packet handlers (handlers never re-enter).
    uint8_t g_rxScratch[kMsgCap];

    // Host: most recent intent per client seat (AID 1..kMaxClients).
    volatile bool g_intentDirty[NetLink::kMaxClients + 1] = {};
    volatile int  g_intentLen[NetLink::kMaxClients + 1] = {};
    uint8_t       g_intentBuf[NetLink::kMaxClients + 1][kIntentCap];

    // Host: most recent lobby "hello" (console name) per client seat.
    volatile bool g_helloDirty[NetLink::kMaxClients + 1] = {};
    volatile int  g_helloLen[NetLink::kMaxClients + 1] = {};
    uint8_t       g_helloBuf[NetLink::kMaxClients + 1][kMsgCap];

    // Client: separate slots for start vs snapshot so one never clobbers the other.
    volatile bool g_startDirty = false;
    volatile int  g_startLen = 0;
    uint8_t       g_startBuf[kMsgCap];

    volatile bool g_snapDirty = false;
    volatile int  g_snapLen = 0;
    uint8_t       g_snapBuf[kMsgCap];

    // Disconnect tracking.
    uint16_t g_lockedMask = 0;
    bool g_hostLost = false;
    bool g_clientLost = false;
    bool g_everAssociated = false;

    // Client: the host said it is leaving. A single bool rather than a buffer,
    // so the packet handler can set it without any of the copy-under-IRQ care
    // the other messages need.
    volatile bool g_byeSeen = false;

    // Frames a bounded wait on the ARM7 will sit through before giving up. The
    // transitions being waited for take a handful of frames; a second and a half
    // means a wedged ARM7 costs a visible pause rather than a hung console.
    constexpr int kModeWaitFrames = 90;

    // Frames the host repeats its goodbye for. Each one carries a drive cycle,
    // so this is how many chances a client gets to hear it.
    //
    // It is also how long the caller is blocked, and the caller is not pumping
    // the music stream while it waits. Eight frames is 133 ms against the 372 ms
    // the audio ring holds (16 KB at 11025 Hz stereo 16-bit), so the buffer
    // drains but does not run dry. Raising this materially means pumping the
    // stream from the loop below.
    constexpr int kByeFrames = 8;

    // ---- Download Play room announcement ----

    // The 32 bytes handed to the program a guest is sent. It lives here rather
    // than inside DlPlayStart() because the library keeps the pointer: it is
    // read again every time the boot information goes out.
    SkyjoDlPlayParam g_dlParam = {};

    // The description announced in beacon frames, in the UTF-16LE the record
    // wants, and how much of it is used.
    u16 g_dlDesc[DSWIFI_DLPLAY_DESC_LEN] = {};
    u8  g_dlDescLen = 0;

    // What was last put on the air. Anything below zero means "nothing yet", so
    // the first announcement of a session always goes out.
    int g_dlLastGuests = -1;
    int g_dlLastSeats = -1;
    int g_dlLastPhase = -1;

    // Renders the room into g_dlDesc. Plain ASCII widened by hand: the record
    // stores UTF-16LE, and every character the game puts here is in the part of
    // it that widening gets right.
    void BuildRoomDescription(int guests, int maxSeats, DlPlayRoomPhase phase)
    {
        char text[DSWIFI_DLPLAY_DESC_LEN + 1];

        switch (phase)
        {
            case DlPlayRoomPhase::Closing:
                snprintf(text, sizeof(text), "Room closed - %d joined, starting",
                         guests + 1);
                break;
            case DlPlayRoomPhase::Booting:
                snprintf(text, sizeof(text), "Starting - room closed");
                break;
            case DlPlayRoomPhase::Open:
            default:
                snprintf(text, sizeof(text), "Waiting for players - %d/%d",
                         guests + 1, maxSeats);
                break;
        }

        std::size_t len = strlen(text);
        if (len > DSWIFI_DLPLAY_DESC_LEN) len = DSWIFI_DLPLAY_DESC_LEN;

        for (std::size_t i = 0; i < DSWIFI_DLPLAY_DESC_LEN; ++i)
            g_dlDesc[i] = (i < len) ? static_cast<u16>(text[i]) : 0;

        g_dlDescLen = static_cast<u8>(len);
    }

    // Wait until the ARM7 has really finished the mode change that was asked
    // for, or until the patience above runs out. Bounded rather than a plain
    // while loop, so an ARM7 that never arrives costs a visible pause instead of
    // a console that has to be switched off.
    //
    // Note what this can and cannot see. Wifi_LibraryModeReady() only says
    // anything while the requested library mode differs from the current one,
    // because the ARM7 copies one into the other exclusively when it is idle --
    // so it is a real wait for a host being asked to become a client, and an
    // instant yes for a client that stays one. That asymmetry is fine: leaving
    // access point mode is the transition with state latched on entry
    // (curMaxClients, curCmdDataSize) that the next session would otherwise
    // inherit, and leaving an association is not.
    //
    // Wifi_AssocStatus() deliberately has no part in this. In its searching
    // state it does more than report: it looks for the access point it was last
    // asked for and, if it is still on the air, sets reqMode back to CONNECTED
    // -- so polling it here would have re-armed the very connection Shutdown()
    // is trying to drop.
    void WaitForLibraryMode()
    {
        for (int i = 0; i < kModeWaitFrames && !Wifi_LibraryModeReady(); ++i)
            swiWaitForVBlank();
    }

    // ---- IRQ-context packet handlers ----

    void FromClientHandler(Wifi_MPPacketType type, int aid, int base, int len)
    {
        if (type != WIFI_MPTYPE_DATA) return;
        if (len < 2 || len > kMsgCap) return;
        if (aid < 1 || aid > NetLink::kMaxClients) return;

        Wifi_RxRawReadPacket(base, len, g_rxScratch);
        switch (static_cast<NetMsgType>(g_rxScratch[0]))
        {
            case NetMsgType::Intent:
                if (len > kIntentCap) return;
                memcpy(const_cast<uint8_t*>(g_intentBuf[aid]), g_rxScratch, len);
                g_intentLen[aid] = len;
                g_intentDirty[aid] = true;
                break;
            case NetMsgType::Hello:
                memcpy(const_cast<uint8_t*>(g_helloBuf[aid]), g_rxScratch, len);
                g_helloLen[aid] = len;
                g_helloDirty[aid] = true;
                break;
            default:
                break;
        }
    }

    void FromHostHandler(Wifi_MPPacketType type, int base, int len)
    {
        if (type != WIFI_MPTYPE_DATA) return;
        if (len < 2 || len > kMsgCap) return;

        Wifi_RxRawReadPacket(base, len, g_rxScratch);
        switch (static_cast<NetMsgType>(g_rxScratch[0]))
        {
            case NetMsgType::Start:
                memcpy(g_startBuf, g_rxScratch, len);
                g_startLen = len;
                g_startDirty = true;
                break;
            case NetMsgType::Snapshot:
                memcpy(g_snapBuf, g_rxScratch, len);
                g_snapLen = len;
                g_snapDirty = true;
                break;
            case NetMsgType::Bye:
                // The reason is carried for the sake of anyone who wants it
                // later; all a client has to know here is that the host is
                // going, and a latched flag says that without a buffer.
                g_byeSeen = true;
                break;
            default:
                break;
        }
    }

    // Frame a yas blob as [tag][blob], padded to an even length (dswifi reads
    // frames in 16-bit words; yas ignores any trailing pad byte). Returns the
    // framed length, or -1 if it doesn't fit.
    int FrameMessage(uint8_t* out, NetMsgType tag, const yas::shared_buffer& sb)
    {
        int len = 1 + static_cast<int>(sb.size);
        if (len + 1 > kMsgCap) return -1;
        out[0] = static_cast<uint8_t>(tag);
        memcpy(out + 1, sb.data.get(), sb.size);
        if (len & 1) out[len++] = 0;
        return len;
    }

    // Copy one dirty receive buffer out under IRQ lock, then deserialize.
    template <class T>
    bool PollInto(volatile bool& dirty, const uint8_t* src, volatile int& srcLen, T& out)
    {
        if (!dirty) return false;
        uint8_t tmp[kMsgCap];
        int len;
        uint32_t ime = REG_IME;
        REG_IME = 0;
        len = srcLen;
        if (len > kMsgCap) len = kMsgCap;
        memcpy(tmp, src, len);
        dirty = false;
        REG_IME = ime;

        if (len < 2) return false;
        yas::load<kYasNetFlag>(
            yas::intrusive_buffer(reinterpret_cast<const char*>(tmp + 1), len - 1), out);
        return true;
    }
}

namespace NetLink
{
    bool InitWifi()
    {
        if (g_wifiInited) return true;
        if (!Wifi_InitDefault(INIT_ONLY | WIFI_LOCAL_ONLY)) return false;
        g_wifiInited = true;
        return true;
    }

    int InitFailStage()
    {
        return Wifi_GetInitFailStage();
    }

    void Shutdown()
    {
        DlPlayStop();

        if (g_wifiInited)
        {
            // Stop the handlers before the mode goes, so a frame still in flight
            // cannot raise a dirty flag after the loop below has cleared it.
            Wifi_MultiplayerFromClientSetPacketHandler(nullptr);
            Wifi_MultiplayerFromHostSetPacketHandler(nullptr);

            // Take the association state machine out of its searching state.
            // Left there it would keep trying to reach the host we are walking
            // away from, and any later Wifi_AssocStatus() call would help it
            // along rather than just report (see WaitForLibraryMode above).
            Wifi_DisconnectAP();

            // Ask for a library mode, not just an idle mode.
            //
            // Wifi_IdleMode() on its own only moves reqMode; reqLibraryMode is
            // left wherever the session put it. The ARM7 copies reqLibraryMode
            // into curLibraryMode exclusively while it is idle, so leaving the
            // two equal makes every later Wifi_LibraryModeReady() answer true on
            // the first ask -- and the next StartHost() then ran Wifi_SetChannel,
            // Wifi_MultiplayerHostName and Wifi_BeaconStart while the ARM7 was
            // still tearing the previous access point down. Wifi_BeaconStart
            // reads curMaxClients and curCmdDataSize, which are only written on
            // *entering* access point mode, so the second beacon of a session
            // advertised the first one's frame sizes and no console could join
            // it. Restarting the game was the only way out of that, which is
            // exactly the thing this is here to stop.
            //
            // Requesting client mode is what makes the wait mean something:
            // coming from a host, the two modes now disagree and the ARM7 can
            // only reconcile them by finishing the teardown first.
            Wifi_MultiplayerClientMode(8);
            Wifi_IdleMode();
            WaitForLibraryMode();
        }

        g_isHost = false;
        g_isClient = false;

        // Everything the session accumulated goes with it.
        //
        // This used to be left to whichever of StartHost() / StartClientScan() /
        // ClientConnectDirect() came next, and worked only because each of them
        // remembered to do it. That is the wrong way round: there are a dozen
        // places a party can end -- the pause menu, the end-game menu, a lost
        // client, a lost host, Back out of three different screens -- against
        // three that begin one. Clearing it at the single point they all pass
        // through means a latched "the link is gone" or a half-read frame from
        // the console that just left cannot still be sitting here when the next
        // party opens.
        g_hostLost = false;
        g_hostLostFrames = 0;
        g_lockedMask = 0;

        g_clientLost = false;
        g_clientLostFrames = 0;
        g_everAssociated = false;
        g_byeSeen = false;

        g_startDirty = false;
        g_snapDirty = false;
        for (int aid = 0; aid <= NetLink::kMaxClients; ++aid)
        {
            g_intentDirty[aid] = false;
            g_helloDirty[aid] = false;
        }
    }

    void PowerDown()
    {
        if (!g_wifiInited) return;

        Shutdown();

        Wifi_DisableWifi();

        // Retrying is the wait. Wifi_Deinit() refuses until the ARM7 has really
        // reached its disabled mode, and there is no other way from here to ask
        // whether it has -- so calling it once a frame both tests the condition
        // and does the work the moment it holds.
        for (int i = 0; i < kModeWaitFrames; ++i)
        {
            if (Wifi_Deinit())
                break;
            swiWaitForVBlank();
        }

        // Even if the ARM7 never got there, nothing may touch the library again
        // without going back through Wifi_InitDefault(): Wifi_Deinit() frees the
        // structure both processors share, and a caller that assumed otherwise
        // would be reading freed memory.
        g_wifiInited = false;
    }

    // ----------------------------------------------------------------- Host ---

    bool StartHost()
    {
        if (!InitWifi()) return false;

        // Idempotent, so the beacon can be brought up the moment it is useful
        // and the lobby screen's own call becomes a no-op. Restarting it there
        // would tear down a beacon that clients may already be associating
        // with, for no reason.
        //
        // The state a *lobby* owns is still reset, though: leaving it stale
        // would have HostLostClient() comparing against the previous lobby's
        // locked mask and declaring a client lost before anyone had joined.
        if (g_isHost)
        {
            Wifi_MultiplayerAllowNewClients(true);
            g_lockedMask = 0;
            g_hostLost = false;
            g_hostLostFrames = 0;
            return true;
        }

        // CMD/REPLY frames are unused (we drive everything over DATA frames), so
        // the host/client packet sizes are nominal.
        Wifi_MultiplayerHostMode(kMaxClients, 8, 8);
        Wifi_MultiplayerFromClientSetPacketHandler(FromClientHandler);

        WaitForLibraryMode();

        Wifi_SetChannel(kHostChannel);

        Wifi_MultiplayerHostName(kHostSsid, strlen(kHostSsid));
        Wifi_MultiplayerAllowNewClients(true);
        Wifi_BeaconStart(kHostSsid, kSkyjoGameId);

        for (int i = 0; i <= kMaxClients; ++i) { g_intentDirty[i] = false; g_helloDirty[i] = false; }
        g_byeSeen = false;
        g_isHost = true;
        g_isClient = false;
        g_hostLost = false;
        g_hostLostFrames = 0;
        g_lockedMask = 0;
        return true;
    }

    int HostNumClients()
    {
        return g_isHost ? Wifi_MultiplayerGetNumClients() : 0;
    }

    uint16_t HostClientMask()
    {
        return g_isHost ? Wifi_MultiplayerGetClientMask() : 0;
    }

    void LockLobby()
    {
        Wifi_MultiplayerAllowNewClients(false);
        g_lockedMask = Wifi_MultiplayerGetClientMask();
    }

    void HostDriveCycle()
    {
        if (!g_isHost) return;
        // The payload is irrelevant (clients ignore CMD frames); sending it is
        // what drives the hardware MP transfer so queued DATA frames flush.
        static uint16_t beat = 0;
        ++beat;
        Wifi_MultiplayerHostCmdTxFrame(&beat, sizeof(beat));
    }

    void HostSendStart(int seat, const GameNetStart& start)
    {
        if (!g_isHost) return;
        yas::shared_buffer sb = yas::save<kYasNetFlag>(start);
        uint8_t frame[kMsgCap];
        int len = FrameMessage(frame, NetMsgType::Start, sb);
        if (len > 0) Wifi_MultiplayerHostToClientDataTxFrame(seat, frame, len);
    }

    void HostBroadcastSnapshot(const GameNetSnapshot& snap)
    {
        if (!g_isHost) return;
        yas::shared_buffer sb = yas::save<kYasNetFlag>(snap);
        uint8_t frame[kMsgCap];
        int len = FrameMessage(frame, NetMsgType::Snapshot, sb);
        if (len <= 0) return;

        uint16_t mask = Wifi_MultiplayerGetClientMask();
        for (int aid = 1; aid <= kMaxClients; ++aid)
            if (mask & BIT(aid))
                Wifi_MultiplayerHostToClientDataTxFrame(aid, frame, len);
    }

    bool HostPollIntent(int seat, GameNetIntent& out)
    {
        if (!g_isHost || seat < 1 || seat > kMaxClients) return false;
        return PollInto(g_intentDirty[seat], const_cast<const uint8_t*>(g_intentBuf[seat]),
                        g_intentLen[seat], out);
    }

    bool HostPollHello(int seat, GameNetHello& out)
    {
        if (!g_isHost || seat < 1 || seat > kMaxClients) return false;
        return PollInto(g_helloDirty[seat], const_cast<const uint8_t*>(g_helloBuf[seat]),
                        g_helloLen[seat], out);
    }

    bool HostLostClient()
    {
        if (!g_isHost || g_lockedMask == 0) return g_hostLost;

        uint16_t m = Wifi_MultiplayerGetClientMask();
        if ((g_lockedMask & m) != g_lockedMask)
        {
            // Only give up after the mask has stayed short for several frames
            // in a row. Latching on one sample meant a momentary blip ended the
            // game for everybody, which is indistinguishable from somebody
            // quitting; a console that has really gone stays gone.
            if (++g_hostLostFrames >= kLinkLossFrames) g_hostLost = true;
        }
        else
        {
            g_hostLostFrames = 0;
        }
        return g_hostLost;
    }

    void HostAnnounceBye(NetByeReason reason)
    {
        if (!g_isHost) return;

        GameNetBye bye;
        bye.reason = static_cast<uint8_t>(reason);

        yas::shared_buffer sb = yas::save<kYasNetFlag>(bye);
        uint8_t frame[kMsgCap];
        int len = FrameMessage(frame, NetMsgType::Bye, sb);
        if (len <= 0) return;

        // Repeated across several drive cycles on purpose. A queued host->client
        // DATA frame is only put on the air when a CMD frame goes out, so one
        // send with no cycle behind it reaches nobody -- and a client that
        // happened to miss the one cycle it did get would be back to waiting for
        // a timeout, which is the thing this message exists to avoid.
        for (int i = 0; i < kByeFrames; ++i)
        {
            uint16_t mask = Wifi_MultiplayerGetClientMask();
            for (int aid = 1; aid <= kMaxClients; ++aid)
            {
                if (mask & BIT(aid))
                    Wifi_MultiplayerHostToClientDataTxFrame(aid, frame, len);
            }

            HostDriveCycle();
            swiWaitForVBlank();
        }
    }

    // --------------------------------------------------------------- Client ---

    bool StartClientScan()
    {
        if (!InitWifi()) return false;

        Wifi_MultiplayerClientMode(8);
        Wifi_MultiplayerFromHostSetPacketHandler(FromHostHandler);

        WaitForLibraryMode();

        Wifi_ScanMode();

        g_startDirty = false;
        g_snapDirty = false;
        g_byeSeen = false;
        g_isClient = true;
        g_isHost = false;
        g_clientLost = false;
        g_clientLostFrames = 0;
        g_everAssociated = false;
        return true;
    }

    bool ClientReconnectDirect(const uint8_t bssid[6], int channel)
    {
        if (!g_isClient) return false;

        // A goodbye from the host we were talking to says nothing about the one
        // we are about to look for, and leaving it latched would have the retry
        // give up the instant it succeeded.
        g_byeSeen = false;

        Wifi_AccessPoint ap = {};
        memcpy(ap.bssid, bssid, sizeof(ap.bssid));
        ap.channel = static_cast<u8>(channel);
        strncpy(ap.ssid, kHostSsid, sizeof(ap.ssid) - 1);
        ap.ssid_len = static_cast<u8>(strlen(kHostSsid));

        // The association state is what is being retried, so it is the only
        // thing reset. The AP list the ARM7 has been collecting stays: throwing
        // it away is what made the previous attempt start from nothing.
        g_clientLost = false;
        g_clientLostFrames = 0;

        Wifi_ConnectOpenAP(&ap);
        return true;
    }

    int ClientNumAP()
    {
        return g_isClient ? Wifi_GetNumAP() : 0;
    }

    bool ClientGetAPInfo(int idx, char* nameOut, int nameCap,
                         int* playersCur, int* playersMax)
    {
        if (!g_isClient || idx < 0 || idx >= Wifi_GetNumAP()) return false;

        Wifi_AccessPoint ap;
        Wifi_GetAPData(idx, &ap);

        // Only list Skyjo hosts that are still accepting players.
        if (ap.nintendo.game_id != kSkyjoGameId) return false;
        if (!ap.nintendo.allows_connections) return false;

        if (nameOut && nameCap > 0)
        {
            int n = nameCap - 1;
            strncpy(nameOut, ap.ssid, n);
            nameOut[n] = '\0';
        }
        if (playersCur) *playersCur = ap.nintendo.players_current;
        if (playersMax) *playersMax = ap.nintendo.players_max;
        return true;
    }

    bool ClientConnectDirect(const uint8_t bssid[6], int channel)
    {
        if (!InitWifi()) return false;

        // Note what this does and does not save. It does NOT skip the channel
        // sweep: Wifi_ConnectOpenAP() stores the AP, calls Wifi_ScanMode()
        // itself and then waits in ASSOCSTATUS_SEARCHING until the beacon of
        // that BSSID turns up in the ARM7's scan list (access_point.c:230,
        // :299). There is no way to associate with a console that has not been
        // heard from, and callers must be patient enough for the sweep.
        //
        // What it does buy is the *right* console with no application-level
        // scan loop of our own: the loader left us the host's MAC, so
        // Wifi_FindMatchingAP() matches on that instead of us picking the first
        // beacon advertising the Skyjo game id, and none of the guests can end
        // up at a different Skyjo host that happens to be in range.
        Wifi_MultiplayerClientMode(8);
        Wifi_MultiplayerFromHostSetPacketHandler(FromHostHandler);

        WaitForLibraryMode();

        // Built by hand rather than copied out of a scan result: BSSID and
        // channel are what the association actually needs, and the SSID is
        // filled in to match what StartHost() beacons.
        Wifi_AccessPoint ap = {};
        memcpy(ap.bssid, bssid, sizeof(ap.bssid));
        ap.channel = static_cast<u8>(channel);
        strncpy(ap.ssid, kHostSsid, sizeof(ap.ssid) - 1);
        ap.ssid_len = static_cast<u8>(strlen(kHostSsid));

        g_startDirty = false;
        g_snapDirty = false;
        g_byeSeen = false;
        g_isClient = true;
        g_isHost = false;
        g_clientLost = false;
        g_clientLostFrames = 0;
        g_everAssociated = false;

        Wifi_ConnectOpenAP(&ap);
        return true;
    }

    bool ClientConnectTo(int idx)
    {
        if (!g_isClient || idx < 0 || idx >= Wifi_GetNumAP()) return false;
        Wifi_AccessPoint ap;
        Wifi_GetAPData(idx, &ap);

        // Register the from-host handler here (after scanning, right before
        // connecting) like the dswifi examples do: entering scan mode clears the
        // handler set in StartClientScan(), so registering it only there would
        // mean the host's start frame / snapshots are never received.
        Wifi_MultiplayerFromHostSetPacketHandler(FromHostHandler);

        Wifi_ConnectOpenAP(&ap);
        return true;
    }

    int ClientAssocStatus()
    {
        return Wifi_AssocStatus();
    }

    bool ClientAssociated()
    {
        return Wifi_AssocStatus() == ASSOCSTATUS_ASSOCIATED;
    }

    bool ClientConnectFailed()
    {
        return Wifi_AssocStatus() == ASSOCSTATUS_CANNOTCONNECT;
    }

    void ClientDriveCycle()
    {
        if (!g_isClient) return;
        static uint16_t beat = 0;
        ++beat;
        Wifi_MultiplayerClientReplyTxFrame(&beat, sizeof(beat));
    }

    void ClientSendIntent(const GameNetIntent& intent)
    {
        if (!g_isClient) return;
        yas::shared_buffer sb = yas::save<kYasNetFlag>(intent);
        uint8_t frame[kMsgCap];
        int len = FrameMessage(frame, NetMsgType::Intent, sb);
        if (len > 0) Wifi_MultiplayerClientToHostDataTxFrame(frame, len);
    }

    void ClientSendHello(const GameNetHello& hello)
    {
        if (!g_isClient) return;
        yas::shared_buffer sb = yas::save<kYasNetFlag>(hello);
        uint8_t frame[kMsgCap];
        int len = FrameMessage(frame, NetMsgType::Hello, sb);
        if (len > 0) Wifi_MultiplayerClientToHostDataTxFrame(frame, len);
    }

    bool ClientPollStart(GameNetStart& out)
    {
        return PollInto(g_startDirty, g_startBuf, g_startLen, out);
    }

    bool ClientPollSnapshot(GameNetSnapshot& out)
    {
        return PollInto(g_snapDirty, g_snapBuf, g_snapLen, out);
    }

    bool ClientLostHost()
    {
        if (!g_isClient) return g_clientLost;

        int s = Wifi_AssocStatus();
        if (s == ASSOCSTATUS_ASSOCIATED)
        {
            g_everAssociated = true;
            g_clientLostFrames = 0;
        }
        else if (g_everAssociated)
        {
            // Same reasoning as HostLostClient: a single frame that isn't
            // ASSOCIATED is not a lost host, and treating it as one dropped the
            // player out of a perfectly good game with no explanation.
            if (++g_clientLostFrames >= kLinkLossFrames) g_clientLost = true;
        }
        return g_clientLost;
    }

    bool ClientHostSaidBye()
    {
        return g_byeSeen;
    }

    // ---------------------------------------------------- DS Download Play ---

    bool DlPlayStart(const void* rom, std::size_t romSize, int maxPlayers)
    {
        if (!InitWifi()) return false;
        if (g_isDlPlay) return true;

        // The normal program frame (510): this path streams the child in full
        // blocks, where the larger frame is more efficient.
        Wifi_DlPlaySetSmallFrame(false);

        if (maxPlayers < 2) maxPlayers = 2;
        // Counting the host. Staying at 12 or below keeps the protocol on its
        // 504-byte frame; past that it drops to 250 and the transfer halves in
        // speed for everyone.
        if (maxPlayers > kMaxDlPlayGuests + 1) maxPlayers = kMaxDlPlayGuests + 1;

        // The room also has to fit in the beacon. The game information record
        // takes nine fragments and the list of players one more per group of
        // four, and a scanning console only treats the record as complete once
        // it holds every fragment -- so a room too big for the table is a room
        // nobody can see, which is the same symptom as announcing a player with
        // no record for it and just as quiet.
        static_assert(9 + (((kMaxDlPlayGuests - 1) / 4) + 1)
                          <= DSWIFI_BEACON_MAX_FRAGMENTS,
                      "kMaxDlPlayGuests needs more beacon fragments than there are");

        // Tell the guests where to find us once the transfer is over. Without
        // this they would have to guess the channel or sweep for it, and both
        // cost the player seconds on the join screen.
        g_dlParam = {};
        g_dlParam.magic = kDlPlayParamMagic;
        g_dlParam.channel = static_cast<uint8_t>(kHostChannel);
        static_assert(sizeof(g_dlParam) == DSWIFI_DLPLAY_USER_PARAM_SIZE,
                      "the user parameter is a fixed 32 bytes");

        // Nothing has been announced for this session yet, so the first
        // DlPlaySetRoomInfo() call always gets through its own change guard.
        g_dlLastGuests = -1;
        g_dlLastSeats = -1;
        g_dlLastPhase = -1;

        BuildRoomDescription(0, maxPlayers, DlPlayRoomPhase::Open);

        Wifi_DlPlayInfo info = {};
        // Title and icon are left NULL and come from the child ROM's own banner;
        // the host name comes from the DS firmware. The description is ours,
        // because it says what the room is doing rather than what the program is.
        info.description = g_dlDesc;
        info.description_len = g_dlDescLen;
        info.max_players = static_cast<u8>(maxPlayers);
        info.user_param = &g_dlParam;

        if (Wifi_DlPlayStart(rom, romSize, &info) != 0)
            return false;

        // Hold every guest with the program in memory instead of letting each
        // boot the moment it finishes. The game needs them all to leave the
        // session together, because the host can't serve the binary and run the
        // ordinary multiplayer protocol at the same time.
        //
        // This has to come AFTER Wifi_DlPlayStart, which resets the mode to
        // automatic as part of setting up the session (protocol.c:1900).
        // Setting it first -- the way the API reads -- was silently undone, so
        // guests booted themselves the instant they hit 100% and vanished from
        // the room before the host could press Start.
        Wifi_DlPlaySetBootMode(WIFI_DLPLAY_BOOT_MANUAL);

        // Hold arrivals to begin with, so consoles that turn up together take the
        // program together. The menu releases them after a short quiet spell --
        // see DlPlayHoldNewGuests().
        //
        // This is not the old "hold everyone until the host presses Start". That
        // was here because a second console appeared unable to find a host that
        // was already streaming, and that diagnosis was wrong: the room was
        // disappearing because the beacon announced a player without a record
        // describing it, so a console collecting the game information waited for
        // a record that never came. With the beacon carrying a name and a colour
        // for everyone it lists, the room stays on the air while the host sends
        // -- the announced attribute is the same value idle or streaming, and
        // only a locked or full room clears "accepting entries".
        //
        // What is left is a much smaller point about throughput. Guests share one
        // stream whose cursor follows whichever of them is furthest along, and a
        // console that joins late rides that to the end before wrapping round for
        // what it missed -- so it is never stuck, but the room pays for close to
        // a second pass. Guests released together need only one. A few seconds of
        // holding at the start is what turns the usual case, where everybody
        // picks Download Play at roughly the same moment, into that single pass.
        //
        // Must come after Wifi_DlPlayStart, which clears it.
        Wifi_DlPlaySetGather(true);

        g_isDlPlay = true;
        g_isHost = false;
        g_isClient = false;
        return true;
    }

    void DlPlayHoldNewGuests(bool hold)
    {
        if (!g_isDlPlay) return;

        // Only ever consulted where a guest moves from "has the boot
        // information" to "is downloading", so this holds arrivals without
        // touching anyone already taking the program.
        Wifi_DlPlaySetGather(hold);
    }

    void DlPlayStop()
    {
        if (!g_isDlPlay) return;
        Wifi_DlPlayStop();
        g_isDlPlay = false;
    }

    void DlPlayUpdate()
    {
        if (!g_isDlPlay) return;
        Wifi_DlPlayUpdate();
    }

    bool DlPlayActive()
    {
        return g_isDlPlay;
    }

    void DlPlayLockRoom(bool locked)
    {
        if (!g_isDlPlay) return;
        Wifi_DlPlayLockRoom(locked);
    }

    void DlPlaySetRoomInfo(int guests, int maxSeats, DlPlayRoomPhase phase)
    {
        if (!g_isDlPlay) return;

        if (guests < 0) guests = 0;
        if (maxSeats < 1) maxSeats = 1;

        // Say nothing when there is nothing to say. Handing the library a new
        // record restarts the cycle of beacon fragments from the first one, and
        // a console collecting the record has to see the whole cycle -- so a
        // value that changed every frame would keep the room permanently
        // unreadable rather than permanently up to date.
        const int phaseValue = static_cast<int>(phase);
        if ((guests == g_dlLastGuests) && (maxSeats == g_dlLastSeats) &&
            (phaseValue == g_dlLastPhase))
            return;

        g_dlLastGuests = guests;
        g_dlLastSeats = maxSeats;
        g_dlLastPhase = phaseValue;

        BuildRoomDescription(guests, maxSeats, phase);

        // Rebuilt whole: Wifi_DlPlaySetInfo() replaces every field it reads, so
        // anything left out here would be dropped from the announcement rather
        // than kept. max_players and user_param are ignored by it -- the room
        // size and the boot parameter are settled when the host starts -- but
        // they are filled in so this stays a faithful copy of what DlPlayStart()
        // asked for.
        Wifi_DlPlayInfo info = {};
        info.description = g_dlDesc;
        info.description_len = g_dlDescLen;
        info.max_players = static_cast<u8>(maxSeats);
        info.user_param = &g_dlParam;

        Wifi_DlPlaySetInfo(&info);

        // The same numbers again, in the eight bytes that travel next to the
        // list of players. A console reads these without joining the room, so
        // anything that wants to know what is going on in there can, without
        // taking a seat to find out.
        SkyjoDlPlayVolat volat = {};
        volat.magic0 = 'S';
        volat.magic1 = 'K';
        volat.version = kDlPlayVolatVersion;
        volat.phase = static_cast<uint8_t>(phase);
        volat.guests = static_cast<uint8_t>(guests);
        volat.maxSeats = static_cast<uint8_t>(maxSeats);

        const int free = maxSeats - guests - 1;
        volat.seatsFree = static_cast<uint8_t>((free > 0) ? free : 0);

        static_assert(sizeof(volat) == DSWIFI_DLPLAY_USER_VOLAT_SIZE,
                      "the volatile beacon data is a fixed 8 bytes");

        Wifi_DlPlaySetUserVolatData(&volat);
    }

    bool DlPlayGuestPresent(int aid)
    {
        if (!g_isDlPlay || aid < 1 || aid > kMaxDlPlayGuests) return false;
        return (Wifi_DlPlayGetClientMask() & (1u << aid)) != 0;
    }

    GuestPhase DlPlayGuestState(int aid)
    {
        if (!DlPlayGuestPresent(aid)) return GuestPhase::Absent;

        switch (Wifi_DlPlayGetClientState(aid))
        {
            case WIFI_DLPLAY_CONNECTING: return GuestPhase::Joining;
            case WIFI_DLPLAY_VERIFYING:  return GuestPhase::Verifying;
            case WIFI_DLPLAY_SENDING:    return GuestPhase::Sending;
            case WIFI_DLPLAY_BOOTING:    return GuestPhase::Ready;
            case WIFI_DLPLAY_ERROR:      return GuestPhase::Failed;
            // IDLE for a present guest means it has associated but the library
            // has nothing to say about it yet, which is the same thing as
            // Joining from where the host is standing. STATION cannot happen:
            // this build offers no station content.
            default:                     return GuestPhase::Joining;
        }
    }

    void DlPlayGuestBlocks(int aid, int* held, int* total)
    {
        if (held  != nullptr) *held = 0;
        if (total != nullptr) *total = 0;
        if (!DlPlayGuestPresent(aid)) return;
        Wifi_DlPlayGetClientProgress(aid, held, total);
    }

    unsigned DlPlayGaveUpCount()
    {
        if (!g_isDlPlay) return 0;
        const Wifi_DlPlayDiag* d = Wifi_DlPlayGetDiag();
        if (d == nullptr) return 0;

        unsigned n = 0;
        for (int i = 0; i < WIFI_DLPLAY_GAVE_UP_COUNT; ++i)
            n += d->gave_up[i];
        return n;
    }

    int DlPlayGaveUpLastAid()
    {
        if (!g_isDlPlay) return 0;
        const Wifi_DlPlayDiag* d = Wifi_DlPlayGetDiag();
        return (d != nullptr) ? d->gave_up_last_aid : 0;
    }

    uint16_t DlPlayGuestMask()
    {
        if (!g_isDlPlay) return 0;
        uint16_t mask = 0;
        for (int aid = 1; aid <= kMaxDlPlayGuests; ++aid)
            if (DlPlayGuestPresent(aid)) mask |= static_cast<uint16_t>(1u << aid);
        return mask;
    }

    int DlPlayNumGuests()
    {
        int n = 0;
        for (int aid = 1; aid <= kMaxDlPlayGuests; ++aid)
            if (DlPlayGuestPresent(aid)) ++n;
        return n;
    }

    const char* DlPlayGuestName(int aid)
    {
        if (!DlPlayGuestPresent(aid)) return nullptr;
        return Wifi_DlPlayGetClientNameByAID(aid);
    }

    int DlPlayGuestPercent(int aid)
    {
        if (!DlPlayGuestPresent(aid)) return 0;

        int current = 0;
        int total = 0;
        Wifi_DlPlayGetClientProgress(aid, &current, &total);
        if (total <= 0) return 0;
        if (current >= total) return 100;
        return (current * 100) / total;
    }

    bool DlPlayGuestReady(int aid)
    {
        if (!DlPlayGuestPresent(aid)) return false;

        // BOOTING is what the library reports once a guest holds the whole
        // program; with the manual boot mode it stays there until we say so.
        return Wifi_DlPlayGetClientState(aid) == WIFI_DLPLAY_BOOTING;
    }

    bool DlPlayAllGuestsReady()
    {
        int guests = 0;
        for (int aid = 1; aid <= kMaxDlPlayGuests; ++aid)
        {
            if (!DlPlayGuestPresent(aid)) continue;
            if (!DlPlayGuestReady(aid)) return false;
            ++guests;
        }
        return guests > 0;
    }

    bool DlPlayBootAll()
    {
        if (!g_isDlPlay) return false;
        return Wifi_DlPlayBootAll();
    }
}
