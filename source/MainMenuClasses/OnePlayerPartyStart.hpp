#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class OnePlayerPartyStart
{
    private:
        NEA_Material *EmptyMat;
        NEA_Palette *EmptyPal;

        NEA_GUIObj *EmptyNumberCPUButton;
        NEA_GUIObj *EmptyLevelCPUButton;

        NEA_Material *NextPlayerMat[2];
        NEA_Palette *NextPlayerPal[2];

        NEA_GUIObj *NextPlayerNumberCPUButton;
        NEA_GUIObj *NextPlayerLevelCPUButton;


        NEA_Material *PrevPlayerMat[2];
        NEA_Palette *PrevPlayerPal[2];

        NEA_GUIObj *PrevPlayerNumberCPUButton;
        NEA_GUIObj *PrevPlayerLevelCPUButton;

        NEA_Material *StartGameMat[2];
        NEA_Palette *StartGamePal[2];

        NEA_GUIObj *StartGameButton;

        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];

        NEA_GUIObj *BackButton;

    public:
        void LoadAssetsOnePlayerPartyStart();
        void UnloadAssetsOnePlayerPartyStart();
        std::optional<MainMenuStates> ProcessLogicOnePlayerPartyStart();

};
