#include "MainMenu.hpp"
#include "MainMenuClasses/MainMenuStates.hpp"
#include "Process.hpp"
#include "globalHeader.hpp"
#include <NEACamera.h>
#include <NEAPalette.h>
#include <NEAParticle.h>
#include <NEATexture.h>


MainMenu::MainMenu()
{

}

MainMenu::~MainMenu()
{

}

void MainMenu::SCREEN_TOP()
{
    int angle = (this->emitFrame * 1) & 0x1FF;
    int32_t cx = mulf32(floattof32(3.5f), cosLerp(angle << 6));
    int32_t cz = mulf32(floattof32(3.5f), sinLerp(angle << 6));
    NEA_CameraSetI(this->emitCam, cx, floattof32(1.5f), cz,
                              0, floattof32(0.4f), 0,
                              0, floattof32(1.0f), 0);
    NEA_CameraUse(this->emitCam);
    NEA_ParticleEmitterDraw(this->hexEmit);

    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);

    // The hex background is on a hardware 2D BG layer — the GPU renders it
    // every scanline automatically. We only submit 3D content here (text).
    // BG0 (3D) sits in front of BG1 (the 2D hex layer) per the priorities
    // set in LoadAssetsMainMenu, so text appears above the pattern with no
    // submission-order tricks.

    switch (this->mainmenustates){
        case MainMenuStates::MainTitle:
        {
            if ((this->showOrNotTouchScreenText / 30) % 2 == 0)
            {
                NEA_RichTextRender3D(0, "Touch screen!", 70, 90);
            }
            this->showOrNotTouchScreenText++;

            break;
        }

        case MainMenuStates::MainSelectionMenu:
        {
            this->mainSelec.ActionMainSelectionMenu();
            break;
        }

        case MainMenuStates::PlaySelectionMenu:
        {
            this->playSelec.ActionPlaySelectionMenu();
            break;
        }

        case MainMenuStates::OnePlayerPartyStart:
        {
            this->onePlayerParty.ActionOnePlayerPartyStart();
            break;
        }

        case MainMenuStates::TransitionToPlayOnePlayer:
        {
            break;
        }
    }



}

void MainMenu::SCREEN_BOTTOM()
{
    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);
    // Hardware 2D BG renders itself; nothing to submit here yet.
}

void MainMenu::LoadAssetsMainMenu()
{
    this->brightness = 16;
    this->frameTrigger = 0;
    this->triggerPlayPartyOnePlayer = false;
    this->fadePhase = FadePhase::FadingIn;
    this->fadeStepInterval = 5;
    this->pendingNextState.reset();

    this->emitCam = NEA_CameraCreate();
    NEA_ParticleSystemReset(0);
    NEA_ParticleSystemSetCamera(this->emitCam);

    this->hexParMat = NEA_MaterialCreate();
    this->hexParPal = NEA_PaletteCreate();

    
    this->hexEmit = NEA_ParticleEmitterCreate();

    NEA_MaterialSetName(this->hexParMat, "hexPart");

    NEA_MaterialTexLoadGRF(this->hexParMat, this->hexParPal,
                            NEA_TEXGEN_TEXCOORD, "mainmenu/hex/hexParticle_png.grf");

    NEA_ParticleEmitterLoadFAT(this->hexEmit, "mainmenu/hex/hexNPE.npe");
    NEA_ParticleEmitterSetPosition(this->hexEmit, 0, floattof32(-2.0), 0);
    NEA_ParticleEmitterPlay(this->hexEmit);

    // Rich-text font (3D quad path) for the menu labels.
    NEA_RichTextResetSystem();
    NEA_RichTextInit(0);
    NEA_RichTextMetadataLoadFAT(0, "mainmenu/font/DejaVuSans-Bold.fnt");
    NEA_RichTextMaterialLoadGRF(0, "mainmenu/font/DejaVuSans-Bold_0_png.grf");

    // Hex background as a hardware 2D BG on each engine.
    // Layer 1 on main (layer 0 is reserved for 3D output); priority 3 puts
    // it behind the 3D layer so text submitted as a 3D quad renders above.
    this->hexBGtop = NEA_Hw2DBGCreate(NEA_ENGINE_MAIN, 1,
                                       NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->hexBGtop, 3);

    NEA_Hw2DBGLoadGRFFAT(this->hexBGtop, "mainmenu/hex_background2_png.grf", 0);
    NEA_Hw2DBGSetVisible(this->hexBGtop, true);

    this->hexBGbot = NEA_Hw2DBGCreate(NEA_ENGINE_SUB, 0,
                                       NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->hexBGbot, 3);
    NEA_Hw2DBGLoadGRFFAT(this->hexBGbot, "mainmenu/hex_background_png.grf", 1);

    NEA_Hw2DBGSetVisible(this->hexBGbot, true);

    // Per-screen materials/palettes are created in each sub-menu's own
    // LoadAssets* and freed in the matching UnloadAssets* — creating them
    // here once would leave dangling handles after the first Unload.
}

void MainMenu::ProcessLogicMainTitle()
{
    if (this->keys & KEY_TOUCH)
    {
        this->StartTransitionTo(MainMenuStates::MainSelectionMenu);
    }
}

