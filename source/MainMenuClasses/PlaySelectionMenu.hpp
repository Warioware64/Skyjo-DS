#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class PlaySelectionMenu
{
    private:
        NEA_Material * OnePlayerMat[2] = {};
        NEA_Palette * OnePlayerPal[2] = {};

        NEA_GUIObj * OnePlayerButton = nullptr;

        NEA_Material * MultiplayerMat[2] = {};
        NEA_Palette * MultiplayerPal[2] = {};

        NEA_GUIObj * MultiplayerButton = nullptr;

        NEA_Material * BackMat[2] = {};
        NEA_Palette * BackPal[2] = {};

        NEA_GUIObj * BackButton = nullptr;

    public:
        void LoadAssetsPlaySelectionMenu();
        void UnloadAssetsPlaySelectionMenu();
        std::optional<MainMenuStates> ProcessLogicPlaySelectionMenu();
        void ActionPlaySelectionMenu();

};
