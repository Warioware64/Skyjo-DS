#include "MultiplayerFirstMenu.hpp"
#include "../AssetLoader.hpp"
#include "../GuiClickSound.hpp"
#include "MainMenuStates.hpp"

void MultiplayerFirstMenu::LoadAssetsMultiplayerFirstMenu()
{
    // Every button texture is read in the background; Wait() at the end of
    // this function drains the batch. The menu calls this at the fade apex,
    // so the wait is hidden by the fade.
    AsyncAssetBatch assets;

    this->HostMat[0] = NEA_MaterialCreate();
    this->HostMat[1] = NEA_MaterialCreate();
    this->HostPal[0] = NEA_PaletteCreate();
    this->HostPal[1] = NEA_PaletteCreate();

    this->JoinMat[0] = NEA_MaterialCreate();
    this->JoinMat[1] = NEA_MaterialCreate();
    this->JoinPal[0] = NEA_PaletteCreate();
    this->JoinPal[1] = NEA_PaletteCreate();

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    assets.QueueTexGRF(this->HostMat[0], this->HostPal[0],
                       "mainmenu/btns/hostButton_png.grf");

    assets.QueueTexGRF(this->HostMat[1], this->HostPal[1],
                       "mainmenu/btns/hostButtonPressed_png.grf");

    assets.QueueTexGRF(this->JoinMat[0], this->JoinPal[0],
                       "mainmenu/btns/joinButton_png.grf");

    assets.QueueTexGRF(this->JoinMat[1], this->JoinPal[1],
                       "mainmenu/btns/joinButtonPressed_png.grf");


    assets.QueueTexGRF(this->BackMat[0], this->BackPal[0],
                       "mainmenu/btns/BackButton_png.grf");

    assets.QueueTexGRF(this->BackMat[1], this->BackPal[1],
                       "mainmenu/btns/BackButtonPressed_png.grf");


    this->HostButton = NEA_GUIButtonCreate(60, 50,
                                            60 + 128, 50 + 32);
    NEA_GUIButtonConfig(this->HostButton,
                        this->HostMat[0], NEA_White, 31,
                        this->HostMat[1], NEA_White, 31);

    this->JoinButton = NEA_GUIButtonCreate(60, 100,
                                            60 + 128, 100 + 32);

    NEA_GUIButtonConfig(this->JoinButton,
                        this->JoinMat[0], NEA_White, 31,
                        this->JoinMat[1], NEA_White, 31);

    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);

    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    assets.Wait("Loading...");
}

void MultiplayerFirstMenu::UnloadAssetsMultiplayerFirstMenu()
{
    NEA_GUIDeleteObject(this->HostButton);
    NEA_GUIDeleteObject(this->JoinButton);
    NEA_GUIDeleteObject(this->BackButton);

    NEA_MaterialDelete(this->HostMat[0]);
    NEA_MaterialDelete(this->HostMat[1]);

    NEA_MaterialDelete(this->JoinMat[0]);
    NEA_MaterialDelete(this->JoinMat[1]);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);

    NEA_PaletteDelete(this->HostPal[0]);
    NEA_PaletteDelete(this->HostPal[1]);

    NEA_PaletteDelete(this->JoinPal[0]);
    NEA_PaletteDelete(this->JoinPal[1]);

    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);
}

std::optional<MainMenuStates> MultiplayerFirstMenu::ProcessLogicMultiplayerFirstMenu()
{
    if (GuiClicked(this->BackButton))
    {
        return MainMenuStates::PlaySelectionMenu;
    }
    else if (GuiClicked(this->HostButton))
    {
        return MainMenuStates::MultiplayerHostMenu;
    }
    else if (GuiClicked(this->JoinButton))
    {
        return MainMenuStates::MultiplayerJoinMenu;
    }
    return std::nullopt;
}

void MultiplayerFirstMenu::ActionMultiplayerFirstMenu()
{
    NEA_GUIDraw();
}