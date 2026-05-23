#include "OnePlayerPartyStart.hpp"
#include "MainMenu.hpp"
#include <NEAGUI.h>
#include <NEATexture.h>


void OnePlayerPartyStart::LoadAssetsOnePlayerPartyStart()
{
    this->player_number = 2;
    this->old_player_number = 0;

    this->cpu_level = CPULevel::Easy;
    this->old_cpu_level = std::nullopt;

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

    this->PrevPlayerLevelCPUButton = NEA_GUIButtonCreate(25, 110,
                                                         25 + 32, 110 + 32);
    NEA_GUIButtonConfig(this->PrevPlayerLevelCPUButton,
                        this->PrevPlayerMat[0], NEA_White, 31,
                        this->PrevPlayerMat[1], NEA_White, 31);

    this->NextPlayerLevelCPUButton = NEA_GUIButtonCreate(190, 110,
                                                         190 + 32, 110 + 32);
    NEA_GUIButtonConfig(this->NextPlayerLevelCPUButton,
                        this->NextPlayerMat[0], NEA_White, 31,
                        this->NextPlayerMat[1], NEA_White, 31);


    this->EmptyLevelCPUButton = NEA_GUIButtonCreate(60, 110,
                                            60 + 128, 110 + 32);

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

    this->StartGameButton = NEA_GUIButtonCreate(190, 160,
                                        190 + 64, 160 + 32);

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
    if ((this->player_number == 2) && (this->player_number != this->old_player_number))
    {
        NEA_GUIButtonConfig(this->PrevPlayerNumberCPUButton,
                            this->PrevPlayerMat[0], RGB15(15, 15, 15), 31,
                            this->PrevPlayerMat[1], RGB15(15, 15, 15), 31);
                            
        this->old_player_number = 2;
    }

    if ((this->player_number == 3) && (this->player_number != this->old_player_number))
    {
        NEA_GUIButtonConfig(this->PrevPlayerNumberCPUButton,
                            this->PrevPlayerMat[0], NEA_White, 31,
                            this->PrevPlayerMat[1], NEA_White, 31);
                            
        this->old_player_number = 3;        
    }

    if ((this->player_number == 8) && (this->player_number != this->old_player_number))
    {
        NEA_GUIButtonConfig(this->NextPlayerNumberCPUButton,
                            this->NextPlayerMat[0], RGB15(15, 15, 15), 31,
                            this->NextPlayerMat[1], RGB15(15, 15, 15), 31);
                            
        this->old_player_number = 8;
    }

    if ((this->player_number == 7) && (this->player_number != this->old_player_number))
    {
        NEA_GUIButtonConfig(this->NextPlayerNumberCPUButton,
                            this->NextPlayerMat[0], NEA_White, 31,
                            this->NextPlayerMat[1], NEA_White, 31);
                            
        this->old_player_number = 7;        
    }


    if ((this->cpu_level == CPULevel::Easy) && (this->cpu_level != this->old_cpu_level))
    {
        NEA_GUIButtonConfig(this->PrevPlayerLevelCPUButton,
                            this->PrevPlayerMat[0], RGB15(15, 15, 15), 31,
                            this->PrevPlayerMat[1], RGB15(15, 15, 15), 31);
        
        NEA_GUIButtonConfig(this->NextPlayerLevelCPUButton,
                            this->NextPlayerMat[0], NEA_White, 31,
                            this->NextPlayerMat[1], NEA_White, 31);
                            
        this->old_cpu_level = CPULevel::Easy;        
    }

    if ((this->cpu_level == CPULevel::Hard) && (this->cpu_level != this->old_cpu_level))
    {
        NEA_GUIButtonConfig(this->NextPlayerLevelCPUButton,
                            this->NextPlayerMat[0], RGB15(15, 15, 15), 31,
                            this->NextPlayerMat[1], RGB15(15, 15, 15), 31);
                            
        this->old_cpu_level = CPULevel::Hard;        
    }


    if ((this->cpu_level == CPULevel::Medium) && (this->cpu_level != this->old_cpu_level))
    {
        NEA_GUIButtonConfig(this->PrevPlayerLevelCPUButton,
                            this->PrevPlayerMat[0], NEA_White, 31,
                            this->PrevPlayerMat[1], NEA_White, 31);

        NEA_GUIButtonConfig(this->NextPlayerLevelCPUButton,
                            this->NextPlayerMat[0], NEA_White, 31,
                            this->NextPlayerMat[1], NEA_White, 31);
                            
        this->old_cpu_level = CPULevel::Medium;        
    }

    if (NEA_GUIObjectGetEvent(this->PrevPlayerNumberCPUButton) == NEA_Clicked)
    {
        if (this->player_number != 2)
            this->player_number--;
    }

    if (NEA_GUIObjectGetEvent(this->NextPlayerNumberCPUButton) == NEA_Clicked)
    {
        if (this->player_number != 8)
            this->player_number++;
    }

    if (NEA_GUIObjectGetEvent(this->PrevPlayerLevelCPUButton) == NEA_Clicked)
    {
        if (this->cpu_level != CPULevel::Easy)
            this->cpu_level = static_cast<CPULevel>( static_cast<int>(this->cpu_level) - 1);
    }

    if (NEA_GUIObjectGetEvent(this->NextPlayerLevelCPUButton) == NEA_Clicked)
    {
        if (this->cpu_level != CPULevel::Hard)
            this->cpu_level = static_cast<CPULevel>( static_cast<int>(this->cpu_level) + 1);
    }


    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        return MainMenuStates::PlaySelectionMenu;
    }


    return std::nullopt;
}

void OnePlayerPartyStart::ActionOnePlayerPartyStart()
{
    NEA_GUIDraw();
    NEA_RichTextRender3D(0, "Number of players", 72, 30);
    NEA_RichTextRender3D(0, (std::to_string(this->player_number) + std::string(" players")).c_str(), 95, 57);

    NEA_RichTextRender3D(0, "CPU Level", 87, 90);
    switch (this->cpu_level)
    {
        case CPULevel::Easy:
        {
            NEA_RichTextRender3D(0, "Easy", 100, 117);
            break;
        }

        case CPULevel::Medium:
        {
            NEA_RichTextRender3D(0, "Medium", 95, 117);
            break;
        }

        case CPULevel::Hard:
        {
            NEA_RichTextRender3D(0, "Hard", 100, 117);
            break;
        }
    }
}