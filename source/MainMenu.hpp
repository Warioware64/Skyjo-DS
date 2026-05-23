#pragma once


#include "globalHeader.hpp"
#include "MainMenuClasses/MainMenuStates.hpp"
#include "MainMenuClasses/MainSelectionMenu.hpp"
#include "MainMenuClasses/PlaySelectionMenu.hpp"
#include "MainMenuClasses/OnePlayerPartyStart.hpp"

class MainMenu
{
    //friend class Process;
    private:
        void SCREEN_TOP();
        void SCREEN_BOTTOM();

        void ProcessLogicMainTitle();
        // Hex background as a hardware 2D BG on each engine instead of a
        // 3D sprite. Drawing order is now decided by BG-layer priority, so
        // the rich-text 3D quads naturally render above without us having
        // to reorder NEA_SpriteDraw calls.
        NEA_Hw2DBG *hexBGtop;
        NEA_Hw2DBG *hexBGbot;

        MainSelectionMenu mainSelec;
        PlaySelectionMenu playSelec;
        OnePlayerPartyStart onePlayerParty;


        MainMenuStates mainmenustates = MainMenuStates::MainTitle;
        MainMenuStates OLDmainmenustates;

        uint32_t keys;

        int showOrNotTouchScreenText = 1;
        int frameTouchDetect = 0;
        bool triggerCanTouchDetect = false;
        bool canTouchDetect = false;
        int frameTrigger = 0;
        int brightness = 16;
        bool frameDoCount;

    public:
        MainMenu();
        ~MainMenu();

        void LoadAssetsMainMenu();
        void RenderMainMenu();
};

extern MainMenu mainmenu;
