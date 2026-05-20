#include "Intro.hpp"

Intro::Intro()
{

}

Intro::~Intro()
{

}

void Intro::SCREEN_TOP()
{
    NEA_2DViewInit();
    // Alpha = 0: the 3D layer clears to transparent so the hardware 2D BG
    // (where the intro artwork lives now) shows through. The brightness
    // fade still works because setBrightness() operates on the final
    // composited output, not just the 3D layer.
    NEA_ClearColorSet(NEA_White, 0, 63);
}

void Intro::SCREEN_BOTTOM()
{
    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);
}

void Intro::LoadAssetsIntro()
{
    this->bgTop = NEA_Hw2DBGCreate(NEA_ENGINE_MAIN, 1,
                                    NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->bgTop, 3);
    NEA_Hw2DBGLoadGRFFAT(this->bgTop, "intro/BottomSCREEN_png.grf", 4);
    NEA_Hw2DBGSetVisible(this->bgTop, true);

    this->bgBot = NEA_Hw2DBGCreate(NEA_ENGINE_SUB, 0,
                                    NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->bgBot, 3);

    NEA_Hw2DBGLoadGRFFAT(this->bgBot, "intro/MainSCREEN_png.grf", 3);
    NEA_Hw2DBGSetVisible(this->bgBot, true);

    DEBUG_PRINT("Intro::LoadAssetsIntro() : Assets loaded");
}

void Intro::RenderIntro()
{
    while(1)
    {
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(0));
        if (this->FrameCounterTrigger == 5)
        {
            this->FrameCounterTrigger = 0;
            if (this->incrementOrDecrement)
                this->brightness++;
            else
                this->brightness--;
            if (this->brightness == 0)
            {
                this->incrementOrDecrement = true;
                this->FrameCounterTrigger = 10;
            }
        }

        if (this->FrameCounterTrigger == 80)
            this->FrameCounterTrigger = 0;

        if (this->FrameCounterTrigger == 180)
            break;

        setBrightness(3, this->brightness);
        // Render the 3D scene every frame. SCREEN_TOP() clears the main
        // engine's 3D layer (BG0) with alpha 0 so it is transparent and the
        // hardware 2D background on BG1 shows through. Without this the 3D
        // layer stays opaque and the top screen is black. The sub engine is
        // pure hardware 2D and needs no per-frame work.
        
        NEA_Process([](){
            intro.SCREEN_TOP();
        });

        this->FrameCounterTrigger++;

        if ( (this->incrementOrDecrement) && (this->brightness == 16) )
        {
            this->incrementOrDecrement = false;
            this->FrameCounterTrigger = 0;
            if (this->secondState)
                this->FrameCounterTrigger = 90;
            this->secondState = true;

            // Reload tile/map/palette in place to swap to the second
            // intro state. Happens once at fade-up apex, hidden by the
            // brightness fade so the swap isn't visible.

            NEA_Hw2DBGLoadGRFFAT(this->bgTop, "intro/BottomSCREEN2_png.grf", 4);
            NEA_Hw2DBGLoadGRFFAT(this->bgBot, "intro/MainSCREEN2_png.grf", 3);
        }
    }
}

void Intro::UnloadAssetsIntro()
{
    NEA_Hw2DBGDelete(this->bgTop);
    NEA_Hw2DBGDelete(this->bgBot);
    DEBUG_PRINT("Intro::UnloadAssetsIntro() : Assets Unloaded");
}

Intro intro;
