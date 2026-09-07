#pragma once

// Thin C++ wrapper around DSWiFi local (DS-to-DS) multiplayer for Skyjo.
//
// dswifi packet handlers run in interrupt context, so they only ever copy raw
// bytes into fixed buffers and raise a "dirty" flag. All yas (de)serialization
// happens on the main thread when the menu / game loop polls NetLink. The whole
// dswifi surface is contained in NetLink.cpp; callers stay header-clean.

#include "NetProtocol.hpp"

namespace NetLink
{
    // Up to 8 players: host (seat 0) + 7 clients (AID / seat 1..7).
    constexpr int kMaxClients = 7;

    // Initialise the WiFi stack once (idempotent). Returns false on failure.
    bool InitWifi();

    // Why InitWifi() last failed, from the library (Wifi_GetInitFailStage()).
    // Meaningless unless something actually failed; it exists so a console with
    // no console output can still say what went wrong.
    int InitFailStage();

    // Leave whatever mode we are in and go idle, and forget everything the
    // session accumulated -- association state, the "link lost" latches, and any
    // frame still sitting unread in a receive buffer. Safe to call repeatedly,
    // and safe to call when nothing was ever started.
    //
    // Every way out of a party goes through here, so callers do not have to
    // remember which pieces of state their particular exit needs to undo.
    void Shutdown();

    // Shutdown(), then hand the hardware and the library itself back: the radio
    // is disengaged and the shared structure freed, so the next InitWifi() runs
    // a full Wifi_InitDefault() instead of resuming whatever the last session
    // left behind.
    //
    // Only call this with no link in use. It belongs at the point the game
    // returns to the title screen, not at the end of every party: the Download
    // Play handoff deliberately keeps the link alive from one screen to the next.
    void PowerDown();


    // ---------------------------------------------------------------- Host ---

    // Enter host mode and start beaconing as a Skyjo host. Returns false on
    // failure. New clients are allowed to join until LockLobby() is called.
    bool StartHost();

    int      HostNumClients();          // connected clients (excludes host)
    uint16_t HostClientMask();          // bit i set => seat i connected (bit0=host)
    void     LockLobby();               // stop accepting new clients

    // Drive the multiplayer transfer cycle. On NDS hardware the host only
    // physically transmits queued host->client frames (and opens client reply
    // slots) when it sends a CMD frame, so this must be called once per frame on
    // the host whenever it is connected (lobby and in-game). Without it, DATA
    // frames such as the start handshake and snapshots never reach clients.
    void     HostDriveCycle();

    // Tell a single client which seat it has and the full roster (game start).
    void HostSendStart(int seat, const GameNetStart& start);
    // Broadcast the current renderable state to every connected client.
    void HostBroadcastSnapshot(const GameNetSnapshot& snap);

    // Pop the most recent intent received from a client seat, if any.
    bool HostPollIntent(int seat, GameNetIntent& out);

    // Pop the most recent lobby "hello" (console name) from a client seat, if any.
    bool HostPollHello(int seat, GameNetHello& out);

    // True once a client that was connected during the game has dropped.
    bool HostLostClient();

    // Tell every connected client that this host is leaving, then return.
    //
    // Blocks for a fraction of a second: queued host->client DATA frames only go
    // out when a CMD frame is sent, so the message has to be repeated across a
    // handful of drive cycles for a client that missed one to still get it. Call
    // it immediately before Shutdown() on a deliberate exit -- there is nothing
    // to say when the link is already broken.
    void HostAnnounceBye(NetByeReason reason);


    // -------------------------------------------------------------- Client ---

    // The channel StartHost() beacons on. Fixed, so a client that already knows
    // which console to join can go straight there instead of scanning.
    constexpr int kHostChannel = 6;

    // Enter client mode and start scanning for hosts. Returns false on failure.
    bool StartClientScan();

    // Connect straight to a known host by MAC and channel, with no scan at all.
    // For a Download Play guest, which is told the host's address by the loader
    // that started it and its channel through SkyjoDlPlayParam. Returns false
    // only if wifi could not be brought up; whether the host is actually there
    // shows up through ClientAssocStatus() as usual.
    bool ClientConnectDirect(const uint8_t bssid[6], int channel);

    // Re-issue a direct connect on a stack that is already in client mode.
    //
    // Unlike ClientConnectDirect() this changes no mode and tears nothing down,
    // so the ARM7 keeps listening across the retry. That matters when the host
    // is not on the air yet: the expensive path goes deaf for the seconds it
    // spends re-entering client mode, which is long enough to miss the beacon it
    // is waiting for and lose another whole retry interval.
    bool ClientReconnectDirect(const uint8_t bssid[6], int channel);

    int  ClientNumAP();
    // Read scan entry `idx`; fills name/current/max if non-null. Returns false
    // for hosts that aren't accepting connections or out-of-range indices.
    bool ClientGetAPInfo(int idx, char* nameOut, int nameCap,
                         int* playersCur, int* playersMax);
    // Begin associating with the host at scan index `idx`.
    bool ClientConnectTo(int idx);

    // ASSOCSTATUS_* from dswifi (ASSOCSTATUS_ASSOCIATED == connected).
    int  ClientAssocStatus();
    bool ClientAssociated();
    bool ClientConnectFailed(); // association reached the "cannot connect" state

    // Client counterpart to HostDriveCycle: prepare a reply frame each frame so
    // the client transmits in its MP reply slot (needed for client->host DATA
    // such as intents to reach the host). Call once per frame while connected.
    void ClientDriveCycle();

