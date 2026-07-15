#include "SettingsMenu.hpp"
#include "../Process.hpp"
#include "../Music.hpp"


void SettingsMenu::LoadAssetsSettingsMenu()
{

    oldSettings = process.gamesettings;

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

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
    
    // Number CPU player section
    //

    this->PrevPlayerMusicButton = NEA_GUIButtonCreate(25, 50,
                                                         25 + 32, 50 + 32);
    NEA_GUIButtonConfig(this->PrevPlayerMusicButton,
                        this->PrevPlayerMat[0], NEA_White, 31,
                        this->PrevPlayerMat[1], NEA_White, 31);

    this->NextPlayerMusicButton = NEA_GUIButtonCreate(190, 50,
                                                         190 + 32, 50 + 32);
    NEA_GUIButtonConfig(this->NextPlayerMusicButton,
                        this->NextPlayerMat[0], NEA_White, 31,
                        this->NextPlayerMat[1], NEA_White, 31);


    this->EmptyMusicButton = NEA_GUIButtonCreate(60, 50,
                                            60 + 128, 50 + 32);

    NEA_GUIButtonConfig(this->EmptyMusicButton,
                        this->EmptyMat, NEA_White, 31,
                        this->EmptyMat, NEA_White, 31);



    this->PrevPlayerSoundButton = NEA_GUIButtonCreate(25, 110,
                                                         25 + 32, 110 + 32);
    NEA_GUIButtonConfig(this->PrevPlayerSoundButton,
                        this->PrevPlayerMat[0], NEA_White, 31,
                        this->PrevPlayerMat[1], NEA_White, 31);

    this->NextPlayerSoundButton = NEA_GUIButtonCreate(190, 110,
                                                         190 + 32, 110 + 32);
    NEA_GUIButtonConfig(this->NextPlayerSoundButton,
                        this->NextPlayerMat[0], NEA_White, 31,
                        this->NextPlayerMat[1], NEA_White, 31);


    this->EmptySoundButton = NEA_GUIButtonCreate(60, 110,
                                            60 + 128, 110 + 32);

    NEA_GUIButtonConfig(this->EmptySoundButton,
                        this->EmptyMat, NEA_White, 31,
                        this->EmptyMat, NEA_White, 31);

    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);

    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);
}

void SettingsMenu::UnloadAssetsSettingsMenu()
{

    NEA_GUIDeleteObject(this->BackButton);
    NEA_GUIDeleteObject(this->EmptyMusicButton);
    NEA_GUIDeleteObject(this->EmptySoundButton);
    NEA_GUIDeleteObject(this->NextPlayerMusicButton);
    NEA_GUIDeleteObject(this->NextPlayerSoundButton);

    NEA_GUIDeleteObject(this->PrevPlayerMusicButton);
    NEA_GUIDeleteObject(this->PrevPlayerSoundButton);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);

    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);

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
}

std::optional<MainMenuStates> SettingsMenu::ProcessLogicSettingsMenu()
{
    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        process.CallSaveSettings();
        return MainMenuStates::MainSelectionMenu;
    }

    if (NEA_GUIObjectGetEvent(this->NextPlayerMusicButton) == NEA_Held)
    {
        if (process.gamesettings.musicSoundVolume != 1024)
        {
            this->incrementMusic++;
            process.gamesettings.musicSoundVolume += this->incrementMusic;
            if (process.gamesettings.musicSoundVolume >= 1024)
            {
                process.gamesettings.musicSoundVolume = 1024;
            }
        }
    }
    else 
    {
        this->incrementMusic = 0;
    }

    if (NEA_GUIObjectGetEvent(this->PrevPlayerMusicButton) == NEA_Held)
    {
        if (process.gamesettings.musicSoundVolume != 0)
        {
            this->decrementMusic++;
            process.gamesettings.musicSoundVolume -= this->decrementMusic;
            if (process.gamesettings.musicSoundVolume <= 0)
            {
                process.gamesettings.musicSoundVolume = 0;
            }
        }
    }
    else
    {
        this->decrementMusic = 0;
    }

    // Reflect the new music volume on the running stream immediately.
    Music::ApplyVolume();

    if (NEA_GUIObjectGetEvent(this->NextPlayerSoundButton) == NEA_Held)
    {
        if (process.gamesettings.nosesSoundVolume != 1024)
        {
            this->incrementSound++;
            process.gamesettings.nosesSoundVolume += this->incrementSound;
            if (process.gamesettings.nosesSoundVolume >= 1024)
            {
                process.gamesettings.nosesSoundVolume = 1024;
            }
        }
    }
    else {
        this->incrementSound = 0;
    }

    if (NEA_GUIObjectGetEvent(this->PrevPlayerSoundButton) == NEA_Held)
    {
        if (process.gamesettings.nosesSoundVolume != 0)
        {
            this->decrementSound++;
            process.gamesettings.nosesSoundVolume -= this->decrementSound;
            if (process.gamesettings.nosesSoundVolume <= 0)
            {
                process.gamesettings.nosesSoundVolume = 0;
            }
        }
    }
    else {
        this->decrementSound = 0;
    }
    return std::nullopt;
}

void SettingsMenu::ActionSettingsMenu()
{
    NEA_GUIDraw();
    this->oldpercentageMusic = process.gamesettings.musicSoundVolume;
    this->oldpercentageSound = process.gamesettings.nosesSoundVolume;

    this->percentageMusic = (static_cast<float>(process.gamesettings.musicSoundVolume) / 1024.0 ) * 100.0;
    this->percentageSound = (static_cast<float>(process.gamesettings.nosesSoundVolume) / 1024.0 ) * 100.0;

    this->percentageMusic = std::nearbyint(this->percentageMusic);
    this->percentageSound = std::nearbyint(this->percentageSound);

    NEA_RichTextRender3D(0, "Music", 102, 30);
    NEA_RichTextRender3D(0, (std::to_string(static_cast<int>(percentageMusic)) + "%").c_str(), 105, 57);
    NEA_RichTextRender3D(0, "Sound", 102, 90);
    NEA_RichTextRender3D(0, (std::to_string(static_cast<int>(percentageSound)) + "%").c_str(), 105, 117);
}