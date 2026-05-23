#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class OnePlayerPartyStart
{
    private:

        int old_player_number = 0;
        int player_number = 2;

        std::optional<CPULevel> old_cpu_level = std::nullopt;
        CPULevel cpu_level = CPULevel::Easy;
        
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
        void ActionOnePlayerPartyStart();

};
