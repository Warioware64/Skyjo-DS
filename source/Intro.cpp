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

    //NEA_ClearColorSet(NEA_White, 31, 63);

    NEA_SpriteDraw(this->bgSprite[0]);
}

void Intro::SCREEN_BOTTOM()
{
    NEA_2DViewInit();

    //NEA_ClearColorSet(NEA_White, 31, 63);

    NEA_SpriteDraw(this->bgSprite[1]);
}
void Intro::LoadAssetsIntro()
{
    this->matBg[0] = NEA_MaterialCreate();
    this->palBG[0] = NEA_PaletteCreate();

    this->matBg[1] = NEA_MaterialCreate();
    this->palBG[1] = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->matBg[0], this->palBG[0],
                         NEA_TEXGEN_TEXCOORD, "introTitle/MainSCREEN_png.grf");
    NEA_MaterialTexLoadGRF(this->matBg[1], this->palBG[1],
                        NEA_TEXGEN_TEXCOORD, "introTitle/BottomSCREEN_png.grf");

    this->bgSprite[0] = NEA_SpriteCreate();
    this->bgSprite[1] = NEA_SpriteCreate();

    NEA_SpriteSetMaterial(this->bgSprite[0], this->matBg[0]);
    NEA_SpriteSetMaterial(this->bgSprite[1], this->matBg[1]);

    NEA_SpriteSetPos(this->bgSprite[0], 0, 0);
    NEA_SpriteSetPos(this->bgSprite[1], 0, 0);

    //NEA_SpriteSetPriority(this->bgSprite[0], 10);
    //NEA_SpriteSetPriority(this->bgSprite[1], 10);
}

void Intro::RenderIntro()
{
    while(1)
    {
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(0));
        NEA_ProcessDual([](){
            intro.SCREEN_BOTTOM();
        }, [](){
            intro.SCREEN_TOP();
        });
    }
}
Intro intro;