#include "Intro.hpp"
#include "NeaDelete.hpp"
#include "AssetLoader.hpp"
#include "ErrorHandler.hpp"

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
    // NEA_Hw2DBGCreate returns NULL if the layer is still claimed, and the
    // NEA_Hw2DBGSetPriority on the next line would then write through it --
    // which on the ARM9 lands in ITCM instead of faulting.
    this->bgTop = NEA_Hw2DBGCreate(NEA_ENGINE_MAIN, 1,
                                    NEA_HW2D_BG_TILED_8BPP, 256, 256);
    if (this->bgTop == nullptr)
    {
        error.errorReason.assign("Couldn't create the intro main BG");
        std::terminate();
    }
    NEA_Hw2DBGSetPriority(this->bgTop, 3);
    // Freshly created layers are shown by default, so keep them hidden until
    // their artwork has landed.
    NEA_Hw2DBGSetVisible(this->bgTop, false);

    this->bgBot = NEA_Hw2DBGCreate(NEA_ENGINE_SUB, 0,
                                    NEA_HW2D_BG_TILED_8BPP, 256, 256);
    if (this->bgBot == nullptr)
    {
        error.errorReason.assign("Couldn't create the intro sub BG");
        std::terminate();
    }
    NEA_Hw2DBGSetPriority(this->bgBot, 3);
    NEA_Hw2DBGSetVisible(this->bgBot, false);

    // Both backgrounds are read in the background and uploaded during the
    // vertical blank. The palette_slot argument is ignored for 8bpp backgrounds
    // -- each engine's BG palette comes wholesale from its own background.
    AsyncAssetBatch assets;
    assets.QueueBGGRF(this->bgTop, "intro/BottomSCREEN_png.grf", 0);
    assets.QueueBGGRF(this->bgBot, "intro/MainSCREEN_png.grf", 0);
    assets.Wait("Loading...");

    ClearBackdropToWhite();
    NEA_Hw2DBGSetVisible(this->bgTop, true);
    NEA_Hw2DBGSetVisible(this->bgBot, true);

    DEBUG_PRINT("Intro::LoadAssetsIntro() : Assets loaded");
}

void Intro::RenderIntro()
{
    while(1)
    {
        // NEA_UPDATE_ASSETS finalizes the second-state background swap queued
        // below; NEA_UPDATE_HW2D is what actually pushes OAM to the hardware,
        // so anything a previous screen hid really disappears.
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_ASSETS |
                                                     NEA_UPDATE_HW2D));
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

            // Queued, not awaited: the fade is 16 frames of white and the
            // reads land well inside it, so there is nothing to wait for. The
            // NEA_UPDATE_ASSETS above runs the uploads.
            this->swap.QueueBGGRF(this->bgTop, "intro/BottomSCREEN2_png.grf", 0);
            this->swap.QueueBGGRF(this->bgBot, "intro/MainSCREEN2_png.grf", 0);
        }
    }
}

void Intro::UnloadAssetsIntro()
{
    // Releases the second-state swap handles too, in case the intro was cut
    // short while they were still in flight.
    this->swap.Wait("Loading...");

    DeleteBG(this->bgTop);
    DeleteBG(this->bgBot);
    this->bgTop = nullptr;
    this->bgBot = nullptr;
    DEBUG_PRINT("Intro::UnloadAssetsIntro() : Assets Unloaded");
}

Intro intro;
