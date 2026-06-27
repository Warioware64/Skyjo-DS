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

    // Leave whatever mode we are in and go idle. Safe to call repeatedly.
    void Shutdown();

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

    // True once a client that was connected during the game has dropped.
    bool HostLostClient();

    // -------------------------------------------------------------- Client ---

    // Enter client mode and start scanning for hosts. Returns false on failure.
    bool StartClientScan();

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

    // Pop a pending game-start / snapshot message from the host, if any.
    bool ClientPollStart(GameNetStart& out);
    bool ClientPollSnapshot(GameNetSnapshot& out);

    // True once association with the host has been lost.
    bool ClientLostHost();
}
