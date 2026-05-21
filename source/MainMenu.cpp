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

}

void MainMenu::SCREEN_BOTTOM()
{
    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);
    // Hardware 2D BG renders itself; nothing to submit here yet.
}

void MainMenu::LoadAssetsMainSelectionMenu()
{
    // Materials/palettes are created here and destroyed in the matching
    // UnloadAssetsMainSelectionMenu — keep Create/Delete paired per screen.
    this->PlayMat[0] = NEA_MaterialCreate();
    this->PlayMat[1] = NEA_MaterialCreate();
    this->PlayPal[0] = NEA_PaletteCreate();
    this->PlayPal[1] = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->PlayMat[0], this->PlayPal[0], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/playButton_png.grf");
    NEA_MaterialTexLoadGRF(this->PlayMat[1], this->PlayPal[1], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/playButtonPressed_png.grf");

    this->PlayButton = NEA_GUIButtonCreate(60, 50,
                                            60 + 128, 50 + 32);
    NEA_GUIButtonConfig(this->PlayButton,
                        this->PlayMat[0], NEA_White, 31,
                        this->PlayMat[1], NEA_White, 31);
}

void MainMenu::LoadAssetsPlaySelectionMenu()
{
    // Materials/palettes are created here and destroyed in the matching
    // UnloadAssetsPlaySelectionMenu — keep Create/Delete paired per screen.
    this->OnePlayerMat[0] = NEA_MaterialCreate();
    this->OnePlayerMat[1] = NEA_MaterialCreate();
    this->OnePlayerPal[0] = NEA_PaletteCreate();
    this->OnePlayerPal[1] = NEA_PaletteCreate();

    this->MultiplayerMat[0] = NEA_MaterialCreate();
    this->MultiplayerMat[1] = NEA_MaterialCreate();
    this->MultiplayerPal[0] = NEA_PaletteCreate();
    this->MultiplayerPal[1] = NEA_PaletteCreate();

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->OnePlayerMat[0], this->OnePlayerPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/OnePlayerButton_png.grf");
                        
    NEA_MaterialTexLoadGRF(this->OnePlayerMat[1], this->OnePlayerPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/OnePlayerButtonPressed_png.grf");
    
    NEA_MaterialTexLoadGRF(this->MultiplayerMat[0], this->MultiplayerPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/MultiplayerButton_png.grf");

    NEA_MaterialTexLoadGRF(this->MultiplayerMat[1], this->MultiplayerPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/MultiplayerButtonPressed_png.grf");


    NEA_MaterialTexLoadGRF(this->BackMat[0], this->BackPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButton_png.grf");

    NEA_MaterialTexLoadGRF(this->BackMat[1], this->BackPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButtonPressed_png.grf");


    this->OnePlayerButton = NEA_GUIButtonCreate(60, 50,
                                            60 + 128, 50 + 32);
    NEA_GUIButtonConfig(this->OnePlayerButton,
                        this->OnePlayerMat[0], NEA_White, 31,
                        this->OnePlayerMat[1], NEA_White, 31);

    this->MultiplayerButton = NEA_GUIButtonCreate(60, 100,
                                            60 + 128, 100 + 32);
    
    NEA_GUIButtonConfig(this->MultiplayerButton,
                        this->MultiplayerMat[0], NEA_White, 31,
                        this->MultiplayerMat[1], NEA_White, 31);

    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);
    
    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);
}

void MainMenu::LoadAssetsOnePlayerPartyStart()
{
    this->EmptyMat = NEA_MaterialCreate();
    this->EmptyMat = NEA_MaterialCreate();


    this->NextPlayerMat[0] = NEA_MaterialCreate();
    this->NextPlayerMat[1] = NEA_MaterialCreate();
    this->NextPlayerPal[0] = NEA_PaletteCreate();
    this->NextPlayerPal[1] = NEA_PaletteCreate();

    this->PrevPlayerMat[0] = NEA_MaterialCreate();
    this->PrevPlayerMat[1] = NEA_MaterialCreate();
    this->PrevPlayerPal[0] = NEA_PaletteCreate();
    this->PrevPlayerPal[1] = NEA_PaletteCreate();

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    this->StartGameMat[0] = NEA_MaterialCreate();
    this->StartGameMat[1] = NEA_MaterialCreate();
    this->StartGamePal[0] = NEA_PaletteCreate();
    this->StartGamePal[1] = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->EmptyMat, this->EmptyPal, NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/EmptyPlayerNumberButton_png.grf");

    NEA_MaterialTexLoadGRF(this->BackMat[0], this->BackPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButton_png.grf");

    NEA_MaterialTexLoadGRF(this->BackMat[1], this->BackPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->NextPlayerMat[0], this->NextPlayerPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/NextPlayerButton_png.grf");

    NEA_MaterialTexLoadGRF(this->NextPlayerMat[1], this->NextPlayerPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/NextPlayerButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->PrevPlayerMat[0], this->PrevPlayerPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/PrevPlayerButton_png.grf");

    NEA_MaterialTexLoadGRF(this->PrevPlayerMat[1], this->PrevPlayerPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/PrevPlayerButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->StartGameMat[0], this->StartGamePal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButton_png.grf");

    
    NEA_MaterialTexLoadGRF(this->StartGameMat[1], this->StartGamePal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButtonPressed_png.grf");
    
    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);
    
    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    this->StartGameButton = NEA_GUIButtonCreate(180, 160,
                                        180 + 64, 160 + 32);
    
    NEA_GUIButtonConfig(this->StartGameButton,
                        this->StartGameMat[0], NEA_White, 31,
                        this->StartGameMat[1], NEA_White, 31);
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

    // Per-screen materials/palettes are created in their own LoadAssets*
    // functions and freed in the matching UnloadAssets* — creating them
    // here once would leave dangling handles after the first Unload.
}

void MainMenu::UnloadAssetsMainSelectionMenu()
{
    NEA_GUIDeleteObject(this->PlayButton);

    NEA_MaterialDelete(this->PlayMat[0]);
    NEA_MaterialDelete(this->PlayMat[1]);

    NEA_PaletteDelete(this->PlayPal[0]);
    NEA_PaletteDelete(this->PlayPal[1]);
}

void MainMenu::UnloadAssetsPlaySelectionMenu()
{
    NEA_GUIDeleteObject(this->OnePlayerButton);
    NEA_GUIDeleteObject(this->MultiplayerButton);
    NEA_GUIDeleteObject(this->BackButton);

    NEA_MaterialDelete(this->OnePlayerMat[0]);
    NEA_MaterialDelete(this->OnePlayerMat[1]);

    NEA_MaterialDelete(this->MultiplayerMat[0]);
    NEA_MaterialDelete(this->MultiplayerMat[1]);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);

    NEA_PaletteDelete(this->OnePlayerPal[0]);
    NEA_PaletteDelete(this->OnePlayerPal[1]);
    
    NEA_PaletteDelete(this->MultiplayerPal[0]);
    NEA_PaletteDelete(this->MultiplayerPal[1]);

    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);

}

void MainMenu::UnloadAssetsOnePlayerPartyStart()
{

}

void MainMenu::ProcessLogicMainTitle()
{
    if (this->brightness == 0)
    {
        if (this->keys & KEY_TOUCH)
        {
            this->mainmenustates = MainMenuStates::MainSelectionMenu;
            this->LoadAssetsMainSelectionMenu();
        }        
    }
}

void MainMenu::ProcessLogicMainSelectionMenu()
{
    if ( NEA_GUIObjectGetEvent(this->PlayButton) == NEA_Clicked)
    {
        this->mainmenustates = MainMenuStates::PlaySelectionMenu;
        this->UnloadAssetsMainSelectionMenu();
        this->LoadAssetsPlaySelectionMenu();
                    
    } 
}

void MainMenu::ProcessLogicPlaySelectionMenu()
{
    if ( NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        this->mainmenustates = MainMenuStates::MainSelectionMenu;
        this->UnloadAssetsPlaySelectionMenu();
        this->LoadAssetsMainSelectionMenu();  
    }
    else if ( NEA_GUIObjectGetEvent(this->OnePlayerButton) == NEA_Clicked)
    {
        this->mainmenustates = MainMenuStates::OnePlayerPartyStart;
        this->UnloadAssetsPlaySelectionMenu();
        this->LoadAssetsOnePlayerPartyStart();
    }
}

void MainMenu::ProcessLogicOnePlayerPartyStart()
{

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
                this->ProcessLogicMainSelectionMenu();
                break;
            }

            case MainMenuStates::PlaySelectionMenu:
            {
                this->ProcessLogicPlaySelectionMenu();
                break;
            }

            case MainMenuStates::OnePlayerPartyStart:
            {
                this->ProcessLogicOnePlayerPartyStart();
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