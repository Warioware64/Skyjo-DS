#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class PlaySelectionMenu
{
    private:
        NEA_Material *OnePlayerMat[2];
        NEA_Palette *OnePlayerPal[2];

        NEA_GUIObj *OnePlayerButton;

        NEA_Material *MultiplayerMat[2];
        NEA_Palette *MultiplayerPal[2];

        NEA_GUIObj *MultiplayerButton;

        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];

        NEA_GUIObj *BackButton;

    public:
        void LoadAssetsPlaySelectionMenu();
        void UnloadAssetsPlaySelectionMenu();
        std::optional<MainMenuStates> ProcessLogicPlaySelectionMenu();

};
