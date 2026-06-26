#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class MainSelectionMenu
{
    private:
        NEA_Material *PlayMat[2];
        NEA_Palette *PlayPal[2];

        NEA_GUIObj *PlayButton;

        NEA_Material *SettingsMat[2];
        NEA_Palette *SettingsPal[2];

        NEA_GUIObj *SettingsButton;

        NEA_Material *ResumeMat[2];
        NEA_Palette *ResumePal[2];

        NEA_GUIObj *ResumeButton;
    public:
        bool resumableParty = false;


        void LoadAssetsMainSelectionMenu();
        void UnloadAssetsMainSelectionMenu();
        std::optional<MainMenuStates> ProcessLogicMainSelectionMenu();
        void ActionMainSelectionMenu();

};
