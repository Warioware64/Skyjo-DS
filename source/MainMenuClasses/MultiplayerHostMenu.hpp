#pragma once

#include "../globalHeader.hpp"
#include "../Net/NetLink.hpp"
#include "MainMenuStates.hpp"

class MultiplayerHostMenu
{
    private:
        NEA_Material * BackMat[2] = {};
        NEA_Palette * BackPal[2] = {};

        NEA_GUIObj * BackButton = nullptr;

        NEA_Material * StartMat[2] = {};
        NEA_Palette * StartPal[2] = {};

        NEA_GUIObj * StartButton = nullptr;

        // CPU count / level pickers (ported from OnePlayerPartyStart): let the host
        // add computer opponents on top of the connected human clients.
        NEA_Material * EmptyMat = nullptr;
        NEA_Palette * EmptyPal = nullptr;
        NEA_Material * PrevMat[2] = {};
        NEA_Palette * PrevPal[2] = {};
        NEA_Material * NextMat[2] = {};
        NEA_Palette * NextPal[2] = {};

        NEA_GUIObj * PrevCpuCountButton = nullptr;
        NEA_GUIObj * NextCpuCountButton = nullptr;
        NEA_GUIObj * EmptyCpuCountButton = nullptr;
        NEA_GUIObj * PrevCpuLevelButton = nullptr;
        NEA_GUIObj * NextCpuLevelButton = nullptr;
        NEA_GUIObj * EmptyCpuLevelButton = nullptr;

        int cpuCount = 0;
        CPULevel cpuLevel = CPULevel::Easy;

        // Console names announced by connected clients (indexed by AID). Filled as
        // Hello frames arrive in the lobby and folded into the roster at start.
        std::array<std::string, NetLink::kMaxClients + 1> clientNames;


        // WaitingGuests holds the screen while consoles that were just handed the
        // game over Download Play reboot and find their way back; Lobby waits for
        // clients and lets the host pick CPUs; Starting resends the game-start
        // handshake for a short window so a just-connected client can't miss it.
        enum class Phase { WaitingGuests, Lobby, Starting };
        Phase phase = Phase::Lobby;
        int startFrame = 0;

        // How many Download Play guests are on their way back, and how long we
        // have been waiting. Set through ExpectDownloadPlayGuests() before this
        // screen loads; zero for an ordinary cart-to-cart host, which skips the
        // waiting phase entirely.
        int expectedGuests = 0;
        int pendingExpectedGuests = 0;
        int waitFrames = 0;
        // Frames since the last guest turned up. The full wait is sized for a
        // console that is still rebooting; once they have started arriving, a
        // much shorter quiet spell is enough to conclude that the rest of the
        // expected count was never real.
        int settleFrames = 0;

        void CreateLobbyButtons();

        int playerCount = 0;            // host + clients + CPUs, set at start
        int humanCount = 0;            // host + connected clients, set at start
        std::vector<std::string> names; // full roster (humans then CPUs)

    public:
        // Values chosen for the launched game (read by MainMenu on start).
        int GetPlayerCount() const { return this->playerCount; }
        int GetHumanCount() const { return this->humanCount; }
        CPULevel GetCpuLevel() const { return this->cpuLevel; }
        const std::vector<std::string>& GetNames() const { return this->names; }

        // Tells the next load of this screen that `count` consoles were just
        // booted over Download Play and should be waited for. Stored as a pending
        // value that LoadAssets consumes and clears, so it cannot leak into a
        // later cart-to-cart host session.
        void ExpectDownloadPlayGuests(int count) { this->pendingExpectedGuests = count; }

        void LoadAssetsMultiplayerHostMenu();
        void UnloadAssetsMultiplayerHostMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerHostMenu();
        void ActionMultiplayerHostMenu();

};
