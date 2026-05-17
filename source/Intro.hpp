#pragma once

#include "globalHeader.hpp"

class Intro
{
    //friend class Process;
    private:
        void SCREEN_TOP();
        void SCREEN_BOTTOM();
        
        NEA_Sprite *bgSprite[2];
        NEA_Material *matBg[2];
        NEA_Palette *palBG[2];
    public:
        Intro();
        ~Intro();

        void LoadAssetsIntro();
        void RenderIntro();
};

extern Intro intro;