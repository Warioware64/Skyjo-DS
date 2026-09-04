#pragma once

#include "globalHeader.hpp"
#include "ErrorHandler.hpp"
#include "DebugPrint.hpp"
#include "GamePartyClasses/GamePartySharedAssets.hpp"
#include "GamePartyClasses/PlayerController.hpp"
#include "Net/NetProtocol.hpp"
#include "Process.hpp"
#include "MainMenu.hpp"
#include <NEAGUI.h>
#include <memory>


using PlayerGames = std::array<CardType, 12>;
using CardReturnType = std::array<CardReturn, 12>;
struct TouchPoseMyCard
{
    int x_min, x_max, y_min, y_max;

    YAS_DEFINE_STRUCT_SERIALIZE("TouchPoseMyCard", x_min, x_max, y_min, y_max);
};

enum class PausePhase
{
    PauseMenuMain,
    QuitMenu,
    SaveMenu
};

// In-game sound-effect kinds (see GameParty::EmitSfx).
enum class SfxKind
{
    Pose,  // a card was placed into the grid
    Take,  // a card was taken from a pile, or revealed
    Clear  // a full matching column was cleared
};

class GameParty
{
    private:
        void GamePartyLogic();

        void DestroyQuitMenu();
        void InitQuitMenu();

        void DestroyPauseMenuMain();
        void PauseMenuGUIlogic();

        void ComputeFinalScores();
        void InitEndGameMenu();
        void DestroyEndGameMenu();
        void EndGameGUIlogic();

        void GamePartyLogicRender();
        void InitPauseMenuGUIbutton();

        void UnloadGamePartyAssets();
        void LoadGamePartyAssets();
        void BuildGamePartyScene();
        void InitCardStack();

        void BuildControllers(int playerCount);

        // Local-multiplayer (host-authoritative) networking helpers.
        GameNetSnapshot BuildSnapshot() const;     // host: pack renderable state
        void ApplySnapshot(const GameNetSnapshot& s); // client: render from state
        void NetHostBroadcastIfChanged();          // host: send snapshot on change
        void NetClientSendInput(IPlayerController& input); // client: emit intents
        std::vector<uint8_t> netLastSnapshot;      // last bytes sent (change detect)

        void TickInitialReveal();
        void TickTurn();
        void TickScoring();
        void HandleTopScreenCycling();
        void RefreshDiscardSprite();
        void RefreshTopScreen();
        void RefreshMyHandSprite(int slot);
        void AnimateHandSprites();
        void ResolveColumnClears(int playerIdx);
        bool HandFullyRevealed(int playerIdx) const;

        // Plays an in-game SFX locally (host / single-player) and bumps the
        // matching authoritative counter so the client can replay it from the
        // snapshot. See EmitSfx in GameParty.cpp.
        void EmitSfx(SfxKind kind);

        // One start per effect per frame. Two copies of the same sample begun on
        // the same frame are sample-aligned, so they sum into a clipped, flanged
        // version of themselves instead of just sounding louder. Reset at the top
        // of GamePartyLogic().
        std::array<bool, 3> sfxEmittedThisFrame{};
        void EndTurn(int playerIdx);
        void AdvanceToNextPlayer();

        std::vector<CardType> cardPreStack;

        //int frameToSeconds = 0;
        //std::chrono::seconds secondCount;
        // The party's own hex backgrounds (main layer 1, sub layer 0). Created
        // in LoadGamePartyAssets and freed in UnloadGamePartyAssets -- the main
        // menu owns the same two layers, so the create/delete pair on each side
        // is what lets the other one claim them.
        NEA_Hw2DBG *hexBGtop = nullptr;
        NEA_Hw2DBG *hexBGbot = nullptr;

        // True between LoadGamePartyAssets() and UnloadGamePartyAssets(). Eight
        // routes end a party and some run in sequence, so teardown checks this
        // to make sure it runs exactly once.
        bool assetsLoaded = false;

        NEA_Material *ContinueButtonMat;
        NEA_Palette *ContinueButtonPal;
        NEA_Material *ContinueButtonPressedMat;
        NEA_Palette *ContinueButtonPressedPal;
        NEA_GUIObj *ContinueButton;

        NEA_Material *QuitButtonMat;
        NEA_Palette *QuitButtonPal;
        NEA_Material *QuitButtonPressedMat;
        NEA_Palette *QuitButtonPressedPal;
        NEA_GUIObj *QuitButton;

        NEA_Material *YesButtonMat;
        NEA_Palette *YesButtonPal;
        NEA_Material *YesButtonPressedMat;
        NEA_Palette *YesButtonPressedPal;
        NEA_GUIObj *YesButton;

        NEA_Material *NoButtonMat;
        NEA_Palette *NoButtonPal;
        NEA_Material *NoButtonPressedMat;
        NEA_Palette *NoButtonPressedPal;
        NEA_GUIObj *NoButton;

        NEA_Material *ReplayButtonMat;
        NEA_Palette *ReplayButtonPal;
        NEA_Material *ReplayButtonPressedMat;
        NEA_Palette *ReplayButtonPressedPal;
        NEA_GUIObj *ReplayButton;

        NEA_Material *ExitButtonMat;
        NEA_Palette *ExitButtonPal;
        NEA_Material *ExitButtonPressedMat;
        NEA_Palette *ExitButtonPressedPal;
        NEA_GUIObj *ExitButton;