void MainMenu::StartTransitionTo(MainMenuStates next)
{
    // Defer the actual asset swap to the fade apex (when brightness == 16,
    // i.e. fully white). Step interval 2 is quicker than the 5-frame step
    // used by the intro / initial boot fade.
    this->pendingNextState = next;
    this->fadePhase = FadePhase::FadingOut;
    this->fadeStepInterval = 2;
    this->frameTrigger = 0;
}

void MainMenu::RenderMainMenu()
{
    while(1)
    {
        this->OLDmainmenustates = this->mainmenustates;

        if (this->canTouchDetect)
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_GUI | NEA_UPDATE_PARTICLES));
        else
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_PARTICLES));

        scanKeys();
        this->keys = keysDown();


        // Only run menu logic when no fade is in progress — clicks during a
        // transition would either be lost (assets are mid-swap) or queue up
        // a second transition before the first one finishes.
        if (this->fadePhase == FadePhase::None)
        {
            switch (this->mainmenustates){
                case MainMenuStates::MainTitle:
                {
                    this->ProcessLogicMainTitle();
                    break;
                }

                case MainMenuStates::MainSelectionMenu:
                {
                    if (auto next = this->mainSelec.ProcessLogicMainSelectionMenu())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::PlaySelectionMenu:
                {
                    if (auto next = this->playSelec.ProcessLogicPlaySelectionMenu())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::OnePlayerPartyStart:
                {
                    if (auto next = this->onePlayerParty.ProcessLogicOnePlayerPartyStart())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::TransitionToPlayOnePlayer:
                {
                    break;
                }
            }
        }

        // Fade tick. fadeStepInterval is 5 for the initial boot fade-in and
        // 2 for menu-to-menu transitions, giving menu switches a quicker
        // white fade than the intro / first MainMenu fade.
        if (this->fadePhase != FadePhase::None)
        {
            this->frameTrigger++;
            if (this->frameTrigger >= this->fadeStepInterval)
            {
                this->frameTrigger = 0;
                if (this->fadePhase == FadePhase::FadingOut)
                {
                    this->brightness++;
                    if (this->brightness >= 16)
                    {
                        this->brightness = 16;
                        // Apex of the fade: screen is fully white, swap
                        // assets out of sight.
                        switch (this->mainmenustates)
                        {
                            case MainMenuStates::MainSelectionMenu:
                                this->mainSelec.UnloadAssetsMainSelectionMenu(); break;
                            case MainMenuStates::PlaySelectionMenu:
                                this->playSelec.UnloadAssetsPlaySelectionMenu(); break;
                            case MainMenuStates::OnePlayerPartyStart:
                                this->onePlayerParty.UnloadAssetsOnePlayerPartyStart(); break;
                            case MainMenuStates::TransitionToPlayOnePlayer:
                                break;
                            default: break;
                        }
                        if (this->pendingNextState)
                        {
                            this->mainmenustates = *this->pendingNextState;
                            this->pendingNextState.reset();
                        }
                        switch (this->mainmenustates)
                        {
                            case MainMenuStates::MainSelectionMenu:
                                this->mainSelec.LoadAssetsMainSelectionMenu(); break;
                            case MainMenuStates::PlaySelectionMenu:
                                this->playSelec.LoadAssetsPlaySelectionMenu(); break;
                            case MainMenuStates::OnePlayerPartyStart:
                                this->onePlayerParty.LoadAssetsOnePlayerPartyStart(); break;
                            case MainMenuStates::TransitionToPlayOnePlayer:
                                this->triggerPlayPartyOnePlayer = true;
                            default: break;
                        }
                        this->fadePhase = FadePhase::FadingIn;
                    }
                }
                else // FadingIn
                {
                    this->brightness--;
                    if (this->brightness <= 0)
                    {
                        this->brightness = 0;
                        this->fadePhase = FadePhase::None;
                    }
                }
            }
        }

        setBrightness(3, this->brightness);
        // Render the 3D scene every frame. SCREEN_TOP() clears the main
        // engine's 3D layer (BG0) with alpha 0 so the hardware hex
        // background on BG1 shows through, then submits the menu text on
        // top. The sub engine is pure hardware 2D — nothing to submit.
        NEA_Process([](){
            mainmenu.SCREEN_TOP();
        });

        if (this->mainmenustates != this->OLDmainmenustates)
        {
            this->triggerCanTouchDetect = true;
            this->frameTouchDetect = 0;
        }

        if (this->triggerCanTouchDetect)
        {
            if (!this->canTouchDetect)
                this->frameTouchDetect++;

            if (this->frameTouchDetect == 20)
            {
                this->canTouchDetect = true;
                this->triggerCanTouchDetect = false;
                this->frameTouchDetect = 0;
            }

        }

        if (this->triggerPlayPartyOnePlayer)
            break;
    }

    if (this->triggerPlayPartyOnePlayer)
    {
        process.CallInitializationOnePlayerParty(this->onePlayerParty.Get_player_number(),
                                                 this->onePlayerParty.Get_CPULevel());
    }
}
MainMenu mainmenu;
