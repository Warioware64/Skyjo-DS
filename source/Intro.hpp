#pragma once

#include "globalHeader.hpp"
#include "DebugPrint.hpp"

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

        // Backgrounds are now hardware 2D BGs (one per engine). The intro's
        // mid-sequence change of artwork is implemented by reloading the
        // tile/map/palette from NitroFS at the transition point, instead of
        // swapping materials on a 3D sprite.
        NEA_Hw2DBG *bgTop;
        NEA_Hw2DBG *bgBot;
    public:
        Intro();
        ~Intro();

        void LoadAssetsIntro();
        void RenderIntro();
        void UnloadAssetsIntro();
};

extern Intro intro;