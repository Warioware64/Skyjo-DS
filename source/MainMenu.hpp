#pragma once


#include "globalHeader.hpp"

enum class MainMenuStates
{
    MainTitle,
    MainSelectionMenu,
    PlaySelectionMenu,
    OnePlayerPartyStart
};

class MainMenu
{
    //friend class Process;
    private:
        void SCREEN_TOP();
        void SCREEN_BOTTOM();

        void LoadAssetsMainSelectionMenu();
        void LoadAssetsPlaySelectionMenu();
        void LoadAssetsOnePlayerPartyStart();

        void UnloadAssetsMainSelectionMenu();
        void UnloadAssetsPlaySelectionMenu();
        void UnloadAssetsOnePlayerPartyStart();

        void ProcessLogicMainTitle();
        void ProcessLogicMainSelectionMenu();
        void ProcessLogicPlaySelectionMenu();
        void ProcessLogicOnePlayerPartyStart();
        // Hex background as a hardware 2D BG on each engine instead of a
        // 3D sprite. Drawing order is now decided by BG-layer priority, so
        // the rich-text 3D quads naturally render above without us having
        // to reorder NEA_SpriteDraw calls.
        NEA_Hw2DBG *hexBGtop;
        NEA_Hw2DBG *hexBGbot;
        





        // MainSelectionMenu
        NEA_Material *PlayMat[2];
        NEA_Palette *PlayPal[2];

        NEA_GUIObj *PlayButton;




        // PlaySelectionMenu
        NEA_Material *OnePlayerMat[2];
        NEA_Palette *OnePlayerPal[2];

        NEA_GUIObj *OnePlayerButton;

        NEA_Material *MultiplayerMat[2];
        NEA_Palette *MultiplayerPal[2];

        NEA_GUIObj *MultiplayerButton;
        
                
        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];

        NEA_GUIObj *BackButton;


        // OnePlayerPartyStart
        NEA_Material *EmptyMat;
        NEA_Palette *EmptyPal;

        NEA_GUIObj *EmptyNumberCPUButton;
        NEA_GUIObj *EmptyLevelCPUButton;

        NEA_Material *NextPlayerMat[2];
        NEA_Palette *NextPlayerPal[2];

        NEA_GUIObj *NextPlayerNumberCPUButton;
        NEA_GUIObj *NextPlayerLevelCPUButton;


        NEA_Material *PrevPlayerMat[2];
        NEA_Palette *PrevPlayerPal[2];

        NEA_GUIObj *PrevPlayerNumberCPUButton;
        NEA_GUIObj *PrevPlayerLevelCPUButton;

        NEA_Material *StartGameMat[2];
        NEA_Palette *StartGamePal[2];

        NEA_GUIObj *StartGameButton;
        

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