    void ClientSendIntent(const GameNetIntent& intent);

    // Announce this console's player name to the host during the join lobby.
    void ClientSendHello(const GameNetHello& hello);

    // Pop a pending game-start / snapshot message from the host, if any.
    bool ClientPollStart(GameNetStart& out);
    bool ClientPollSnapshot(GameNetSnapshot& out);

    // True once association with the host has been lost.
    bool ClientLostHost();

    // True once the host has said it is leaving. Latched, and cleared by
    // Shutdown() and by every Start* entry point, so it always refers to the
    // session in progress. Unlike ClientLostHost() it is immediate, which is the
    // whole point: a host that quits on purpose says so.
    bool ClientHostSaidBye();


    // ------------------------------------------------- DS Download Play ---
    //
    // Used only to hand the child binary to guest consoles. Once every guest
    // has booted it, DlPlayStop() ends the session and the game switches to the
    // ordinary host/client protocol above -- the guests rejoin by scanning for
    // the SKYJO beacon like any other client. Nothing of the game itself goes
    // over the Download Play channel, which only carries 64 bytes down and 7
    // bytes up per cycle.

    // Guest association IDs run 1..kMaxDlPlayGuests.
    constexpr int kMaxDlPlayGuests = 7;

    // Begin announcing `rom` (a whole .nds image) in the Download Play menu of
    // nearby consoles. Arrivals are held only long enough for the room to
    // settle (DlPlayHoldNewGuests), never until the host is ready; they are then
    // only stopped from *booting* what they downloaded, so the room still leaves
    // together. See DlPlayStart().
    // The buffer must stay valid and unmodified until
    // DlPlayStop(): the library reads blocks straight out of it rather than
    // taking a copy. Returns false if wifi failed or the ROM is unusable (too
    // big for the protocol, or not a valid NDS image).
    bool DlPlayStart(const void* rom, std::size_t romSize, int maxPlayers);

    // Hold guests that arrive from now on at the point just before their
    // download begins, or let them go. Guests already taking the program are
    // unaffected either way, so this admits consoles in batches rather than
    // stopping and starting a transfer.
    //
    // A session starts out holding (see DlPlayStart); the caller decides when
    // the room has settled enough to release it.
    void DlPlayHoldNewGuests(bool hold);

    // End the session and go idle. Safe to call when no session is running.
    void DlPlayStop();

    // Advance the transfer. Must be called once per frame while serving, or the
    // transfer stalls waiting for a frame that never goes out.
    void DlPlayUpdate();

    // True while a Download Play session is running.
    bool DlPlayActive();

    // Stop (or resume) accepting new guests. A locked room disappears from the
    // Download Play menu of consoles that are scanning.
    void DlPlayLockRoom(bool locked);

    // Announce what the room is doing, so a console that is only looking at it
    // sees more than the child ROM's banner. The description shown in the
    // Download Play menu is rebuilt from these, and the same numbers go out as
    // SkyjoDlPlayVolat next to the list of players.
    //
    // Safe to call every frame: nothing is sent unless one of the values
    // actually changed. That guard is not an optimisation -- every real change
    // restarts the ~1 s cycle of beacon fragments, and a value that ticks would
    // stop the record from ever reaching a scanning console.
    void DlPlaySetRoomInfo(int guests, int maxSeats, DlPlayRoomPhase phase);

    // Where one guest has got to. Mirrors the library's own per-client state,
    // which is finer than "ready or not": with guests held in the room before
    // any download starts, most of them spend the wait at Joining, and calling
    // that "0%" reads as a transfer that is stuck rather than one that has
    // deliberately not begun.
    enum class GuestPhase
    {
        Absent,     // no guest on this AID
        Joining,    // associated; the host is asking for its name
        Verifying,  // has the boot information, checking the signature
        Sending,    // taking the program
        Ready,      // holds all of it, waiting to be started
        Failed,     // the library reports this one as errored
    };
    GuestPhase DlPlayGuestState(int aid);

    // Blocks this guest says it holds, and how many there are. What the guest
    // reports, not what the host has sent -- guests share one stream and keep
    // whichever blocks they still need, so they finish at different times.
    void DlPlayGuestBlocks(int aid, int* held, int* total);

    // Guests the library stopped waiting for, all reasons together, and the last
    // one it gave up on. Without this a guest dropped by a timeout looks exactly
    // like one whose owner walked away, and the host says nothing about either.
    unsigned DlPlayGaveUpCount();
    int      DlPlayGaveUpLastAid();

    // Guests currently associated, and per-guest state for the lobby display.
    int  DlPlayNumGuests();
    // Bit i set means guest AID i is in the room. Handed out whole so a caller
    // can compare two moments and see who left, which counting cannot.
    uint16_t DlPlayGuestMask();
    bool DlPlayGuestPresent(int aid);
    // Console name the guest reported, or nullptr before it has arrived.
    const char* DlPlayGuestName(int aid);
    // 0..100 for what this guest says it has received.
    int  DlPlayGuestPercent(int aid);
    // True once this guest holds the whole program and is only waiting to boot.
    bool DlPlayGuestReady(int aid);
    // True when every associated guest is ready to boot (and there is at least
    // one). This is the condition for starting the game.
    bool DlPlayAllGuestsReady();

    // Tell every guest that holds the whole program to start it. They leave the
    // session as they boot, so DlPlayNumGuests() drains to zero afterwards.
    bool DlPlayBootAll();
}
