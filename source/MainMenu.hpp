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

        NEA_Material *hexBGmat;
        NEA_Palette *hexBGpal;
        NEA_Sprite *hexBGspr[2];
        
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