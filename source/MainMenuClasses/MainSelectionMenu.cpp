#include "MainSelectionMenu.hpp"


void MainSelectionMenu::LoadAssetsMainSelectionMenu()
{
    // Materials/palettes are created here and destroyed in the matching
    // UnloadAssetsMainSelectionMenu — keep Create/Delete paired per screen.
    this->PlayMat[0] = NEA_MaterialCreate();
    this->PlayMat[1] = NEA_MaterialCreate();
    this->PlayPal[0] = NEA_PaletteCreate();
    this->PlayPal[1] = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->PlayMat[0], this->PlayPal[0], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/playButton_png.grf");
    NEA_MaterialTexLoadGRF(this->PlayMat[1], this->PlayPal[1], NEA_TEXGEN_TEXCOORD, "mainmenu/btns/playButtonPressed_png.grf");

    this->PlayButton = NEA_GUIButtonCreate(60, 50,
                                           60 + 128, 50 + 32);
    NEA_GUIButtonConfig(this->PlayButton,
                        this->PlayMat[0], NEA_White, 31,
                        this->PlayMat[1], NEA_White, 31);
}

void MainSelectionMenu::UnloadAssetsMainSelectionMenu()
{
    NEA_GUIDeleteObject(this->PlayButton);

    NEA_MaterialDelete(this->PlayMat[0]);
    NEA_MaterialDelete(this->PlayMat[1]);

    NEA_PaletteDelete(this->PlayPal[0]);
    NEA_PaletteDelete(this->PlayPal[1]);
}

std::optional<MainMenuStates> MainSelectionMenu::ProcessLogicMainSelectionMenu()
{
    if (NEA_GUIObjectGetEvent(this->PlayButton) == NEA_Clicked)
    {
        return MainMenuStates::PlaySelectionMenu;
    }
    return std::nullopt;
}
