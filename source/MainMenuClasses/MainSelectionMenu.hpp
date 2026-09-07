#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class MainSelectionMenu
{
    private:
        NEA_Material * PlayMat[2] = {};
        NEA_Palette * PlayPal[2] = {};

        NEA_GUIObj * PlayButton = nullptr;

        NEA_Material * SettingsMat[2] = {};
        NEA_Palette * SettingsPal[2] = {};

        NEA_GUIObj * SettingsButton = nullptr;

        NEA_Material * ResumeMat[2] = {};
        NEA_Palette * ResumePal[2] = {};

        NEA_GUIObj * ResumeButton = nullptr;
    public:
        bool resumableParty = false;
        // Set when the Resume button is clicked; consumed by MainMenu at launch
        // to load the saved party instead of starting a new one.
        bool resumeSelected = false;


        void LoadAssetsMainSelectionMenu();
        void UnloadAssetsMainSelectionMenu();
        std::optional<MainMenuStates> ProcessLogicMainSelectionMenu();
        void ActionMainSelectionMenu();

};
