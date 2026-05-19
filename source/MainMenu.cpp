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

    NEA_ClearColorSet(NEA_White, 31, 63);

    NEA_SpriteDraw(this->hexBGspr[0]);

    if (this->mainmenustates == MainMenuStates::MainTitle)
    {
        if ((this->showOrNotTouchScreenText / 30) % 2 == 0)
        {
            NEA_RichTextRender3DAlpha(0, "Touch screen!", 30, 120,
                POLY_ALPHA(20) | POLY_CULL_BACK, 30);
        }
        this->showOrNotTouchScreenText++;
    }
}

void MainMenu::SCREEN_BOTTOM()
{
    NEA_2DViewInit();

    NEA_ClearColorSet(NEA_White, 31, 63);

    NEA_SpriteDraw(this->hexBGspr[1]);
}
void MainMenu::LoadAssetsMainMenu()
{
    this->brightness = 16;
    this->frameTrigger = 0;
    this->frameDoCount = true;

    NEA_RichTextResetSystem();

    NEA_RichTextInit(0);
    NEA_RichTextMetadataLoadFAT(0, "MainMenu/font/DejaVuSans-Bold.fnt");
    NEA_RichTextMaterialLoadGRF(0, "MainMenu/font/DejaVuSans-Bold_0_png.grf");

    this->hexBGmat = NEA_MaterialCreate();
    this->hexBGpal = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->hexBGmat, this->hexBGpal,
                         NEA_TEXGEN_TEXCOORD, "MainMenu/hex_background_png.grf");
                         
    this->hexBGspr[0] = NEA_SpriteCreate();
    this->hexBGspr[1] = NEA_SpriteCreate();


    NEA_SpriteSetMaterial(this->hexBGspr[0], this->hexBGmat);
    NEA_SpriteSetMaterial(this->hexBGspr[1], this->hexBGmat);

    NEA_SpriteSetPos(this->hexBGspr[0], 0, 0);
    NEA_SpriteSetPos(this->hexBGspr[1], 0, -50);  
    NEA_SpriteSetRot(this->hexBGspr[1], 256 & 511);
    
    NEA_SpriteSetParams(this->hexBGspr[0], 31, 60, NEA_White);
    NEA_SpriteSetParams(this->hexBGspr[1], 31, 60, NEA_White);
}

void MainMenu::RenderMainMenu()
{
    while(1)
    {
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(0));

        if (this->frameTrigger == 5)
        {
            this->frameTrigger = 0;
            this->brightness--;
            if (this->brightness == 0)
                this->frameDoCount = false;
        }

        setBrightness(3, this->brightness);
        NEA_ProcessDual([](){
            mainmenu.SCREEN_BOTTOM();
        }, [](){
            mainmenu.SCREEN_TOP();
        });

        if (this->frameDoCount)
            this->frameTrigger++;

    }
}
MainMenu mainmenu;