#pragma once

#include "ErrorHandler.hpp"
#include "globalHeader.hpp"
#include "Intro.hpp"
#include "MainMenu.hpp"

class Process
{
    private:
        ClassStates classstates;
        MenusStates menustates;
    public:
        Process();
        ~Process();

        void ProcessInit();
        void ProcessGame();

};

extern Process process;