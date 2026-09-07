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



        NEA_Material * BackMat[2] = {};
        NEA_Palette * BackPal[2] = {};

        NEA_GUIObj * BackButton = nullptr;

        NEA_Material * StartMat[2] = {};
        NEA_Palette * StartPal[2] = {};

        NEA_GUIObj * StartButton = nullptr;        


        NEA_Material * EmptyMat = nullptr;
        NEA_Palette * EmptyPal = nullptr;

        NEA_GUIObj * EmptyMusicButton = nullptr;
        NEA_GUIObj * EmptySoundButton = nullptr;

        NEA_Material * NextPlayerMat[2] = {};
        NEA_Palette * NextPlayerPal[2] = {};

        int incrementMusic = 0;
        int incrementSound = 0;

        NEA_GUIObj * NextPlayerMusicButton = nullptr;
        NEA_GUIObj * NextPlayerSoundButton = nullptr;


        NEA_Material * PrevPlayerMat[2] = {};
        NEA_Palette * PrevPlayerPal[2] = {};

        int decrementMusic = 0;
        int decrementSound = 0;

        NEA_GUIObj * PrevPlayerMusicButton = nullptr;
        NEA_GUIObj * PrevPlayerSoundButton = nullptr;

    public:
        GameSettings oldSettings;
        void LoadAssetsSettingsMenu();
        void UnloadAssetsSettingsMenu();
        std::optional<MainMenuStates> ProcessLogicSettingsMenu();
        void ActionSettingsMenu();    

};