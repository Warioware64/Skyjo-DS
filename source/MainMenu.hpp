#pragma once


#include "GameParty.hpp"
#include "globalHeader.hpp"
#include "Process.hpp"
#include "GameParty.hpp"
#include "MainMenuClasses/MainMenuStates.hpp"
#include "MainMenuClasses/MainSelectionMenu.hpp"
#include "MainMenuClasses/PlaySelectionMenu.hpp"
#include "MainMenuClasses/OnePlayerPartyStart.hpp"
#include "MainMenuClasses/MultiplayerFirstMenu.hpp"
#include "MainMenuClasses/MultiplayerHostMenu.hpp"
#include "MainMenuClasses/MultiplayerJoinMenu.hpp"
#include "MainMenuClasses/SettingsMenu.hpp"

class MainMenu
{
    friend GameParty;
    private:
        enum class FadePhase { None, FadingOut, FadingIn };

        void SCREEN_TOP();
        void SCREEN_BOTTOM();

        void ProcessLogicMainTitle();
        // Begin a white-fade transition to the next menu. The current menu's
        // Unload + the next menu's Load are deferred until the fade apex so
        // the asset swap is hidden behind a fully white screen.
        void StartTransitionTo(MainMenuStates next);
        void CreateHexBackgrounds();

        // Hex background as a hardware 2D BG on each engine instead of a
        // 3D sprite. Drawing order is decided by BG-layer priority, so the
        // rich-text 3D quads render above without any submission-order tricks.
        //
        // Created in LoadAssetsMainMenu and freed in UnloadAssetsMainMenu. The
        // game party claims the same two layers, so the delete on the way out
        // is what lets it create its own, and vice versa on the way back.
        NEA_Hw2DBG *hexBGtop = nullptr;
        NEA_Hw2DBG *hexBGbot = nullptr;

        NEA_Material *hexParMat = nullptr;
        NEA_Palette *hexParPal = nullptr;
        NEA_ParticleEmitter *hexEmit = nullptr;
        NEA_Camera *emitCam = nullptr;

        MainSelectionMenu mainSelec;
        PlaySelectionMenu playSelec;
        OnePlayerPartyStart onePlayerParty;
        MultiplayerFirstMenu multiplayerFirstMenu;
        MultiplayerHostMenu multiplayerHostmenu;
        MultiplayerJoinMenu multiplayerJoinmenu;
        SettingsMenu settingsMenu;

        MainMenuStates mainmenustates = MainMenuStates::MainTitle;
        MainMenuStates OLDmainmenustates;

        uint32_t keys;

        bool triggerPlayPartyOnePlayer = false;
        bool triggerPlayPartyMultiplayerHost = false;
        bool triggerPlayPartyMultiplayerClient = false;


        bool bypassableChangeMenuStates = false;
        int showOrNotTouchScreenText = 1;
        int frameTouchDetect = 0;
        bool triggerCanTouchDetect = false;
        bool canTouchDetect = false;
        int frameTrigger = 0;
        int brightness = 16;

        int emitFrame;

        // Drives the brightness fade. Initial boot is FadingIn at the slow
        // intro speed; menu-to-menu switches use a quicker speed via
        // fadeStepInterval. pendingNextState is consumed at the fade apex.
        FadePhase fadePhase = FadePhase::FadingIn;
        int fadeStepInterval = 5;
        std::optional<MainMenuStates> pendingNextState;

    public:
        MainMenu();
        ~MainMenu();

        void LoadAssetsMainMenu();
        void UnloadAssetsMainMenu();
        void RenderMainMenu();
};

extern MainMenu mainmenu;
