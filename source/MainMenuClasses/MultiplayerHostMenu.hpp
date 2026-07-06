#pragma once

#include "../globalHeader.hpp"
#include "../Net/NetLink.hpp"
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

        // CPU count / level pickers (ported from OnePlayerPartyStart): let the host
        // add computer opponents on top of the connected human clients.
        NEA_Material *EmptyMat;
        NEA_Palette *EmptyPal;
        NEA_Material *PrevMat[2];
        NEA_Palette *PrevPal[2];
        NEA_Material *NextMat[2];
        NEA_Palette *NextPal[2];

        NEA_GUIObj *PrevCpuCountButton;
        NEA_GUIObj *NextCpuCountButton;
        NEA_GUIObj *EmptyCpuCountButton;
        NEA_GUIObj *PrevCpuLevelButton;
        NEA_GUIObj *NextCpuLevelButton;
        NEA_GUIObj *EmptyCpuLevelButton;

        int cpuCount = 0;
        CPULevel cpuLevel = CPULevel::Easy;

        // Console names announced by connected clients (indexed by AID). Filled as
        // Hello frames arrive in the lobby and folded into the roster at start.
        std::array<std::string, NetLink::kMaxClients + 1> clientNames;

        // Lobby waits for clients; Starting resends the game-start handshake for
        // a short window so a just-connected client can't miss it.
        enum class Phase { Lobby, Starting };
        Phase phase = Phase::Lobby;
        int startFrame = 0;

        int playerCount = 0;            // host + clients + CPUs, set at start
        int humanCount = 0;            // host + connected clients, set at start
        std::vector<std::string> names; // full roster (humans then CPUs)

    public:
        // Values chosen for the launched game (read by MainMenu on start).
        int GetPlayerCount() const { return this->playerCount; }
        int GetHumanCount() const { return this->humanCount; }
        CPULevel GetCpuLevel() const { return this->cpuLevel; }
        const std::vector<std::string>& GetNames() const { return this->names; }

        void LoadAssetsMultiplayerHostMenu();
        void UnloadAssetsMultiplayerHostMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerHostMenu();
        void ActionMultiplayerHostMenu();

};
