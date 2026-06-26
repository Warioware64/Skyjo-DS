#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class SettingsMenu
{
    private:
        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];

        NEA_GUIObj *BackButton;

        NEA_Material *StartMat[2];
        NEA_Palette *StartPal[2];

        NEA_GUIObj *StartButton;        

    public:
        void LoadAssetsSettingsMenu();
        void UnloadAssetsSettingsMenu();
        std::optional<MainMenuStates> ProcessLogicSettingsMenu();
        void ActionSettingsMenu();    

};