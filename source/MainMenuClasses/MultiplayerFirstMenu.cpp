#include "MultiplayerFirstMenu.hpp"
#include "../NeaDelete.hpp"
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

    this->DlPlayMat[0] = NEA_MaterialCreate();
    this->DlPlayMat[1] = NEA_MaterialCreate();
    this->DlPlayPal[0] = NEA_PaletteCreate();
    this->DlPlayPal[1] = NEA_PaletteCreate();

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

    assets.QueueTexGRF(this->DlPlayMat[0], this->DlPlayPal[0],
                       "mainmenu/btns/DownloadPlayButton_png.grf");

    assets.QueueTexGRF(this->DlPlayMat[1], this->DlPlayPal[1],
                       "mainmenu/btns/DownloadPlayButtonPressed_png.grf");


    this->HostButton = NEA_GUIButtonCreate(60, 40,
                                            60 + 128, 40 + 32);
    NEA_GUIButtonConfig(this->HostButton,
                        this->HostMat[0], NEA_White, 31,
                        this->HostMat[1], NEA_White, 31);

    this->JoinButton = NEA_GUIButtonCreate(60, 85,
                                            60 + 128, 85 + 32);

    NEA_GUIButtonConfig(this->JoinButton,
                        this->JoinMat[0], NEA_White, 31,
                        this->JoinMat[1], NEA_White, 31);

    // Third way into a multiplayer game, for players whose friends don't own
    // the cartridge: the host sends them the game instead.
    this->DlPlayButton = NEA_GUIButtonCreate(60, 125,
                                             60 + 128, 125 + 32);

    NEA_GUIButtonConfig(this->DlPlayButton,
                        this->DlPlayMat[0], NEA_White, 31,
                        this->DlPlayMat[1], NEA_White, 31);

    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);

    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    assets.Wait("Loading...");
}

void MultiplayerFirstMenu::UnloadAssetsMultiplayerFirstMenu()
{
    DeleteGUI(this->HostButton);
    DeleteGUI(this->JoinButton);
    DeleteGUI(this->DlPlayButton);
    DeleteGUI(this->BackButton);

    DeleteMaterial(this->DlPlayMat[0]);
    DeleteMaterial(this->DlPlayMat[1]);
    DeletePalette(this->DlPlayPal[0]);
    DeletePalette(this->DlPlayPal[1]);

    DeleteMaterial(this->HostMat[0]);
    DeleteMaterial(this->HostMat[1]);

    DeleteMaterial(this->JoinMat[0]);
    DeleteMaterial(this->JoinMat[1]);

    DeleteMaterial(this->BackMat[0]);
    DeleteMaterial(this->BackMat[1]);

    DeletePalette(this->HostPal[0]);
    DeletePalette(this->HostPal[1]);

    DeletePalette(this->JoinPal[0]);
    DeletePalette(this->JoinPal[1]);

    DeletePalette(this->BackPal[0]);
    DeletePalette(this->BackPal[1]);
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
    else if (GuiClicked(this->DlPlayButton))
    {
        return MainMenuStates::MultiplayerDlPlayMenu;
    }
    return std::nullopt;
}

void MultiplayerFirstMenu::ActionMultiplayerFirstMenu()
{
    NEA_GUIDraw();
}