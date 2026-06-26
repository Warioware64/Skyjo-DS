#include "MainSelectionMenu.hpp"
#include "MainMenuStates.hpp"


void MainSelectionMenu::LoadAssetsMainSelectionMenu()
{
    // Materials/palettes are created here and destroyed in the matching
    // UnloadAssetsMainSelectionMenu — keep Create/Delete paired per screen.
    this->PlayMat[0] = NEA_MaterialCreate();
    this->PlayMat[1] = NEA_MaterialCreate();
    this->PlayPal[0] = NEA_PaletteCreate();
    this->PlayPal[1] = NEA_PaletteCreate();

    this->SettingsMat[0] = NEA_MaterialCreate();
    this->SettingsMat[1] = NEA_MaterialCreate();
    this->SettingsPal[0] = NEA_PaletteCreate();
    this->SettingsPal[1] = NEA_PaletteCreate();

    if (resumableParty)
    {
        this->ResumeMat[0] = NEA_MaterialCreate();
        this->ResumeMat[1] = NEA_MaterialCreate();
        this->ResumePal[0] = NEA_PaletteCreate();
        this->ResumePal[1] = NEA_PaletteCreate();

        NEA_MaterialTexLoadGRF(this->ResumeMat[0], this->ResumePal[0], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/ResumeButton_png.grf");
        NEA_MaterialTexLoadGRF(this->ResumeMat[1], this->ResumePal[1], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/ResumeButtonPressed_png.grf");
    }

    NEA_MaterialTexLoadGRF(this->PlayMat[0], this->PlayPal[0], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/playButton_png.grf");
    NEA_MaterialTexLoadGRF(this->PlayMat[1], this->PlayPal[1], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/playButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->SettingsMat[0], this->SettingsPal[0], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/SettingsButton_png.grf");
    NEA_MaterialTexLoadGRF(this->SettingsMat[1], this->SettingsPal[1], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/SettingsButtonPressed_png.grf");

    if (!resumableParty)
    {
        this->PlayButton = NEA_GUIButtonCreate(60, 50,
                                               60 + 128, 50 + 32);
        NEA_GUIButtonConfig(this->PlayButton,
                            this->PlayMat[0], NEA_White, 31,
                            this->PlayMat[1], NEA_White, 31);

        this->SettingsButton = NEA_GUIButtonCreate(60, 100,
                                                60 + 128, 100 + 32);

        NEA_GUIButtonConfig(this->SettingsButton,
                            this->SettingsMat[0], NEA_White, 31,
                            this->SettingsMat[1], NEA_White, 31);
    }
    else 
    {
        this->PlayButton = NEA_GUIButtonCreate(60, 80,
                                               60 + 128, 80 + 32);
        NEA_GUIButtonConfig(this->PlayButton,
                            this->PlayMat[0], NEA_White, 31,
                            this->PlayMat[1], NEA_White, 31);

        this->SettingsButton = NEA_GUIButtonCreate(60, 130,
                                                60 + 128, 130 + 32);

        NEA_GUIButtonConfig(this->SettingsButton,
                            this->SettingsMat[0], NEA_White, 31,
                            this->SettingsMat[1], NEA_White, 31);
                            
        this->ResumeButton = NEA_GUIButtonCreate(60, 30,
                                               60 + 128, 30 + 32);

        NEA_GUIButtonConfig(this->ResumeButton,
                            this->ResumeMat[0], NEA_White, 31,
                            this->ResumeMat[1], NEA_White, 31);    
    }
}

void MainSelectionMenu::UnloadAssetsMainSelectionMenu()
{
    NEA_GUIDeleteObject(this->PlayButton);
    NEA_GUIDeleteObject(this->SettingsButton);

    NEA_MaterialDelete(this->PlayMat[0]);
    NEA_MaterialDelete(this->PlayMat[1]);
    NEA_MaterialDelete(this->SettingsMat[0]);
    NEA_MaterialDelete(this->SettingsMat[1]);

    NEA_PaletteDelete(this->PlayPal[0]);
    NEA_PaletteDelete(this->PlayPal[1]);
    NEA_PaletteDelete(this->SettingsPal[0]);
    NEA_PaletteDelete(this->SettingsPal[1]);

    if (resumableParty)
    {
        NEA_GUIDeleteObject(this->ResumeButton);

        NEA_MaterialDelete(this->ResumeMat[0]);
        NEA_MaterialDelete(this->ResumeMat[1]);
        NEA_PaletteDelete(this->ResumePal[0]);
        NEA_PaletteDelete(this->ResumePal[1]);
    }
}

std::optional<MainMenuStates> MainSelectionMenu::ProcessLogicMainSelectionMenu()
{
    if (NEA_GUIObjectGetEvent(this->PlayButton) == NEA_Clicked)
    {
        return MainMenuStates::PlaySelectionMenu;
    }
    else if (NEA_GUIObjectGetEvent(this->SettingsButton) == NEA_Clicked)
    {
        return MainMenuStates::SettingsMenu;
    }
    return std::nullopt;
}

void MainSelectionMenu::ActionMainSelectionMenu()
{
    NEA_GUIDraw();
}