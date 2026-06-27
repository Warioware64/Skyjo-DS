#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"


class SettingsMenu
{
    private:
        float percentageMusic;
        float percentageSound;

        uint32_t oldpercentageMusic;
        uint32_t oldpercentageSound;



        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];

        NEA_GUIObj *BackButton;

        NEA_Material *StartMat[2];
        NEA_Palette *StartPal[2];

        NEA_GUIObj *StartButton;        


        NEA_Material *EmptyMat;
        NEA_Palette *EmptyPal;

        NEA_GUIObj *EmptyMusicButton;
        NEA_GUIObj *EmptySoundButton;

        NEA_Material *NextPlayerMat[2];
        NEA_Palette *NextPlayerPal[2];

        int incrementMusic = 0;
        int incrementSound = 0;

        NEA_GUIObj *NextPlayerMusicButton;
        NEA_GUIObj *NextPlayerSoundButton;


        NEA_Material *PrevPlayerMat[2];
        NEA_Palette *PrevPlayerPal[2];

        int decrementMusic = 0;
        int decrementSound = 0;

        NEA_GUIObj *PrevPlayerMusicButton;
        NEA_GUIObj *PrevPlayerSoundButton;

    public:
        GameSettings oldSettings;
        void LoadAssetsSettingsMenu();
        void UnloadAssetsSettingsMenu();
        std::optional<MainMenuStates> ProcessLogicSettingsMenu();
        void ActionSettingsMenu();    

};