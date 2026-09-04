#include "PlaySelectionMenu.hpp"
#include "../AssetLoader.hpp"
#include "../GuiClickSound.hpp"
#include "MainMenuStates.hpp"


void PlaySelectionMenu::LoadAssetsPlaySelectionMenu()
{
    // Every button texture is read in the background; Wait() at the end of
    // this function drains the batch. The menu calls this at the fade apex,
    // so the wait is hidden by the fade.
    AsyncAssetBatch assets;

    // Materials/palettes are created here and destroyed in the matching
    // UnloadAssetsPlaySelectionMenu — keep Create/Delete paired per screen.
    this->OnePlayerMat[0] = NEA_MaterialCreate();
    this->OnePlayerMat[1] = NEA_MaterialCreate();
    this->OnePlayerPal[0] = NEA_PaletteCreate();
    this->OnePlayerPal[1] = NEA_PaletteCreate();

    this->MultiplayerMat[0] = NEA_MaterialCreate();
    this->MultiplayerMat[1] = NEA_MaterialCreate();
    this->MultiplayerPal[0] = NEA_PaletteCreate();
    this->MultiplayerPal[1] = NEA_PaletteCreate();

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    assets.QueueTexGRF(this->OnePlayerMat[0], this->OnePlayerPal[0],
                       "mainmenu/btns/OnePlayerButton_png.grf");

    assets.QueueTexGRF(this->OnePlayerMat[1], this->OnePlayerPal[1],
                       "mainmenu/btns/OnePlayerButtonPressed_png.grf");

    assets.QueueTexGRF(this->MultiplayerMat[0], this->MultiplayerPal[0],
                       "mainmenu/btns/MultiplayerButton_png.grf");

    assets.QueueTexGRF(this->MultiplayerMat[1], this->MultiplayerPal[1],
                       "mainmenu/btns/MultiplayerButtonPressed_png.grf");


    assets.QueueTexGRF(this->BackMat[0], this->BackPal[0],
                       "mainmenu/btns/BackButton_png.grf");

    assets.QueueTexGRF(this->BackMat[1], this->BackPal[1],
                       "mainmenu/btns/BackButtonPressed_png.grf");


    this->OnePlayerButton = NEA_GUIButtonCreate(60, 50,
                                            60 + 128, 50 + 32);
    NEA_GUIButtonConfig(this->OnePlayerButton,
                        this->OnePlayerMat[0], NEA_White, 31,
                        this->OnePlayerMat[1], NEA_White, 31);

    this->MultiplayerButton = NEA_GUIButtonCreate(60, 100,
                                            60 + 128, 100 + 32);

    NEA_GUIButtonConfig(this->MultiplayerButton,
                        this->MultiplayerMat[0], NEA_White, 31,
                        this->MultiplayerMat[1], NEA_White, 31);

    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);

    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    assets.Wait("Loading...");
}

void PlaySelectionMenu::UnloadAssetsPlaySelectionMenu()
{
    NEA_GUIDeleteObject(this->OnePlayerButton);
    NEA_GUIDeleteObject(this->MultiplayerButton);
    NEA_GUIDeleteObject(this->BackButton);

    NEA_MaterialDelete(this->OnePlayerMat[0]);
    NEA_MaterialDelete(this->OnePlayerMat[1]);

    NEA_MaterialDelete(this->MultiplayerMat[0]);
    NEA_MaterialDelete(this->MultiplayerMat[1]);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);

    NEA_PaletteDelete(this->OnePlayerPal[0]);
    NEA_PaletteDelete(this->OnePlayerPal[1]);

    NEA_PaletteDelete(this->MultiplayerPal[0]);
    NEA_PaletteDelete(this->MultiplayerPal[1]);

    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);
}

std::optional<MainMenuStates> PlaySelectionMenu::ProcessLogicPlaySelectionMenu()
{
    if (GuiClicked(this->BackButton))
    {
        return MainMenuStates::MainSelectionMenu;
    }
    else if (GuiClicked(this->OnePlayerButton))
    {
        return MainMenuStates::OnePlayerPartyStart;
    }
    else if (GuiClicked(this->MultiplayerButton))
    {
        return MainMenuStates::MultiplayerFirstMenu;
    }
    return std::nullopt;
}

void PlaySelectionMenu::ActionPlaySelectionMenu()
{
    NEA_GUIDraw();
}