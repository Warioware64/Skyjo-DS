#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class MultiplayerFirstMenu
{
    private:
        NEA_Material * BackMat[2] = {};
        NEA_Palette * BackPal[2] = {};

        NEA_GUIObj * BackButton = nullptr;

        NEA_Material * HostMat[2] = {};
        NEA_Palette * HostPal[2] = {};

        NEA_GUIObj * HostButton = nullptr;        

        NEA_Material * JoinMat[2] = {};
        NEA_Palette * JoinPal[2] = {};

        NEA_GUIObj * JoinButton = nullptr;

        NEA_Material * DlPlayMat[2] = {};
        NEA_Palette * DlPlayPal[2] = {};

        NEA_GUIObj * DlPlayButton = nullptr;
    public:
        void LoadAssetsMultiplayerFirstMenu();
        void UnloadAssetsMultiplayerFirstMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerFirstMenu();
        void ActionMultiplayerFirstMenu();    

};