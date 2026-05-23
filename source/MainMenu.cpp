#include "MainMenu.hpp"


MainMenu::MainMenu()
{

}

MainMenu::~MainMenu()
{

}

void MainMenu::SCREEN_TOP()
{
    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);

    // The hex background is on a hardware 2D BG layer — the GPU renders it
    // every scanline automatically. We only submit 3D content here (text).
    // BG0 (3D) sits in front of BG1 (the 2D hex layer) per the priorities
    // set in LoadAssetsMainMenu, so text appears above the pattern with no
    // submission-order tricks.
    if (this->mainmenustates == MainMenuStates::MainTitle)
    {
        if ((this->showOrNotTouchScreenText / 30) % 2 == 0)
        {
            NEA_RichTextRender3D(0, "Touch screen!", 70, 90);
        }
        this->showOrNotTouchScreenText++;
    }
    else {
        NEA_GUIDraw();
    }
    if (this->mainmenustates == MainMenuStates::OnePlayerPartyStart)
    {
        NEA_RichTextRender3D(0, "Touch screen!", 70, 70);
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
    this->frameDoCount = true;

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
    if (this->brightness == 0)
    {
        if (this->keys & KEY_TOUCH)
        {
            this->mainmenustates = MainMenuStates::MainSelectionMenu;
            this->mainSelec.LoadAssetsMainSelectionMenu();
        }
    }
}

void MainMenu::RenderMainMenu()
{
    while(1)
    {
        this->OLDmainmenustates = this->mainmenustates;

        if (this->canTouchDetect)
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_GUI));
        else
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(0));

        scanKeys();
        this->keys = keysDown();


        switch (this->mainmenustates){
            case MainMenuStates::MainTitle:
            {
                this->ProcessLogicMainTitle();
                break;
            }

            case MainMenuStates::MainSelectionMenu:
            {
                if (auto next = this->mainSelec.ProcessLogicMainSelectionMenu())
                {
                    this->mainSelec.UnloadAssetsMainSelectionMenu();
                    this->mainmenustates = *next;
                    switch (*next)
                    {
                        case MainMenuStates::PlaySelectionMenu:
                            this->playSelec.LoadAssetsPlaySelectionMenu(); break;
                        case MainMenuStates::OnePlayerPartyStart:
                            this->onePlayerParty.LoadAssetsOnePlayerPartyStart(); break;
                        case MainMenuStates::MainSelectionMenu:
                            this->mainSelec.LoadAssetsMainSelectionMenu(); break;
                        default: break;
                    }
                }
                break;
            }

            case MainMenuStates::PlaySelectionMenu:
            {
                if (auto next = this->playSelec.ProcessLogicPlaySelectionMenu())
                {
                    this->playSelec.UnloadAssetsPlaySelectionMenu();
                    this->mainmenustates = *next;
                    switch (*next)
                    {
                        case MainMenuStates::MainSelectionMenu:
                            this->mainSelec.LoadAssetsMainSelectionMenu(); break;
                        case MainMenuStates::OnePlayerPartyStart:
                            this->onePlayerParty.LoadAssetsOnePlayerPartyStart(); break;
                        case MainMenuStates::PlaySelectionMenu:
                            this->playSelec.LoadAssetsPlaySelectionMenu(); break;
                        default: break;
                    }
                }
                break;
            }

            case MainMenuStates::OnePlayerPartyStart:
            {
                if (auto next = this->onePlayerParty.ProcessLogicOnePlayerPartyStart())
                {
                    this->onePlayerParty.UnloadAssetsOnePlayerPartyStart();
                    this->mainmenustates = *next;
                    switch (*next)
                    {
                        case MainMenuStates::MainSelectionMenu:
                            this->mainSelec.LoadAssetsMainSelectionMenu(); break;
                        case MainMenuStates::PlaySelectionMenu:
                            this->playSelec.LoadAssetsPlaySelectionMenu(); break;
                        case MainMenuStates::OnePlayerPartyStart:
                            this->onePlayerParty.LoadAssetsOnePlayerPartyStart(); break;
                        default: break;
                    }
                }
                break;

            }

        }

        if (this->frameTrigger == 5)
        {
            this->frameTrigger = 0;
            this->brightness--;
            if (this->brightness == 0)
                this->frameDoCount = false;
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
        if (this->frameDoCount)
            this->frameTrigger++;

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
    }
}
MainMenu mainmenu;
