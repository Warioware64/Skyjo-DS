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

        // Lobby waits for clients; Starting resends the game-start handshake for
        // a short window so a just-connected client can't miss it.
        enum class Phase { Lobby, Starting };
        Phase phase = Phase::Lobby;
        int startFrame = 0;

        int playerCount = 0;            // host + connected clients, set at start
        std::vector<std::string> names; // roster sent to clients

    public:
        // Player count chosen for the launched game (read by MainMenu on start).
        int GetPlayerCount() const { return this->playerCount; }

        void LoadAssetsMultiplayerHostMenu();
        void UnloadAssetsMultiplayerHostMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerHostMenu();
        void ActionMultiplayerHostMenu();

};
