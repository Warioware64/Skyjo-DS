#include "MainSelectionMenu.hpp"
#include "../NeaDelete.hpp"
#include "../AssetLoader.hpp"
#include "../GuiClickSound.hpp"
#include "MainMenuStates.hpp"


void MainSelectionMenu::LoadAssetsMainSelectionMenu()
{
    // Every button texture is read in the background; Wait() at the end of
    // this function drains the batch. The menu calls this at the fade apex,
    // so the wait is hidden by the fade.
    AsyncAssetBatch assets;

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

        assets.QueueTexGRF(this->ResumeMat[0], this->ResumePal[0],
                       "mainmenu/btns/ResumeButton_png.grf");
        assets.QueueTexGRF(this->ResumeMat[1], this->ResumePal[1],
                       "mainmenu/btns/ResumeButtonPressed_png.grf");
    }

    assets.QueueTexGRF(this->PlayMat[0], this->PlayPal[0],
                       "mainmenu/btns/playButton_png.grf");
    assets.QueueTexGRF(this->PlayMat[1], this->PlayPal[1],
                       "mainmenu/btns/playButtonPressed_png.grf");

    assets.QueueTexGRF(this->SettingsMat[0], this->SettingsPal[0],
                       "mainmenu/btns/SettingsButton_png.grf");
    assets.QueueTexGRF(this->SettingsMat[1], this->SettingsPal[1],
                       "mainmenu/btns/SettingsButtonPressed_png.grf");

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

    assets.Wait("Loading...");
}

void MainSelectionMenu::UnloadAssetsMainSelectionMenu()
{
    DeleteGUI(this->PlayButton);
    DeleteGUI(this->SettingsButton);

    DeleteMaterial(this->PlayMat[0]);
    DeleteMaterial(this->PlayMat[1]);
    DeleteMaterial(this->SettingsMat[0]);
    DeleteMaterial(this->SettingsMat[1]);

    DeletePalette(this->PlayPal[0]);
    DeletePalette(this->PlayPal[1]);
    DeletePalette(this->SettingsPal[0]);
    DeletePalette(this->SettingsPal[1]);

    // Unconditionally, even though Resume is only *created* when a saved party
    // exists. `resumableParty` is not this screen's to trust: MainMenu rewrites
    // it from the filesystem on every entry, and finishing a game deletes the
    // save file, so the flag can differ between the load that built these and
    // the unload that tears them down. Branching on it here would either leak
    // the Resume button or free handles that were never made. The deleters
    // ignore a null handle and clear the one they take, so asking for all four
    // every time is both correct and shorter.
    DeleteGUI(this->ResumeButton);

    DeleteMaterial(this->ResumeMat[0]);
    DeleteMaterial(this->ResumeMat[1]);
    DeletePalette(this->ResumePal[0]);
    DeletePalette(this->ResumePal[1]);
}

std::optional<MainMenuStates> MainSelectionMenu::ProcessLogicMainSelectionMenu()
{
    if (resumableParty && GuiClicked(this->ResumeButton))
    {
        // Flag the resume so MainMenu loads the saved party at launch, then reuse
        // the normal one-player launch transition.
        this->resumeSelected = true;
        return MainMenuStates::TransitionToPlayOnePlayer;
    }
    else if (GuiClicked(this->PlayButton))
    {
        return MainMenuStates::PlaySelectionMenu;
    }
    else if (GuiClicked(this->SettingsButton))
    {
        return MainMenuStates::SettingsMenu;
    }
    return std::nullopt;
}

void MainSelectionMenu::ActionMainSelectionMenu()
{
    NEA_GUIDraw();
}