#pragma once

// Wire protocol for DS-to-DS local multiplayer (host-authoritative).
//
// The host runs the full GameParty simulation. After every state change it
// broadcasts a compact GameNetSnapshot to all clients; clients render from it
// and send a tiny GameNetIntent up whenever the local player acts. None of the
// hidden information (face-down card values, deck order) is ever transmitted.
//
// Every payload is framed as [u8 NetMsgType][yas blob]. The yas blobs use the
// in-memory binary, header-less flag (see kYasNetFlag) so they round-trip the
// same way the suspend-save does, just over RAM buffers instead of a file.

#include "../globalHeader.hpp"

// Beacon "game id" used so the join screen only lists Skyjo hosts.
constexpr uint32_t kSkyjoGameId = 0x534B594Au; // "SKYJ"

// Value sentinels (CardType only ever spans -2..12, so these never collide).
constexpr int8_t kNoCard = 127;     // empty pile / no held card
constexpr int8_t kNoIndex = -1;     // optional player index "not set"

// yas flag for the in-RAM network blobs.
constexpr std::size_t kYasNetFlag = yas::mem | yas::binary | yas::no_header;

enum class NetMsgType : uint8_t
{
    Start    = 1, // host -> client, sent once when the game begins
    Snapshot = 2, // host -> client, sent on every state change
    Intent   = 3, // client -> host, sent when the local player acts
    Hello    = 4, // client -> host, sent in the lobby to announce the console name
};

// Sent repeatedly by a client while in the join lobby so the host can build the
// roster with the real player names instead of "Player N" placeholders.
struct GameNetHello
{
    std::string name;

    YAS_DEFINE_STRUCT_SERIALIZE("GameNetHello", name);
};

// One hand slot as seen by a client: its reveal state plus the card value, which
// is only meaningful (and only sent) once the slot is Returned or Cleared.
struct NetSlot
{
    int8_t state = static_cast<int8_t>(CardReturn::Unreturned); // CardReturn
    int8_t value = kNoCard;                                     // CardType

    YAS_DEFINE_STRUCT_SERIALIZE("NetSlot", state, value);
};

// Sent once at game start: tells the client which seat it occupies and the full
// roster (names + count). Everything else arrives via snapshots.
struct GameNetStart
{
    uint8_t playerCount = 0;
    uint8_t seatIndex   = 0; // this client's player index (1..N-1)
    std::vector<std::string> names;

    YAS_DEFINE_STRUCT_SERIALIZE("GameNetStart", playerCount, seatIndex, names);
};

// Full renderable state of the game, minus hidden information.
struct GameNetSnapshot
{
    uint8_t phase       = 0; // GamePhase
    uint8_t playerCount = 0;
    int8_t  currentPlayerIndex  = 0;
    int8_t  startingPlayerIndex = 0;
    int8_t  lastRoundTrigger    = kNoIndex; // player index or kNoIndex
    uint8_t awaitingDiscardReveal = 0;
    int8_t  drawSource  = kNoIndex; // -1 none, 0 Stack, 1 Discard
    int8_t  heldCard    = kNoCard;  // value held by the active player
    int8_t  discardTop  = kNoCard;  // top of the discard pile
    uint16_t cardStackCount = 0;    // draw-pile height (for rendering only)

    // Monotonic SFX event counters. The host bumps one each time a sound-worthy
    // event happens; the client plays the matching effect when a counter changes
    // (edge-detected, so it survives snapshot coalescing).
    uint16_t sfxPose  = 0; // card placed into the grid
    uint16_t sfxTake  = 0; // card taken from a pile, or revealed
    uint16_t sfxClear = 0; // a full matching column cleared

    std::vector<std::array<NetSlot, 12>> hands; // one row per player
    std::vector<int32_t> finalScores;           // only populated at scoring/end

    YAS_DEFINE_STRUCT_SERIALIZE("GameNetSnapshot", phase, playerCount,
        currentPlayerIndex, startingPlayerIndex, lastRoundTrigger,
        awaitingDiscardReveal, drawSource, heldCard, discardTop, cardStackCount,
        sfxPose, sfxTake, sfxClear,
        hands, finalScores);
};

// A single decision from a client, mirroring the four IPlayerController choices.
enum class NetIntentKind : uint8_t
{
    InitialReveal = 0, // arg = slot
    DrawStack     = 1, // arg unused
    DrawDiscard   = 2, // arg unused
    StackReplace  = 3, // arg = slot
    StackFlipOnly = 4, // arg unused
    DiscardReplace = 5 // arg = slot
};

struct GameNetIntent
{
    uint8_t kind = 0; // NetIntentKind
    int8_t  arg  = kNoIndex;

    YAS_DEFINE_STRUCT_SERIALIZE("GameNetIntent", kind, arg);
};
