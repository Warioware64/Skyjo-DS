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
    public:
        Process();
        ~Process();

        std::string consoleUserName;

        void CallInitializationOnePlayerParty(int cpu_number, CPULevel cpu_level);

        void ProcessInit();
        void ProcessGame();

};

extern Process process;