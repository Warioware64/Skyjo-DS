#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class MultiplayerHostMenu
{
    private:
        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];

        NEA_GUIObj *BackButton;

        NEA_Material *StartMat[2];
        NEA_Palette *StartPal[2];

        NEA_GUIObj *StartButton;        

    public:
        void LoadAssetsMultiplayerHostMenu();
        void UnloadAssetsMultiplayerHostMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerHostMenu();
        void ActionMultiplayerHostMenu();    

};