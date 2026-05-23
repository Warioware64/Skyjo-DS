#include "OnePlayerPartyStart.hpp"
#include "MainMenu.hpp"
#include <NEAGUI.h>
#include <NEATexture.h>


void OnePlayerPartyStart::LoadAssetsOnePlayerPartyStart()
{
    this->EmptyMat = NEA_MaterialCreate();
    this->EmptyPal = NEA_PaletteCreate();


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


                            
    // Number CPU player section
    //

    this->PrevPlayerNumberCPUButton = NEA_GUIButtonCreate(25, 50,
                                                         25 + 32, 50 + 32);
    NEA_GUIButtonConfig(this->PrevPlayerNumberCPUButton,
                        this->PrevPlayerMat[0], NEA_White, 31,
                        this->PrevPlayerMat[1], NEA_White, 31);

    this->NextPlayerNumberCPUButton = NEA_GUIButtonCreate(190, 50,
                                                         190 + 32, 50 + 32);
    NEA_GUIButtonConfig(this->NextPlayerNumberCPUButton,
                        this->NextPlayerMat[0], NEA_White, 31,
                        this->NextPlayerMat[1], NEA_White, 31);


    this->EmptyNumberCPUButton = NEA_GUIButtonCreate(60, 50,
                                            60 + 128, 50 + 32);

    NEA_GUIButtonConfig(this->EmptyNumberCPUButton,
                        this->EmptyMat, NEA_White, 31,
                        this->EmptyMat, NEA_White, 31);

    // Number CPU level section
    //

    this->PrevPlayerLevelCPUButton = NEA_GUIButtonCreate(25, 120,
                                                         25 + 32, 120 + 32);
    NEA_GUIButtonConfig(this->PrevPlayerLevelCPUButton,
                        this->PrevPlayerMat[0], NEA_White, 31,
                        this->PrevPlayerMat[1], NEA_White, 31);

    this->NextPlayerLevelCPUButton = NEA_GUIButtonCreate(190, 120,
                                                         190 + 32, 120 + 32);
    NEA_GUIButtonConfig(this->NextPlayerLevelCPUButton,
                        this->NextPlayerMat[0], NEA_White, 31,
                        this->NextPlayerMat[1], NEA_White, 31);


    this->EmptyLevelCPUButton = NEA_GUIButtonCreate(60, 120,
                                            60 + 128, 120 + 32);

    NEA_GUIButtonConfig(this->EmptyLevelCPUButton,
                        this->EmptyMat, NEA_White, 31,
                        this->EmptyMat, NEA_White, 31);





    //
    //


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

void OnePlayerPartyStart::UnloadAssetsOnePlayerPartyStart()
{
    NEA_GUIDeleteObject(this->EmptyNumberCPUButton);
    NEA_GUIDeleteObject(this->EmptyLevelCPUButton);
    NEA_GUIDeleteObject(this->NextPlayerNumberCPUButton);
    NEA_GUIDeleteObject(this->NextPlayerLevelCPUButton);

    NEA_GUIDeleteObject(this->PrevPlayerNumberCPUButton);
    NEA_GUIDeleteObject(this->PrevPlayerLevelCPUButton);
    NEA_GUIDeleteObject(this->StartGameButton);
    NEA_GUIDeleteObject(this->BackButton);

    NEA_MaterialDelete(this->EmptyMat);
    NEA_PaletteDelete(this->EmptyPal);

    NEA_MaterialDelete(this->NextPlayerMat[0]);
    NEA_PaletteDelete(this->NextPlayerPal[0]);

    NEA_MaterialDelete(this->NextPlayerMat[1]);
    NEA_PaletteDelete(this->NextPlayerPal[1]);

    NEA_MaterialDelete(this->PrevPlayerMat[0]);
    NEA_PaletteDelete(this->PrevPlayerPal[0]);

    NEA_MaterialDelete(this->PrevPlayerMat[1]);
    NEA_PaletteDelete(this->PrevPlayerPal[1]);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_PaletteDelete(this->BackPal[0]);

    NEA_MaterialDelete(this->BackMat[1]);
    NEA_PaletteDelete(this->BackPal[1]);

    NEA_MaterialDelete(this->StartGameMat[0]);
    NEA_PaletteDelete(this->StartGamePal[0]);

    NEA_MaterialDelete(this->StartGameMat[1]);
    NEA_PaletteDelete(this->StartGamePal[1]);
}

std::optional<MainMenuStates> OnePlayerPartyStart::ProcessLogicOnePlayerPartyStart()
{
    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        return MainMenuStates::PlaySelectionMenu;
    }
    return std::nullopt;
}