        NEA_Material *SaveButtonMat;
        NEA_Palette *SaveButtonPal;
        NEA_Material *SaveButtonPressedMat;
        NEA_Palette *SaveButtonPressedPal;
        NEA_GUIObj *SaveButton;

        NEA_Material *NotPossibleIconMat;
        NEA_Palette *NotPossibleIconPal;

        bool partyFirstTwoDraw;
        bool awaitingDiscardReveal;

        // Bottom-screen card animations (player 0 hand).
        uint32_t animTick;
        uint8_t popTimer[12];
        uint8_t clearTimer[12];

        // Countdown that pauses turn logic right after a hand-off (set in
        // EndTurn), so the next player's turn doesn't start on the very next
        // frame. Host-side only; clients mirror the frozen state via snapshots.
        int turnTransitionFrames = 0;

        // SFX event counters. Host/single-player: bumped by EmitSfx as events
        // happen and mirrored into the snapshot (BuildSnapshot). Client: the
        // *Seen values track the last snapshot's counters so ApplySnapshot can
        // edge-detect changes and replay the sound; sfxSeenInit suppresses a
        // spurious burst on the first applied snapshot. Runtime-only (not
        // serialized).
        uint16_t sfxPoseCount  = 0;
        uint16_t sfxTakeCount  = 0;
        uint16_t sfxClearCount = 0;
        uint16_t sfxPoseSeen   = 0;
        uint16_t sfxTakeSeen   = 0;
        uint16_t sfxClearSeen  = 0;
        bool     sfxSeenInit   = false;

        NEA_Sprite *myPacket[12] = {};
        NEA_Sprite *pullpacket[2] = {};
        NEA_Sprite *pullpacketIconNot[2] = {};
        NEA_Sprite *heldCardSprite = nullptr;


        NEA_Hw2DOBJAsset *OBJ2dCardsAssets[16] = {};
        NEA_Hw2DOBJ *viewGame[12] = {};

        std::vector<std::unique_ptr<IPlayerController>> controllers;
        std::array<int, 12> initialRevealCount; // per-player count revealed during InitialReveal

        CPULevel cpuLevel;
        PartyType partyType;
        int playerCount;
        // Seats [0, humanSeatCount) are human (host + clients); seats
        // [humanSeatCount, playerCount) are host-run CPUs. 1 for single-player.
        int humanSeatCount;

        GamePhase phase;
        int currentPlayerIndex;
        int startingPlayerIndex;
        std::optional<int> lastRoundTriggerPlayerIndex;
        std::optional<DrawSource> drawSource;
        std::optional<CardType> heldCard;

        int topScreenViewPlayerIdx;
        uint32_t prevKeydown;

        bool StartMenu;
        bool Quited;
        bool EndMenu;
        bool Restarted;
        std::vector<int> finalScores;
        PausePhase pausephase;

    public:
        GameParty();
        ~GameParty();

        // Public game state. Controllers (HumanTouchController, CpuController and
        // the strategy namespaces) read these to make their decisions; the state
        // machine in GameParty.cpp is the only writer.
        std::vector<PlayerGames> playerDeck;
        std::vector<CardReturnType> cardReturns;
        std::vector<CardType> cardStack;
        std::vector<CardType> discardPile;
        std::vector<std::string> namePlayers;

        std::array<TouchPoseMyCard, 12> MyCardPos;
        uint32_t keydown;
        touchPosition touchData;

        // The player whose hand is shown on the bottom screen and driven by the
        // local touch input. 0 for single-player and for the multiplayer host;
        // the assigned seat for a multiplayer client. Not serialized (runtime).
        int localPlayerIndex = 0;

        void InitGamePartySituation(int number_arg, CPULevel cpu_arg, PartyType party_arg);
        // Local-multiplayer host: seats [0, humanCount) are human (host + clients,
        // named from the roster), the rest are host-run CPUs.
        void InitGamePartyHost(int playerCnt, int humanCount, CPULevel cpu_arg,
                               const std::vector<std::string>& names);
        void ResumeGamePartySituation();
        void RenderGameParty();

        // Local-multiplayer client: build the mirror scene for an assigned seat
        // and render/drive it purely from host snapshots.
        void InitGamePartyClient(int seatIndex, int playerCnt,
                                 const std::vector<std::string>& names);
        void RenderGamePartyClient();

        // NOTE: 'controllers' is intentionally NOT serialized — it's a vector of
        // std::unique_ptr<IPlayerController> (polymorphic), which yas can't
        // round-trip. Rebuild it after loading with BuildControllers(playerCount);
        // CpuController carries no persistent state beyond cpuLevel.
        YAS_DEFINE_STRUCT_SERIALIZE("GameParty", partyFirstTwoDraw, awaitingDiscardReveal, animTick,
        popTimer, clearTimer, initialRevealCount, cpuLevel, partyType, playerCount,
        phase, currentPlayerIndex, startingPlayerIndex, lastRoundTriggerPlayerIndex, drawSource,
        heldCard, topScreenViewPlayerIdx, playerDeck, cardReturns, cardStack, discardPile,
        namePlayers, MyCardPos);
};

extern GameParty gameparty;
