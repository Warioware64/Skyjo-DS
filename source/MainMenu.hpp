#pragma once


#include "globalHeader.hpp"

enum class MainMenuStates
{
    MainTitle,
    MainSelectionMenu
};

class MainMenu
{
    //friend class Process;
    private:
        void SCREEN_TOP();
        void SCREEN_BOTTOM();

        // Hex background as a hardware 2D BG on each engine instead of a
        // 3D sprite. Drawing order is now decided by BG-layer priority, so
        // the rich-text 3D quads naturally render above without us having
        // to reorder NEA_SpriteDraw calls.
        NEA_Hw2DBG *hexBGtop;
        NEA_Hw2DBG *hexBGbot;
        
        NEA_Material *PlayMat[2];
        NEA_Palette *PlayPal[2];

        NEA_GUIObj *PlayButton;
        MainMenuStates mainmenustates = MainMenuStates::MainTitle;

        int showOrNotTouchScreenText = 1;
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