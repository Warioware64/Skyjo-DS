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

    NEA_ClearColorSet(NEA_White, 31, 63);

    NEA_SpriteDraw(this->bgSprite[0]);
}

void Intro::SCREEN_BOTTOM()
{
    NEA_2DViewInit();

    NEA_ClearColorSet(NEA_White, 31, 63);

    NEA_SpriteDraw(this->bgSprite[1]);
}
void Intro::LoadAssetsIntro()
{

    this->matBg[0] = NEA_MaterialCreate();
    this->palBG[0] = NEA_PaletteCreate();

    this->matBg[1] = NEA_MaterialCreate();
    this->palBG[1] = NEA_PaletteCreate();

    this->matBg[2] = NEA_MaterialCreate();
    this->palBG[2] = NEA_PaletteCreate();


    this->matBg[3] = NEA_MaterialCreate();
    this->palBG[3] = NEA_PaletteCreate();

    
    
    NEA_MaterialTexLoadGRF(this->matBg[0], this->palBG[0],
                         NEA_TEXGEN_TEXCOORD, "introTitle/MainSCREEN_png.grf");
    NEA_MaterialTexLoadGRF(this->matBg[1], this->palBG[1],
                        NEA_TEXGEN_TEXCOORD, "introTitle/BottomSCREEN_png.grf");

    NEA_MaterialTexLoadGRF(this->matBg[2], this->palBG[2],
                         NEA_TEXGEN_TEXCOORD, "introTitle/MainSCREEN2_png.grf");
    NEA_MaterialTexLoadGRF(this->matBg[3], this->palBG[3],
                        NEA_TEXGEN_TEXCOORD, "introTitle/BottomSCREEN2_png.grf");
    
    this->bgSprite[0] = NEA_SpriteCreate();
    this->bgSprite[1] = NEA_SpriteCreate();

    NEA_SpriteSetMaterial(this->bgSprite[0], this->matBg[0]);
    NEA_SpriteSetMaterial(this->bgSprite[1], this->matBg[1]);

    NEA_SpriteSetPos(this->bgSprite[0], 0, 0);
    NEA_SpriteSetPos(this->bgSprite[1], 0, 0);

    DEBUG_PRINT("Intro::LoadAssetsIntro() : Assets loaded");
    //NEA_SpriteSetPriority(this->bgSprite[0], 10);
    //NEA_SpriteSetPriority(this->bgSprite[1], 10);
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
        NEA_ProcessDual([](){
            intro.SCREEN_BOTTOM();
        }, [](){
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
            NEA_SpriteSetMaterial(this->bgSprite[0], this->matBg[2]);
            NEA_SpriteSetMaterial(this->bgSprite[1], this->matBg[3]);

        }

    }
}

void Intro::UnloadAssetsIntro()
{
    NEA_MaterialDelete(this->matBg[0]);
    NEA_MaterialDelete(this->matBg[1]);
    NEA_MaterialDelete(this->matBg[2]);
    NEA_MaterialDelete(this->matBg[3]);

    NEA_PaletteDelete(this->palBG[0]);
    NEA_PaletteDelete(this->palBG[1]);
    NEA_PaletteDelete(this->palBG[2]);
    NEA_PaletteDelete(this->palBG[3]);

    NEA_SpriteDelete(this->bgSprite[0]);
    NEA_SpriteDelete(this->bgSprite[1]);
    DEBUG_PRINT("Intro::UnloadAssetsIntro() : Assets Unloaded");
}
Intro intro;