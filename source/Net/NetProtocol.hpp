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


// The 32 bytes a Download Play host sends along with the program, which the
// loader leaves at 0x027FFBE0 for the guest to read (GBATEK's entry point
// information; the dswifi dlplay_child example calls it MB_USER_PARAM).
//
// The guest has to rejoin the host *after* the Download Play session ends, and
// by then the host has moved to its own beacon channel -- so the channel the
// loader records, which is the one the transfer ran on, is the wrong one to
// come back to. Rather than have the guest assume where the host will be, the
// host says: it writes the channel it is about to beacon on into these bytes
// before the transfer starts, and the guest reads it back out.
struct SkyjoDlPlayParam
{
    uint32_t magic;   // kDlPlayParamMagic when this is ours
    uint8_t  channel; // where the host will beacon after the session ends
    uint8_t  pad[27];
};

constexpr uint32_t kDlPlayParamMagic = 0x4A594B53u; // "SKYJ" little-endian

// Where the loader leaves the parameter on a console it has just started.
constexpr uintptr_t kDlPlayParamAddress = 0x027FFBE0;

// The eight bytes a Download Play host announces in every beacon frame, next to
// the list of players. Unlike SkyjoDlPlayParam, which is handed to the program
// once it has been sent, these travel on the air the whole time the room is up
// and are read by any console that is only looking at it.
//
// The library doesn't interpret them; what they mean is entirely ours. Keeping
// them in step with the announced description is what makes the room describe
// itself to a scanner instead of just showing the child ROM's banner.
struct SkyjoDlPlayVolat
{
    uint8_t magic0;    // 'S'
    uint8_t magic1;    // 'K'
    uint8_t version;   // kDlPlayVolatVersion
    uint8_t phase;     // DlPlayRoomPhase
    uint8_t guests;    // consoles in the room, host excluded
    uint8_t maxSeats;  // room size, host included
    uint8_t seatsFree; // maxSeats - guests - 1, floored at zero
    uint8_t reserved;
};

static_assert(sizeof(SkyjoDlPlayVolat) == 8,
              "the volatile beacon data is a fixed 8 bytes");

constexpr uint8_t kDlPlayVolatVersion = 1;

// What the room is doing, as announced in SkyjoDlPlayVolat::phase. Mirrors
// MultiplayerDlPlayMenu::Phase, which is private to that screen; this is the
// part of it that goes on the air.
enum class DlPlayRoomPhase : uint8_t
{
    Open    = 0, // taking guests
    Closing = 1, // shut to newcomers, waiting for the last download
    Booting = 2, // guests are starting the program
};

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
    Bye      = 5, // host -> client, sent when the host is about to drop the link
};

// Why the host is leaving. Without this a client can only notice the host by its
// absence, which takes half a second of missing association (ClientLostHost) or
// three seconds of silence (kHostSilentFrames) to become certain -- so a host
// that quit on purpose looked exactly like one whose batteries died, and the
// client sat on a frozen table until the timeout ran out.
enum class NetByeReason : uint8_t
{
    HostEndedGame = 0, // the host quit the party
    HostLeftLobby = 1, // the host backed out of the lobby before starting
};

struct GameNetBye
{
    uint8_t reason = 0; // NetByeReason

    YAS_DEFINE_STRUCT_SERIALIZE("GameNetBye", reason);
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
