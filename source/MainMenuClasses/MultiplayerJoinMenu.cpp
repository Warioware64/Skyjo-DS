#include "MultiplayerJoinMenu.hpp"


void MultiplayerJoinMenu::LoadAssetsMultiplayerJoinMenu()
{


    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();



    NEA_MaterialTexLoadGRF(this->BackMat[0], this->BackPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButton_png.grf");

    NEA_MaterialTexLoadGRF(this->BackMat[1], this->BackPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButtonPressed_png.grf");



    this->BackButton = NEA_GUIButtonCreate(5, 160,
                                            5 + 64, 160 + 32);

    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);
}

void MultiplayerJoinMenu::UnloadAssetsMultiplayerJoinMenu()
{

    NEA_GUIDeleteObject(this->BackButton);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);

    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);
}

std::optional<MainMenuStates> MultiplayerJoinMenu::ProcessLogicMultiplayerJoinMenu()
{
    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        return MainMenuStates::MultiplayerFirstMenu;
    }
    return std::nullopt;
}

void MultiplayerJoinMenu::ActionMultiplayerJoinMenu()
{
    NEA_GUIDraw();
}