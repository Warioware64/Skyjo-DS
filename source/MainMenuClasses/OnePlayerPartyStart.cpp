#include "OnePlayerPartyStart.hpp"
#include "../AssetLoader.hpp"
#include "../GuiClickSound.hpp"
#include "MainMenu.hpp"
#include "MainMenuStates.hpp"


int OnePlayerPartyStart::Get_player_number()
{
    return this->player_number;
}

CPULevel OnePlayerPartyStart::Get_CPULevel()
{
    return this->cpu_level;
}

void OnePlayerPartyStart::LoadAssetsOnePlayerPartyStart()
{
    // Every button texture is read in the background; Wait() at the end of
    // this function drains the batch. The menu calls this at the fade apex,
    // so the wait is hidden by the fade.
    AsyncAssetBatch assets;

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

    assets.QueueTexGRF(this->EmptyMat, this->EmptyPal,
                       "mainmenu/btns/EmptyPlayerNumberButton_png.grf");

    assets.QueueTexGRF(this->BackMat[0], this->BackPal[0],
                       "mainmenu/btns/BackButton_png.grf");

    assets.QueueTexGRF(this->BackMat[1], this->BackPal[1],
                       "mainmenu/btns/BackButtonPressed_png.grf");

    assets.QueueTexGRF(this->NextPlayerMat[0], this->NextPlayerPal[0],
                       "mainmenu/btns/NextPlayerButton_png.grf");

    assets.QueueTexGRF(this->NextPlayerMat[1], this->NextPlayerPal[1],
                       "mainmenu/btns/NextPlayerButtonPressed_png.grf");

    assets.QueueTexGRF(this->PrevPlayerMat[0], this->PrevPlayerPal[0],
                       "mainmenu/btns/PrevPlayerButton_png.grf");

    assets.QueueTexGRF(this->PrevPlayerMat[1], this->PrevPlayerPal[1],
                       "mainmenu/btns/PrevPlayerButtonPressed_png.grf");

    assets.QueueTexGRF(this->StartGameMat[0], this->StartGamePal[0],
                       "mainmenu/btns/StartGameButton_png.grf");


    assets.QueueTexGRF(this->StartGameMat[1], this->StartGamePal[1],
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

    assets.Wait("Loading...");
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

    if (GuiClicked(this->PrevPlayerNumberCPUButton))
    {
        if (this->player_number != 2)
            this->player_number--;
    }

    if (GuiClicked(this->NextPlayerNumberCPUButton))
    {
        if (this->player_number != 8)
            this->player_number++;
    }

    if (GuiClicked(this->PrevPlayerLevelCPUButton))
    {
        if (this->cpu_level != CPULevel::Easy)
            this->cpu_level = static_cast<CPULevel>( static_cast<int>(this->cpu_level) - 1);
    }

    if (GuiClicked(this->NextPlayerLevelCPUButton))
    {
        if (this->cpu_level != CPULevel::Hard)
            this->cpu_level = static_cast<CPULevel>( static_cast<int>(this->cpu_level) + 1);
    }


    if (GuiClicked(this->BackButton))
    {
        return MainMenuStates::PlaySelectionMenu;
    }
    else if (GuiClicked(this->StartGameButton))
    {
        return MainMenuStates::TransitionToPlayOnePlayer;
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