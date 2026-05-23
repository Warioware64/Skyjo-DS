#include "PlaySelectionMenu.hpp"


void PlaySelectionMenu::LoadAssetsPlaySelectionMenu()
{
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

    NEA_MaterialTexLoadGRF(this->OnePlayerMat[0], this->OnePlayerPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/OnePlayerButton_png.grf");

    NEA_MaterialTexLoadGRF(this->OnePlayerMat[1], this->OnePlayerPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/OnePlayerButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->MultiplayerMat[0], this->MultiplayerPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/MultiplayerButton_png.grf");

    NEA_MaterialTexLoadGRF(this->MultiplayerMat[1], this->MultiplayerPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/MultiplayerButtonPressed_png.grf");


    NEA_MaterialTexLoadGRF(this->BackMat[0], this->BackPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButton_png.grf");

    NEA_MaterialTexLoadGRF(this->BackMat[1], this->BackPal[1], NEA_TEXGEN_TEXCOORD,
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
    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        return MainMenuStates::MainSelectionMenu;
    }
    else if (NEA_GUIObjectGetEvent(this->OnePlayerButton) == NEA_Clicked)
    {
        return MainMenuStates::OnePlayerPartyStart;
    }
    return std::nullopt;
}

void PlaySelectionMenu::ActionPlaySelectionMenu()
{
    NEA_GUIDraw();
}