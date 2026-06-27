#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

class MultiplayerJoinMenu
{
    private:
        NEA_Material *BackMat[2];
        NEA_Palette *BackPal[2];
        NEA_GUIObj *BackButton;

        NEA_Material *JoinMat[2];
        NEA_Palette *JoinPal[2];
        NEA_GUIObj *JoinButton;

        NEA_Material *PrevMat[2];
        NEA_Palette *PrevPal[2];
        NEA_GUIObj *PrevButton; // move selection up

        NEA_Material *NextMat[2];
        NEA_Palette *NextPal[2];
        NEA_GUIObj *NextButton; // move selection down

        // Scanning lists hosts; Connecting waits for association; WaitingStart
        // waits for the host's start handshake.
        enum class Phase { Scanning, Connecting, WaitingStart };
        Phase phase = Phase::Scanning;

        int selected = 0;
        std::vector<int> apIndices;       // scan indices of joinable Skyjo hosts
        std::vector<std::string> apNames; // display strings, parallel to apIndices

        // Filled from the host's start handshake.
        int seat = 0;
        int playerCount = 0;
        std::vector<std::string> names;

        void RefreshApList();

    public:
        int GetSeat() const { return this->seat; }
        int GetPlayerCount() const { return this->playerCount; }
        const std::vector<std::string>& GetNames() const { return this->names; }

        void LoadAssetsMultiplayerJoinMenu();
        void UnloadAssetsMultiplayerJoinMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerJoinMenu();
        void ActionMultiplayerJoinMenu();

};
