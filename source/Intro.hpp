#pragma once

#include "globalHeader.hpp"

class Intro
{
    friend class Process;
    private:

    public:
        Intro();
        ~Intro();

        void LoadAssetsIntro();
};

extern Intro intro;