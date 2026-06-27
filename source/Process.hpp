#pragma once

#include "ErrorHandler.hpp"
#include "globalHeader.hpp"
#include "Intro.hpp"
#include "MainMenu.hpp"
#include "GameParty.hpp"

class Process
{
    friend GameParty;
    private:
        ClassStates classstates;
        MenusStates menustates;

        int cpu_number_arg;
        CPULevel cpu_level_arg;
        PartyType party_type_arg;
        bool resumeRequested = false;

        // Local-multiplayer launch parameters, filled by the host/join menus.
        int mp_player_count_arg = 0;
        int mp_seat_arg = 0;
        std::vector<std::string> mp_names_arg;
    public:
        Process();
        ~Process();

        const char* fatDevice;
        std::string fatDeviceCPP;
        std::string consoleUserName;
        GameSettings gamesettings;
        
        void CallSaveSettings();
        void CallInitializationOnePlayerParty(int cpu_number, CPULevel cpu_level);
        void CallResumeOnePlayerParty();

        // Launch a local-multiplayer game (host runs the simulation; clients
        // mirror it). Called by the host/join lobby menus.
        void CallInitializationMultiplayerHost(int player_count);
        void CallInitializationMultiplayerClient(int seat, int player_count,
                                                 const std::vector<std::string>& names);

        void ProcessInit();
        void ProcessGame();

};

extern Process process;