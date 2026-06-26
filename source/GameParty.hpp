#pragma once

#include "globalHeader.hpp"
#include "ErrorHandler.hpp"
#include "DebugPrint.hpp"
#include "GamePartyClasses/GamePartySharedAssets.hpp"
#include "GamePartyClasses/PlayerController.hpp"
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
    QuitMenu
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
        void InitCardStack();

        void BuildControllers(int playerCount);
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
        void EndTurn(int playerIdx);
        void AdvanceToNextPlayer();

        std::vector<CardType> cardPreStack;

        NEA_Hw2DBG *hexBGtop;
        NEA_Hw2DBG *hexBGbot;

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

        NEA_Material *NotPossibleIconMat;
        NEA_Palette *NotPossibleIconPal;

        bool partyFirstTwoDraw;
        bool awaitingDiscardReveal;

        // Bottom-screen card animations (player 0 hand).
        uint32_t animTick;
        uint8_t popTimer[12];
        uint8_t clearTimer[12];

        NEA_Sprite *myPacket[12];
        NEA_Sprite *pullpacket[2];
        NEA_Sprite *pullpacketIconNot[2];
        NEA_Sprite *heldCardSprite;


        NEA_Hw2DOBJAsset *OBJ2dCardsAssets[16];
        NEA_Hw2DOBJ *viewGame[12];

        std::vector<std::unique_ptr<IPlayerController>> controllers;
        std::array<int, 12> initialRevealCount; // per-player count revealed during InitialReveal

        CPULevel cpuLevel;
        PartyType partyType;
        int playerCount;

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

        void InitGamePartySituation(int number_arg, CPULevel cpu_arg, PartyType party_arg);
        void RenderGameParty();

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
