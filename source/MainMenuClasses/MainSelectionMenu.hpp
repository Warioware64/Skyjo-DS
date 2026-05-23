#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class MainSelectionMenu
{
    private:
        NEA_Material *PlayMat[2];
        NEA_Palette *PlayPal[2];

        NEA_GUIObj *PlayButton;

    public:
        void LoadAssetsMainSelectionMenu();
        void UnloadAssetsMainSelectionMenu();
        std::optional<MainMenuStates> ProcessLogicMainSelectionMenu();
        void ActionMainSelectionMenu();

};
