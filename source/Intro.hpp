#pragma once

#include "globalHeader.hpp"

class Intro
{
    //friend class Process;
    private:
        void SCREEN_TOP();
        void SCREEN_BOTTOM();
        
        bool secondState = false;
        bool incrementOrDecrement = false;
        int FrameCounterTrigger = 0;
        int brightness = 16;
        NEA_Sprite *bgSprite[2];
        NEA_Material *matBg[4];
        NEA_Palette *palBG[4];
    public:
        Intro();
        ~Intro();

        void LoadAssetsIntro();
        void RenderIntro();
        void UnloadAssetsIntro();
};

extern Intro intro;