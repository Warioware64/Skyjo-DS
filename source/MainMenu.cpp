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

    NEA_Hw2DBGLoadGRFFAT(this->hexBGtop, "mainmenu/hex_background_png.grf", 0);
    NEA_Hw2DBGSetVisible(this->hexBGtop, true);

    this->hexBGbot = NEA_Hw2DBGCreate(NEA_ENGINE_SUB, 0,
                                       NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->hexBGbot, 3);
    NEA_Hw2DBGLoadGRFFAT(this->hexBGbot, "mainmenu/hex_background_png.grf", 1);

    NEA_Hw2DBGSetVisible(this->hexBGbot, true);

    this->PlayMat[0] = NEA_MaterialCreate();
    this->PlayMat[1] = NEA_MaterialCreate();

    this->PlayPal[0] = NEA_PaletteCreate();
    this->PlayPal[1] = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->PlayMat[0], this->PlayPal[0], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/hex_button_teal_png.grf");
    NEA_MaterialTexLoadGRF(this->PlayMat[1], this->PlayPal[1], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/hex_button_teal_pressed_png.grf");
}

void MainMenu::RenderMainMenu()
{
    while(1)
    {
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_GUI));
        scanKeys();
        uint32_t keys = keysDown();
        if ((this->mainmenustates == MainMenuStates::MainTitle) & (this->brightness == 0))
        {
            if (keys & KEY_TOUCH)
            {
                this->mainmenustates = MainMenuStates::MainSelectionMenu;
                this->PlayButton = NEA_GUIButtonCreate(50, 30,
                                                        50 + 128, 62);
                NEA_GUIButtonConfig(this->PlayButton,
                                    this->PlayMat[0], NEA_White, 31,
                                    this->PlayMat[1], NEA_White, 31);
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

        if (this->frameDoCount)
            this->frameTrigger++;

    }
}
MainMenu mainmenu;