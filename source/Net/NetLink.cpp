#include "NetLink.hpp"

#include <dswifi9.h>
#include <cstring>

// All dswifi access is confined to this file. Received frames are copied raw in
// the IRQ-context packet handlers; yas (de)serialization happens on the main
// thread in the Poll/Send helpers (guarded by REG_IME for the shared buffers).

namespace
{
    constexpr int kMsgCap = 512; // max framed payload (tag + yas blob)
    constexpr int kIntentCap = 64;

    bool g_wifiInited = false;
    bool g_isHost = false;
    bool g_isClient = false;

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

    void Shutdown()
    {
        if (g_wifiInited) Wifi_IdleMode();
        g_isHost = false;
        g_isClient = false;
    }

    // ----------------------------------------------------------------- Host ---

    bool StartHost()
    {
        if (!InitWifi()) return false;

        // CMD/REPLY frames are unused (we drive everything over DATA frames), so
        // the host/client packet sizes are nominal.
        Wifi_MultiplayerHostMode(kMaxClients, 8, 8);
        Wifi_MultiplayerFromClientSetPacketHandler(FromClientHandler);

        while (!Wifi_LibraryModeReady())
            swiWaitForVBlank();

        Wifi_SetChannel(6);

        static const char kName[] = "SKYJO";
        Wifi_MultiplayerHostName(kName, sizeof(kName) - 1);
        Wifi_MultiplayerAllowNewClients(true);
        Wifi_BeaconStart("SKYJO", kSkyjoGameId);

        for (int i = 0; i <= kMaxClients; ++i) { g_intentDirty[i] = false; g_helloDirty[i] = false; }
        g_isHost = true;
        g_isClient = false;
        g_hostLost = false;
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
        if ((g_lockedMask & m) != g_lockedMask) g_hostLost = true;
        return g_hostLost;
    }

    // --------------------------------------------------------------- Client ---

    bool StartClientScan()
    {
        if (!InitWifi()) return false;

        Wifi_MultiplayerClientMode(8);
        Wifi_MultiplayerFromHostSetPacketHandler(FromHostHandler);

        while (!Wifi_LibraryModeReady())
            swiWaitForVBlank();

        Wifi_ScanMode();

        g_startDirty = false;
        g_snapDirty = false;
        g_isClient = true;
        g_isHost = false;
        g_clientLost = false;
        g_everAssociated = false;
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
        if (s == ASSOCSTATUS_ASSOCIATED) g_everAssociated = true;
        else if (g_everAssociated) g_clientLost = true;
        return g_clientLost;
    }
}
