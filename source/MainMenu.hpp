#pragma once


#include "globalHeader.hpp"
#include "Process.hpp"
#include "MainMenuClasses/MainMenuStates.hpp"
#include "MainMenuClasses/MainSelectionMenu.hpp"
#include "MainMenuClasses/PlaySelectionMenu.hpp"
#include "MainMenuClasses/OnePlayerPartyStart.hpp"


class MainMenu
{
    //friend class Process;
    private:
        enum class FadePhase { None, FadingOut, FadingIn };

        void SCREEN_TOP();
        void SCREEN_BOTTOM();

        void ProcessLogicMainTitle();
        // Begin a white-fade transition to the next menu. The current menu's
        // Unload + the next menu's Load are deferred until the fade apex so
        // the asset swap is hidden behind a fully white screen.
        void StartTransitionTo(MainMenuStates next);
        // Hex background as a hardware 2D BG on each engine instead of a
        // 3D sprite. Drawing order is now decided by BG-layer priority, so
        // the rich-text 3D quads naturally render above without us having
        // to reorder NEA_SpriteDraw calls.
        NEA_Hw2DBG *hexBGtop;
        NEA_Hw2DBG *hexBGbot;

        NEA_Material *hexParMat;
        NEA_Palette *hexParPal;
        NEA_ParticleEmitter *hexEmit;
        NEA_Camera *emitCam;

        MainSelectionMenu mainSelec;
        PlaySelectionMenu playSelec;
        OnePlayerPartyStart onePlayerParty;


        MainMenuStates mainmenustates = MainMenuStates::MainTitle;
        MainMenuStates OLDmainmenustates;

        uint32_t keys;

        bool triggerPlayPartyOnePlayer = false;



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
        void RenderMainMenu();
};

extern MainMenu mainmenu;
