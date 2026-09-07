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
        
        NEA_Material * EmptyMat = nullptr;
        NEA_Palette * EmptyPal = nullptr;

        NEA_GUIObj * EmptyNumberCPUButton = nullptr;
        NEA_GUIObj * EmptyLevelCPUButton = nullptr;

        NEA_Material * NextPlayerMat[2] = {};
        NEA_Palette * NextPlayerPal[2] = {};

        NEA_GUIObj * NextPlayerNumberCPUButton = nullptr;
        NEA_GUIObj * NextPlayerLevelCPUButton = nullptr;


        NEA_Material * PrevPlayerMat[2] = {};
        NEA_Palette * PrevPlayerPal[2] = {};

        NEA_GUIObj * PrevPlayerNumberCPUButton = nullptr;
        NEA_GUIObj * PrevPlayerLevelCPUButton = nullptr;

        NEA_Material * StartGameMat[2] = {};
        NEA_Palette * StartGamePal[2] = {};

        NEA_GUIObj * StartGameButton = nullptr;

        NEA_Material * BackMat[2] = {};
        NEA_Palette * BackPal[2] = {};

        NEA_GUIObj * BackButton = nullptr;

    public:

        int Get_player_number();
        CPULevel Get_CPULevel();

        void LoadAssetsOnePlayerPartyStart();
        void UnloadAssetsOnePlayerPartyStart();
        std::optional<MainMenuStates> ProcessLogicOnePlayerPartyStart();
        void ActionOnePlayerPartyStart();

};
