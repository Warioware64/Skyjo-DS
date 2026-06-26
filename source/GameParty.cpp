#include "GameParty.hpp"
#include "MainMenu.hpp"
#include "MainMenuClasses/MainMenuStates.hpp"
#include "globalHeader.hpp"
#include "GamePartyClasses/HumanTouchController.hpp"
#include "GamePartyClasses/CpuController.hpp"
#include <NEAGUI.h>
#include <NEAGeneral.h>



namespace
{
    constexpr int kHeldCardX = 60;
    constexpr int kHeldCardY = 60;

    // Card animation tuning (frames at ~60fps).
    constexpr int kPopFrames = 9;    // reveal/replace "pop"
    constexpr int kClearFrames = 14; // column-clear fade-out
}

GameParty::GameParty()
{

}

GameParty::~GameParty()
{

}

void GameParty::GamePartyLogic()
{
    HandleTopScreenCycling();

    switch (this->phase)
    {
        case GamePhase::InitialReveal: TickInitialReveal(); break;
        case GamePhase::Turns:
        case GamePhase::LastRound:     TickTurn();          break;
        case GamePhase::Scoring:       TickScoring();       break;
        case GamePhase::Ended:                              break;
    }
}

void GameParty::DestroyQuitMenu()
{
    NEA_GUIDeleteObject(this->YesButton);
    NEA_GUIDeleteObject(this->NoButton);
}

void GameParty::InitQuitMenu()
{
    this->YesButton = NEA_GUIButtonCreate( 89, 60,
                                                89 + 64, 60 + 32);
    this->NoButton =  NEA_GUIButtonCreate( 89, 103,
                                            89 + 64, 103 + 32);

    NEA_GUIButtonConfig(this->YesButton,
         this->YesButtonMat, NEA_White, 31,
        this->YesButtonPressedMat, NEA_White, 31);
    
    NEA_GUIButtonConfig(this->NoButton,
         this->NoButtonMat, NEA_White, 31,
        this->NoButtonPressedMat, NEA_White, 31);   
}

void GameParty::DestroyPauseMenuMain()
{
    NEA_GUIDeleteObject(this->ContinueButton);
    NEA_GUIDeleteObject(this->QuitButton);
    NEA_GUIDeleteObject(this->SaveButton);
}

void GameParty::PauseMenuGUIlogic()
{
    if (this->pausephase == PausePhase::PauseMenuMain)
    {
        if ( (NEA_GUIObjectGetEvent(this->ContinueButton)) == NEA_Clicked)
        {
            this->DestroyPauseMenuMain();
            this->RefreshTopScreen();
            this->StartMenu = false;
        }
        else if ( (NEA_GUIObjectGetEvent(this->QuitButton)) == NEA_Clicked)
        {
            this->pausephase = PausePhase::QuitMenu;
            this->DestroyPauseMenuMain();
            this->InitQuitMenu();
            
        }
        else if ( (NEA_GUIObjectGetEvent(this->SaveButton)) == NEA_Clicked)
        {
            this->pausephase = PausePhase::SaveMenu;
            this->DestroyPauseMenuMain();
            this->InitQuitMenu();
            
        }
    }
    else if (this->pausephase == PausePhase::QuitMenu)
    {
        if ( (NEA_GUIObjectGetEvent(this->NoButton)) == NEA_Clicked)
        {
            this->DestroyQuitMenu();
            this->pausephase = PausePhase::PauseMenuMain;
            this->InitPauseMenuGUIbutton();
        }
        
        if ( (NEA_GUIObjectGetEvent(this->YesButton)) == NEA_Clicked)
        {
            this->DestroyQuitMenu();
            this->UnloadGamePartyAssets();
            this->Quited = true;
            mainmenu.mainmenustates = MainMenuStates::OnePlayerPartyStart;
            mainmenu.bypassableChangeMenuStates = true;
            /*
            constexpr std::size_t yasTestFlag = yas::file | yas::binary | yas::no_header;
            std::filesystem::path test_file(process.fatDeviceCPP + "_nds/SkyjoDS/test.dat");
            yas::file_ostream yasTestOutput(test_file.c_str());
            // yas has no serializer for std::filesystem::path; serialize a string.
            //std::string test_payload = test_file.string();
            yas::save<yasTestFlag>(yasTestOutput, gameparty);
            yasTestOutput.flush();
            */

        } 
    }
    else if (this->pausephase == PausePhase::SaveMenu)
    {
        if ( (NEA_GUIObjectGetEvent(this->NoButton)) == NEA_Clicked)
        {
            this->DestroyQuitMenu();
            this->pausephase = PausePhase::PauseMenuMain;
            this->InitPauseMenuGUIbutton();
        }
        
        if ( (NEA_GUIObjectGetEvent(this->YesButton)) == NEA_Clicked)
        {
            this->DestroyQuitMenu();
            this->UnloadGamePartyAssets();
            this->Quited = true;
            mainmenu.mainmenustates = MainMenuStates::MainSelectionMenu;
            mainmenu.bypassableChangeMenuStates = true;
            
            constexpr std::size_t yasSaveFlag = yas::file | yas::binary | yas::no_header;
            std::filesystem::path save_party(process.fatDeviceCPP + "_nds/SkyjoDS/save_party.dat");
            if ( std::filesystem::exists(save_party) && std::filesystem::is_regular_file(save_party))
            {
                std::filesystem::remove(save_party);
            }
            yas::file_ostream yasSaveOutput(save_party.c_str());
            // yas has no serializer for std::filesystem::path; serialize a string.
            //std::string test_payload = test_file.string();
            yas::save<yasSaveFlag>(yasSaveOutput, gameparty);
            yasSaveOutput.flush();
            
        } 
    }
}
void GameParty::GamePartyLogicRender()
{
    NEA_2DViewInit();

    if (this->StartMenu)
    {
        if (this->pausephase == PausePhase::PauseMenuMain)
        {
            NEA_RichTextRender3D(0, "PAUSE", 102, 35);
        }
        else if (this->pausephase == PausePhase::QuitMenu)
        {
            NEA_RichTextRender3D(0, "ARE YOU SURE TO QUIT ?", 52, 35);
        }
        else if (this->pausephase == PausePhase::SaveMenu)
        {
            NEA_RichTextRender3D(0, "ARE YOU SURE TO SAVE AND QUIT ?", 22, 35);
        }
        
        NEA_GUIDraw();
        return;
    }

    if (this->EndMenu)
    {
        NEA_RichTextRender3D(0, "RESULTS", 95, 6);

        // Rank players by final score, lowest first (Skyjo: low score wins).
        std::vector<int> order(this->finalScores.size());
        for (size_t i = 0; i < order.size(); ++i) order.at(i) = static_cast<int>(i);
        std::sort(order.begin(), order.end(),
                  [this](int a, int b) { return this->finalScores.at(a) < this->finalScores.at(b); });

        int y = 26;
        for (size_t r = 0; r < order.size(); ++r)
        {
            int p = order.at(r);
            std::string line = std::to_string(static_cast<int>(r) + 1) + ". " +
                               this->namePlayers.at(p) + "  " +
                               std::to_string(this->finalScores.at(p));
            NEA_RichTextRender3D(0, line.c_str(), 30, y);
            y += 16;
        }

        NEA_GUIDraw();
        return;
    }

    const bool initPhase = (this->phase == GamePhase::InitialReveal);

    // Discarding a stack-drawn card requires flipping a face-down card. When the
    // human holds a stack card but has none left, discarding is not allowed
    // (they must swap), so flag the discard pile as unavailable.
    const bool mustSwap = this->heldCard.has_value() &&
                          this->drawSource == DrawSource::Stack &&
                          this->currentPlayerIndex == 0 &&
                          this->HandFullyRevealed(0);

    NEA_SpriteVisible(this->pullpacketIconNot[0], initPhase);
    NEA_SpriteVisible(this->pullpacketIconNot[1], initPhase || mustSwap);

    NEA_SpriteVisible(this->heldCardSprite, this->heldCard.has_value());

    ++this->animTick;
    this->AnimateHandSprites();

    // Held card gently pulses (scale + brightness) while a decision is pending.
    if (this->heldCard.has_value())
    {
        float wave = sinLerp(degreesToAngle((this->animTick * 6) % 360)) / 4096.0f;
        NEA_SpriteSetScale(this->heldCardSprite, 1.0f + 0.05f * wave);
        NEA_SpriteSetParams(this->heldCardSprite,
                            static_cast<u8>(26 + 5 * wave),
                            this->heldCardSprite->id, this->heldCardSprite->color);
    }


    if (initPhase)
    {
        NEA_RichTextRender3D(0, "Reveal two card \n", 120, 15);
    }
    else if (this->awaitingDiscardReveal && this->currentPlayerIndex == 0)
    {
        NEA_RichTextRender3D(0, "Reveal a card \n", 120, 15);
    }
    else if (mustSwap)
    {
        NEA_RichTextRender3D(0, "Place the card \n", 120, 15);
    }
    else 
    {
        NEA_RichTextRender3D(0, "<L", 10, 3);
        //NEA_RichTextRender3D(0, ("<L"), 10, 3);
        NEA_RichTextRender3D(0, "R>", 230, 1);
        NEA_RichTextRender3D(0, this->namePlayers.at(this->topScreenViewPlayerIdx).c_str(), 120, 2);
    }
    NEA_SpriteDrawAll();


}

void GameParty::InitPauseMenuGUIbutton()
{
    for (int i = 0; i < 12; ++i)
    {
        NEA_Hw2DOBJSetVisible(this->viewGame[i], false);
    }
    this->ContinueButton = NEA_GUIButtonCreate( 57, 60,
                                                57 + 128, 60 + 32);
    this->QuitButton =  NEA_GUIButtonCreate( 57, 103,
                                            57 + 128, 103 + 32);
    this->SaveButton =  NEA_GUIButtonCreate( 57, 146,
                                            57 + 128, 146 + 32);
    NEA_GUIButtonConfig(this->ContinueButton,
         this->ContinueButtonMat, NEA_White, 31,
        this->ContinueButtonPressedMat, NEA_White, 31);
    
    NEA_GUIButtonConfig(this->QuitButton,
         this->QuitButtonMat, NEA_White, 31,
        this->QuitButtonPressedMat, NEA_White, 31);

    NEA_GUIButtonConfig(this->SaveButton,
        this->SaveButtonMat, NEA_White, 31,
        this->SaveButtonPressedMat, NEA_White, 31);
}

void GameParty::ComputeFinalScores()
{
    // Each non-cleared slot scores its card value (CardType values ARE the
    // points). Cleared columns are worth 0.
    this->finalScores.assign(this->playerCount, 0);
    for (int p = 0; p < this->playerCount; ++p)
        for (int i = 0; i < 12; ++i)
            if (this->cardReturns.at(p).at(i) != CardReturn::Cleared)
                this->finalScores.at(p) += static_cast<int>(this->playerDeck.at(p).at(i));

    // Skyjo penalty: the player who closed the round (revealed their whole hand
    // first) has their total doubled if they did not finish strictly lowest.
    // Doubling only applies to a positive total.
    if (this->lastRoundTriggerPlayerIndex)
    {
        int t = *this->lastRoundTriggerPlayerIndex;
        bool strictlyLowest = true;
        for (int q = 0; q < this->playerCount; ++q)
            if (q != t && this->finalScores.at(q) <= this->finalScores.at(t))
                strictlyLowest = false;
        if (!strictlyLowest && this->finalScores.at(t) > 0)
            this->finalScores.at(t) *= 2;
    }
}

void GameParty::InitEndGameMenu()
{
    for (int i = 0; i < 12; ++i)
        NEA_Hw2DOBJSetVisible(this->viewGame[i], false);

    this->ReplayButton = NEA_GUIButtonCreate( 5, 160,
                                             5 + 64, 160 + 32);
    this->ExitButton =  NEA_GUIButtonCreate( 190, 160,
                                            190 + 64, 160 + 32);

    NEA_GUIButtonConfig(this->ReplayButton,
         this->ReplayButtonMat, NEA_White, 31,
        this->ReplayButtonPressedMat, NEA_White, 31);

    NEA_GUIButtonConfig(this->ExitButton,
         this->ExitButtonMat, NEA_White, 31,
        this->ExitButtonPressedMat, NEA_White, 31);
    

}

void GameParty::DestroyEndGameMenu()
{
    NEA_GUIDeleteObject(this->ReplayButton);
    NEA_GUIDeleteObject(this->ExitButton);
    //NEA_GUIDeleteObject(this->SaveButton);
}

void GameParty::EndGameGUIlogic()
{
    if (NEA_GUIObjectGetEvent(this->ReplayButton) == NEA_Clicked)
    {
        this->DestroyEndGameMenu();
        this->UnloadGamePartyAssets();
        // Re-enter a fresh party with the same settings. This sets process
        // classstates=Init / menustates=PartyGameOnePlayer and stores the args.
        process.CallInitializationOnePlayerParty(this->playerCount, this->cpuLevel);
        this->Restarted = true;
    }

    if (NEA_GUIObjectGetEvent(this->ExitButton) == NEA_Clicked)
    {
        this->DestroyEndGameMenu();
        this->UnloadGamePartyAssets();
        this->Quited = true;
        mainmenu.mainmenustates = MainMenuStates::OnePlayerPartyStart;
        mainmenu.bypassableChangeMenuStates = true;
    }
}

void GameParty::UnloadGamePartyAssets()
{
    // Tear down in reverse dependency order: live instances (sprites + OBJs)
    // must go before the assets they reference. NEA_Hw2DOBJAssetDelete() is a
    // silent no-op while any OBJ is still bound, so deleting the viewGame[]
    // instances first is what actually frees the card OBJ assets from VRAM.
    // Leaving them alive also let MainMenu's HW2D update walk dangling OBJs
    // after teardown -> data abort.
    NEA_SpriteDeleteAll();
    //NEA_Hw2DBGDelete(this->hexBGbot);
    //NEA_Hw2DBGDelete(this->hexBGtop);

    for (int i = 0; i < 12; ++i)
        NEA_Hw2DOBJDelete(this->viewGame[i]);

    NEA_MaterialDelete(this->SaveButtonMat);
    NEA_MaterialDelete(this->SaveButtonPressedMat);

    NEA_MaterialDelete(this->ContinueButtonMat);
    NEA_MaterialDelete(this->ContinueButtonPressedMat);

    NEA_MaterialDelete(this->QuitButtonMat);
    NEA_MaterialDelete(this->QuitButtonPressedMat);

    NEA_MaterialDelete(this->YesButtonMat);
    NEA_MaterialDelete(this->YesButtonPressedMat);

    NEA_MaterialDelete(this->NoButtonMat);
    NEA_MaterialDelete(this->NoButtonPressedMat);

    NEA_MaterialDelete(this->ReplayButtonMat);
    NEA_MaterialDelete(this->ReplayButtonPressedMat);

    NEA_MaterialDelete(this->ExitButtonMat);
    NEA_MaterialDelete(this->ExitButtonPressedMat);


    NEA_PaletteDelete(this->ContinueButtonPal);
    NEA_PaletteDelete(this->ContinueButtonPressedPal);

    NEA_PaletteDelete(this->QuitButtonPal);
    NEA_PaletteDelete(this->QuitButtonPressedPal);

    NEA_PaletteDelete(this->YesButtonPal);
    NEA_PaletteDelete(this->YesButtonPressedPal);

    NEA_PaletteDelete(this->NoButtonPal);
    NEA_PaletteDelete(this->NoButtonPressedPal);

    NEA_PaletteDelete(this->ReplayButtonPal);
    NEA_PaletteDelete(this->ReplayButtonPressedPal);

    NEA_PaletteDelete(this->SaveButtonPal);
    NEA_PaletteDelete(this->SaveButtonPressedPal);

    NEA_PaletteDelete(this->ExitButtonPal);
    NEA_PaletteDelete(this->ExitButtonPressedPal);

    for (int n = static_cast<int>(CardType::Negative_2); n <= static_cast<int>(CardType::Positive_12); ++n)
    {
        CardType i = static_cast<CardType>(n);

        NEA_MaterialDelete(sharedAssetsGameParty.GetCardMat(i));
        NEA_PaletteDelete(sharedAssetsGameParty.GetCardPal(i));
        NEA_Hw2DOBJAssetDelete(sharedAssetsGameParty.GetCardOBJ(i));

    }

    NEA_MaterialDelete(sharedAssetsGameParty.GetCardMat(std::nullopt));
    NEA_PaletteDelete(sharedAssetsGameParty.GetCardPal(std::nullopt));
    NEA_Hw2DOBJAssetDelete(sharedAssetsGameParty.GetCardOBJ(std::nullopt));

    NEA_MaterialDelete(this->NotPossibleIconMat);
    NEA_PaletteDelete(this->NotPossibleIconPal);
}

void GameParty::LoadGamePartyAssets()
{   if (!(NEA_Hw2DGetClaimedBanks() & NEA_VRAM_D))
    {
        std::terminate();
    }
    /*
    this->hexBGtop = NEA_Hw2DBGCreate(NEA_ENGINE_MAIN, 1,
                                       NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->hexBGtop, 3);

    NEA_Hw2DBGLoadGRFFAT(this->hexBGtop, "mainmenu/hex_background2_png.grf", 0);
    NEA_Hw2DBGSetVisible(this->hexBGtop, true);

    this->hexBGbot = NEA_Hw2DBGCreate(NEA_ENGINE_SUB, 0,
                                       NEA_HW2D_BG_TILED_8BPP, 256, 256);
    NEA_Hw2DBGSetPriority(this->hexBGbot, 3);
    NEA_Hw2DBGLoadGRFFAT(this->hexBGbot, "mainmenu/hex_background_png.grf", 1);

    NEA_Hw2DBGSetVisible(this->hexBGbot, true);
    */
    this->ContinueButtonMat = NEA_MaterialCreate();
    this->ContinueButtonPal = NEA_PaletteCreate();
    this->ContinueButtonPressedMat = NEA_MaterialCreate();
    this->ContinueButtonPressedPal = NEA_PaletteCreate();

    this->QuitButtonMat = NEA_MaterialCreate();
    this->QuitButtonPal = NEA_PaletteCreate();
    this->QuitButtonPressedMat = NEA_MaterialCreate();
    this->QuitButtonPressedPal = NEA_PaletteCreate();

    this->YesButtonMat = NEA_MaterialCreate();
    this->YesButtonPal = NEA_PaletteCreate();
    this->YesButtonPressedMat = NEA_MaterialCreate();
    this->YesButtonPressedPal = NEA_PaletteCreate();

    this->NoButtonMat = NEA_MaterialCreate();
    this->NoButtonPal = NEA_PaletteCreate();
    this->NoButtonPressedMat = NEA_MaterialCreate();
    this->NoButtonPressedPal = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->ContinueButtonMat,
                            this->ContinueButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/ResumeButton_png.grf");

    NEA_MaterialTexLoadGRF(this->ContinueButtonPressedMat,
                            this->ContinueButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/ResumeButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->QuitButtonMat,
                            this->QuitButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/QuitButton_png.grf");

    NEA_MaterialTexLoadGRF(this->QuitButtonPressedMat,
                            this->QuitButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/QuitButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->NoButtonMat,
                            this->NoButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/NoButton_png.grf");

    NEA_MaterialTexLoadGRF(this->NoButtonPressedMat,
                            this->NoButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/NoButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->YesButtonMat,
                            this->YesButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/YesButton_png.grf");

    NEA_MaterialTexLoadGRF(this->YesButtonPressedMat,
                            this->YesButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/YesButtonPressed_png.grf");

    this->ReplayButtonMat = NEA_MaterialCreate();
    this->ReplayButtonPal = NEA_PaletteCreate();
    this->ReplayButtonPressedMat = NEA_MaterialCreate();
    this->ReplayButtonPressedPal = NEA_PaletteCreate();

    this->ExitButtonMat = NEA_MaterialCreate();
    this->ExitButtonPal = NEA_PaletteCreate();
    this->ExitButtonPressedMat = NEA_MaterialCreate();
    this->ExitButtonPressedPal = NEA_PaletteCreate();

    this->SaveButtonMat = NEA_MaterialCreate();
    this->SaveButtonPal = NEA_PaletteCreate();
    this->SaveButtonPressedMat = NEA_MaterialCreate();
    this->SaveButtonPressedPal = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->ReplayButtonMat,
                            this->ReplayButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/ReplayButton_png.grf");

    NEA_MaterialTexLoadGRF(this->ReplayButtonPressedMat,
                            this->ReplayButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/ReplayButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->ExitButtonMat,
                            this->ExitButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/ExitButton_png.grf");

    NEA_MaterialTexLoadGRF(this->ExitButtonPressedMat,
                            this->ExitButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/ExitButtonPressed_png.grf");
    
    NEA_MaterialTexLoadGRF(this->SaveButtonMat,
                            this->SaveButtonPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/SaveButton_png.grf");

    NEA_MaterialTexLoadGRF(this->SaveButtonPressedMat,
                            this->SaveButtonPressedPal,
                            NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/SaveButtonPressed_png.grf");
    for (int n = static_cast<int>(CardType::Negative_2); n <= static_cast<int>(CardType::Positive_12); ++n)
    {
        CardType i = static_cast<CardType>(n);

        sharedAssetsGameParty.GetCardMat(i) = NEA_MaterialCreate();
        sharedAssetsGameParty.GetCardPal(i) = NEA_PaletteCreate();
        sharedAssetsGameParty.GetCardOBJ(i) = NEA_Hw2DOBJAssetCreate(NEA_ENGINE_SUB, NEA_OBJ_SIZE_32x64, NEA_OBJ_COLOR_16);

        // AssetLoadGRFFAT auto-allocates a 16-color palette slot and loads the
        // palette into it. Don't override the slot afterwards: SetPaletteSlot
        // re-points the slot number but does NOT move the palette data, leaving
        // the asset pointing at an empty bank (renders fully black).
        NEA_Hw2DOBJAssetLoadGRFFAT(sharedAssetsGameParty.GetCardOBJ(i),
                                    sharedAssetsGameParty.GetHwCardGRFpath(i).c_str());

        NEA_MaterialTexLoadGRF(sharedAssetsGameParty.GetCardMat(i),
                                 sharedAssetsGameParty.GetCardPal(i),
                                  NEA_TEXGEN_TEXCOORD, sharedAssetsGameParty.GetCardGRFpath(i).c_str());
    }

    sharedAssetsGameParty.GetCardMat(std::nullopt) = NEA_MaterialCreate();
    sharedAssetsGameParty.GetCardPal(std::nullopt) = NEA_PaletteCreate();
    sharedAssetsGameParty.GetCardOBJ(std::nullopt) = NEA_Hw2DOBJAssetCreate(NEA_ENGINE_SUB, NEA_OBJ_SIZE_32x64, NEA_OBJ_COLOR_16);

    NEA_Hw2DOBJAssetLoadGRFFAT(sharedAssetsGameParty.GetCardOBJ(std::nullopt),
                                sharedAssetsGameParty.GetHwCardGRFpath(std::nullopt).c_str());
    NEA_MaterialTexLoadGRF(sharedAssetsGameParty.GetCardMat(std::nullopt),
                            sharedAssetsGameParty.GetCardPal(std::nullopt),
                            NEA_TEXGEN_TEXCOORD, sharedAssetsGameParty.GetCardGRFpath(std::nullopt).c_str());

    this->NotPossibleIconMat = NEA_MaterialCreate();
    this->NotPossibleIconPal = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->NotPossibleIconMat, this->NotPossibleIconPal, NEA_TEXGEN_TEXCOORD, "ingame/clear_png.grf");
}

void GameParty::InitCardStack()
{
    this->cardPreStack.clear();
    this->cardStack.clear();
    this->cardPreStack.reserve(150);
    this->cardStack.reserve(150);

    for (auto const& item : cardPackage)
    {
        CardType typeToAdd = item.first;
        uint8_t iterateTimes = item.second;
        for (uint8_t i = 0; i < iterateTimes; i++)
            this->cardPreStack.push_back(typeToAdd);
    }

    this->cardStack = this->cardPreStack;
    std::mt19937 rng{static_cast<std::mt19937::result_type>(time(nullptr))};
    std::shuffle(this->cardStack.begin(), this->cardStack.end(), rng);

    this->cardPreStack.clear();
}

void GameParty::BuildControllers(int n)
{
    this->controllers.clear();
    this->controllers.reserve(n);

    switch (this->partyType)
    {
        case PartyType::OnePlayerCPU:
            this->controllers.push_back(std::make_unique<HumanTouchController>());
            for (int i = 1; i < n; ++i)
                this->controllers.push_back(std::make_unique<CpuController>(this->cpuLevel));
            break;

        case PartyType::LocalMultiplayer:
        case PartyType::OnlineMultiplayer:
            // TODO: route remote/multi-pad input through dedicated controllers.
            for (int i = 0; i < n; ++i)
                this->controllers.push_back(std::make_unique<HumanTouchController>());
            break;
    }
}

void GameParty::InitGamePartySituation(int number_arg, CPULevel cpu_arg, PartyType party_arg)
{
    this->cpuLevel = cpu_arg;
    this->partyType = party_arg;
    this->playerCount = number_arg;
    this->Quited = false;
    this->EndMenu = false;
    this->Restarted = false;
    this->finalScores.clear();
    this->partyFirstTwoDraw = true;
    this->StartMenu = false;
    this->awaitingDiscardReveal = false;
    this->phase = GamePhase::InitialReveal;
    this->animTick = 0;
    for (int i = 0; i < 12; ++i) { this->popTimer[i] = 0; this->clearTimer[i] = 0; }
    this->currentPlayerIndex = 0;
    this->startingPlayerIndex = 0;
    this->lastRoundTriggerPlayerIndex = std::nullopt;
    this->drawSource = std::nullopt;
    this->heldCard = std::nullopt;
    this->topScreenViewPlayerIdx = (number_arg > 1) ? 1 : 0;
    this->prevKeydown = 0;
    this->initialRevealCount.fill(0);

    this->LoadGamePartyAssets();
    this->InitCardStack();
    this->playerDeck.resize(number_arg);

    std::array<CardReturn, 12> unreturnedRow;
    unreturnedRow.fill(CardReturn::Unreturned);
    this->cardReturns.assign(number_arg, unreturnedRow);

    /*
    NEA_Hw2DOBJAsset *tempTEST = NEA_Hw2DOBJAssetCreate(NEA_ENGINE_SUB, NEA_OBJ_SIZE_64x32,  NEA_OBJ_COLOR_256);
    NEA_Hw2DOBJAssetLoadGRFFAT(tempTEST, "mainmenu/btns/DisplayNAME_png.grf");
    NEA_Hw2DOBJ *testTEMPCOPY = NEA_Hw2DOBJCreateFromAsset(tempTEST);
    NEA_Hw2DOBJSetPriority(testTEMPCOPY, 3);
    NEA_Hw2DOBJSetPos(testTEMPCOPY, 5, 5);
    NEA_Hw2DOBJSetVisible(testTEMPCOPY, true);
    */
    this->namePlayers.resize(number_arg);
    this->namePlayers.at(0) = process.consoleUserName;

    std::vector<std::string> cpuPool(CPUnames.begin(), CPUnames.end());
    std::mt19937 rngName{static_cast<std::mt19937::result_type>(time(nullptr))};
    std::shuffle(cpuPool.begin(), cpuPool.end(), rngName);

    for (int i = 1; i < number_arg; ++i)
        this->namePlayers.at(i) = cpuPool.at(i - 1);

    for (auto& hand : this->playerDeck)
    {
        auto first = this->cardStack.end() - 12;
        std::copy(first, this->cardStack.end(), hand.begin());
        this->cardStack.erase(first, this->cardStack.end());
    }

    this->discardPile.clear();
    if (!this->cardStack.empty())
    {
        this->discardPile.push_back(this->cardStack.back());
        this->cardStack.pop_back();
    }

    int x = 112;
    int y = 25;
    for (size_t i = 0; i < 12; i++)
    {
        this->myPacket[i] = NEA_SpriteCreate();
        NEA_SpriteSetMaterial(this->myPacket[i], sharedAssetsGameParty.GetCardMat(std::nullopt));
        NEA_SpriteSetPos(this->myPacket[i], x, y);
        this->MyCardPos.at(i) = {
            .x_min = x,
            .x_max = x + 24,
            .y_min = y,
            .y_max = y + 36,
        };
        x += 28;
        if (((i + 1) % 4 == 0) && ( i != 0))
        {
            y += 40;
            x = 112;
        }
    }

    x = 67;
    y = 32;

    for (size_t i = 0; i < 12; i++)
    {
        this->viewGame[i] = NEA_Hw2DOBJCreateFromAsset(sharedAssetsGameParty.GetCardOBJ(std::nullopt));
        NEA_Hw2DOBJSetPos(this->viewGame[i], x, y);
        NEA_Hw2DOBJSetVisible(this->viewGame[i], true);
        x += 28;
        if (((i + 1) % 4 == 0) && ( i != 0))
        {
            y += 40;
            x = 67;
        }
    }

    this->pullpacket[0] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacket[0], sharedAssetsGameParty.GetCardMat(std::nullopt));
    NEA_SpriteSetPos(this->pullpacket[0], 25, 41);
    NEA_SpriteSetPriority(this->pullpacket[0], 1);

    this->pullpacket[1] = NEA_SpriteCreate();
    NEA_SpriteSetPos(this->pullpacket[1], 25, 81);
    NEA_SpriteSetPriority(this->pullpacket[1], 1);
    this->RefreshDiscardSprite();

    this->pullpacketIconNot[0] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacketIconNot[0], this->NotPossibleIconMat);
    NEA_SpriteSetPos(this->pullpacketIconNot[0], 25, 56);
    NEA_SpriteVisible(this->pullpacketIconNot[0], false);
    NEA_SpriteSetPriority(this->pullpacketIconNot[0], 0);

    this->pullpacketIconNot[1] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacketIconNot[1], this->NotPossibleIconMat);
    NEA_SpriteSetPos(this->pullpacketIconNot[1], 25, 96);
    NEA_SpriteVisible(this->pullpacketIconNot[1], false);
    NEA_SpriteSetPriority(this->pullpacketIconNot[1], 0);

    this->heldCardSprite = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->heldCardSprite, sharedAssetsGameParty.GetCardMat(std::nullopt));
    NEA_SpriteSetPos(this->heldCardSprite, kHeldCardX, kHeldCardY);
    NEA_SpriteSetPriority(this->heldCardSprite, 0);
    NEA_SpriteVisible(this->heldCardSprite, false);

    this->BuildControllers(number_arg);

    setBrightness(3, 0);
}

void GameParty::RefreshMyHandSprite(int slot)
{
    if (this->cardReturns.at(0).at(slot) == CardReturn::Cleared)
    {
        // A clear fade-out owns the sprite until its timer expires.
        if (this->clearTimer[slot] > 0) return;
        NEA_SpriteVisible(this->myPacket[slot], false);
        return;
    }

    std::optional<CardType> mat = std::nullopt;
    if (this->cardReturns.at(0).at(slot) == CardReturn::Returned)
    {
        mat = this->playerDeck.at(0).at(slot);
    }
    NEA_SpriteSetMaterial(this->myPacket[slot], sharedAssetsGameParty.GetCardMat(mat));
    NEA_SpriteVisible(this->myPacket[slot], true);
}

void GameParty::AnimateHandSprites()
{
    // Only ever touch a sprite while it is actively animating, resetting it once
    // when its timer ends. Cards that are not animating are left exactly as
    // RefreshMyHandSprite set them, so an effect on one card never touches others.
    for (int i = 0; i < 12; ++i)
    {
        NEA_Sprite* s = this->myPacket[i];

        if (this->clearTimer[i] > 0)
        {
            // Shrink + fade the matched card, then hide the slot.
            float progress = 1.0f - static_cast<float>(this->clearTimer[i]) / kClearFrames;
            NEA_SpriteSetScale(s, 1.0f - 0.6f * progress);
            NEA_SpriteSetParams(s, static_cast<u8>(31 - 27 * progress), s->id, s->color);
            if (--this->clearTimer[i] == 0)
            {
                NEA_SpriteSetScale(s, 1.0f);
                NEA_SpriteSetParams(s, 31, s->id, s->color);
                NEA_SpriteVisible(s, false);
            }
        }
        else if (this->popTimer[i] > 0)
        {
            // Scale down from a slight overshoot while fading in.
            float progress = 1.0f - static_cast<float>(this->popTimer[i]) / kPopFrames;
            NEA_SpriteSetScale(s, 1.25f - 0.25f * progress);
            NEA_SpriteSetParams(s, static_cast<u8>(18 + 13 * progress), s->id, s->color);
            if (--this->popTimer[i] == 0)
            {
                NEA_SpriteSetScale(s, 1.0f);
                NEA_SpriteSetParams(s, 31, s->id, s->color);
            }
        }
    }
}

void GameParty::RefreshDiscardSprite()
{
    if (this->discardPile.empty())
    {
        NEA_SpriteSetMaterial(this->pullpacket[1],
                              sharedAssetsGameParty.GetCardMat(std::nullopt));
    }
    else
    {
        NEA_SpriteSetMaterial(this->pullpacket[1],
                              sharedAssetsGameParty.GetCardMat(this->discardPile.back()));
    }
}

void GameParty::RefreshTopScreen()
{
    if (this->playerDeck.empty()) return;
    int p = this->topScreenViewPlayerIdx;
    if (p < 0 || p >= this->playerCount) return;
    for (int i = 0; i < 12; ++i)
    {
        if (this->cardReturns.at(p).at(i) == CardReturn::Cleared)
        {
            NEA_Hw2DOBJSetVisible(this->viewGame[i], false);
            continue;
        }
        std::optional<CardType> face = std::nullopt;
        if (this->cardReturns.at(p).at(i) == CardReturn::Returned)
            face = this->playerDeck.at(p).at(i);
        NEA_Hw2DOBJBindAsset(this->viewGame[i],
                             sharedAssetsGameParty.GetCardOBJ(face));
        NEA_Hw2DOBJSetVisible(this->viewGame[i], true);
    }
}

void GameParty::HandleTopScreenCycling()
{
    if (this->playerCount < 2) return;
    auto cycle = [this](int dir) {
        int idx = this->topScreenViewPlayerIdx;
        for (int guard = 0; guard < this->playerCount; ++guard)
        {
            idx = (idx + dir + this->playerCount) % this->playerCount;
            if (idx != 0) break;
        }
        this->topScreenViewPlayerIdx = idx;
        this->RefreshTopScreen();
    };
    if (this->keydown & KEY_L) cycle(-1);
    if (this->keydown & KEY_R) cycle(+1);
}

void GameParty::TickInitialReveal()
{
    for (int p = 0; p < this->playerCount; ++p)
    {
        if (this->initialRevealCount[p] >= 2) continue;
        auto slot = this->controllers[p]->ChooseInitialReveal(*this, p);
        if (!slot) continue;
        if (this->cardReturns.at(p).at(*slot) == CardReturn::Returned) continue;

        this->cardReturns.at(p).at(*slot) = CardReturn::Returned;
        this->initialRevealCount[p]++;

        if (p == 0) { this->RefreshMyHandSprite(*slot); this->popTimer[*slot] = kPopFrames; }
        if (p == this->topScreenViewPlayerIdx) this->RefreshTopScreen();
    }

    bool allDone = true;
    for (int p = 0; p < this->playerCount; ++p)
        if (this->initialRevealCount[p] < 2) { allDone = false; break; }
    if (!allDone) return;

    int bestSum = INT32_MIN;
    int bestIdx = 0;
    for (int p = 0; p < this->playerCount; ++p)
    {
        int s = 0;
        for (int i = 0; i < 12; ++i)
            if (this->cardReturns.at(p).at(i) == CardReturn::Returned)
                s += static_cast<int>(this->playerDeck.at(p).at(i));
        if (s > bestSum) { bestSum = s; bestIdx = p; }
    }
    this->startingPlayerIndex = bestIdx;
    this->currentPlayerIndex = bestIdx;
    this->partyFirstTwoDraw = false;
    this->phase = GamePhase::Turns;
}

void GameParty::TickTurn()
{
    auto& ctrl = *this->controllers[this->currentPlayerIndex];
    int p = this->currentPlayerIndex;

    // The player discarded a stack-drawn card and now must reveal one of their
    // own face-down cards before the turn can end.
    if (this->awaitingDiscardReveal)
    {
        bool hasFaceDown = false;
        for (int i = 0; i < 12; ++i)
            if (this->cardReturns.at(p).at(i) == CardReturn::Unreturned) { hasFaceDown = true; break; }

        if (hasFaceDown)
        {
            auto slot = ctrl.ChooseInitialReveal(*this, p);
            if (!slot) return;
            if (this->cardReturns.at(p).at(*slot) != CardReturn::Unreturned) return;
            this->cardReturns.at(p).at(*slot) = CardReturn::Returned;
            if (p == 0) { this->RefreshMyHandSprite(*slot); this->popTimer[*slot] = kPopFrames; }
            if (p == this->topScreenViewPlayerIdx) this->RefreshTopScreen();
            this->ResolveColumnClears(p);
        }

        this->awaitingDiscardReveal = false;
        this->EndTurn(p);
        return;
    }

    if (!this->drawSource)
    {
        auto src = ctrl.ChooseDrawSource(*this, this->currentPlayerIndex);
        if (!src) return;
        this->drawSource = src;
        // Fall through and grab the card this same frame: a separate "draw"
        // frame would swallow a fast follow-up tap (keysDown is edge-triggered),
        // making the next action silently fail.
    }

    if (!this->heldCard)
    {
        if (*this->drawSource == DrawSource::Stack)
        {
            if (this->cardStack.empty())
            {
                // TODO: reshuffle discardPile (except top) back into cardStack.
                return;
            }
            this->heldCard = this->cardStack.back();
            this->cardStack.pop_back();
        }
        else
        {
            if (this->discardPile.empty())
            {
                this->drawSource = std::nullopt;
                return;
            }
            this->heldCard = this->discardPile.back();
            this->discardPile.pop_back();
            this->RefreshDiscardSprite();
        }
        NEA_SpriteSetMaterial(this->heldCardSprite,
                              sharedAssetsGameParty.GetCardMat(*this->heldCard));
        return;
    }

    int actedSlot = -1;

    if (*this->drawSource == DrawSource::Stack)
    {
        auto act = ctrl.ChooseStackAction(*this, p, *this->heldCard);
        if (!act) return;
        if (act->kind == StackAction::Kind::Replace)
        {
            CardType oldCard = this->playerDeck.at(p).at(act->slot);
            this->playerDeck.at(p).at(act->slot) = *this->heldCard;
            this->cardReturns.at(p).at(act->slot) = CardReturn::Returned;
            this->discardPile.push_back(oldCard);
            actedSlot = act->slot;
        }
        else
        {
            // Discard the drawn card straight onto the discard pile. The forced
            // reveal is handled next tick via the awaitingDiscardReveal branch.
            this->discardPile.push_back(*this->heldCard);
            this->heldCard = std::nullopt;
            this->awaitingDiscardReveal = true;
            this->RefreshDiscardSprite();
            return;
        }
    }
    else
    {
        auto slot = ctrl.ChooseDiscardReplaceSlot(*this, p, *this->heldCard);
        if (!slot) return;
        CardType oldCard = this->playerDeck.at(p).at(*slot);
        this->playerDeck.at(p).at(*slot) = *this->heldCard;
        this->cardReturns.at(p).at(*slot) = CardReturn::Returned;
        this->discardPile.push_back(oldCard);
        actedSlot = *slot;
    }

    if (p == 0) { this->RefreshMyHandSprite(actedSlot); this->popTimer[actedSlot] = kPopFrames; }
    if (p == this->topScreenViewPlayerIdx) this->RefreshTopScreen();
    this->RefreshDiscardSprite();
    this->ResolveColumnClears(p);
    this->EndTurn(p);
}

void GameParty::EndTurn(int p)
{
    if (this->HandFullyRevealed(p) && !this->lastRoundTriggerPlayerIndex)
    {
        this->lastRoundTriggerPlayerIndex = p;
        this->phase = GamePhase::LastRound;
    }

    int nextIdx = (this->currentPlayerIndex + 1) % this->playerCount;
    this->drawSource = std::nullopt;
    this->heldCard = std::nullopt;

    if (this->phase == GamePhase::LastRound &&
        this->lastRoundTriggerPlayerIndex &&
        nextIdx == *this->lastRoundTriggerPlayerIndex)
    {
        this->phase = GamePhase::Scoring;
        return;
    }

    this->currentPlayerIndex = nextIdx;
    if (nextIdx != 0)
    {
        this->topScreenViewPlayerIdx = nextIdx;
        this->RefreshTopScreen();
    }
}

void GameParty::TickScoring()
{
    // Flip every remaining face-down card, then wait for an input to tally the
    // scores and open the end-game leaderboard.
    for (int p = 0; p < this->playerCount; ++p)
        for (int i = 0; i < 12; ++i)
            if (this->cardReturns.at(p).at(i) != CardReturn::Cleared)
                this->cardReturns.at(p).at(i) = CardReturn::Returned;

    for (int i = 0; i < 12; ++i) this->RefreshMyHandSprite(i);
    this->RefreshTopScreen();

    if (this->keydown & (KEY_A | KEY_B | KEY_START | KEY_TOUCH))
    {
        this->phase = GamePhase::Ended;
        this->ComputeFinalScores();
        this->EndMenu = true;
        this->InitEndGameMenu();
    }
}

void GameParty::ResolveColumnClears(int playerIdx)
{
    // Skyjo column-clear rule: when the 3 cards of a vertical column are all
    // revealed and identical, the whole column is discarded and the slots are
    // emptied. Hand layout is 4 columns x 3 rows; slot i has column = i % 4,
    // row = i / 4, so column c is slots {c, c+4, c+8}.
    auto& ret = this->cardReturns.at(playerIdx);
    auto& deck = this->playerDeck.at(playerIdx);

    for (int c = 0; c < 4; ++c)
    {
        int a = c, b = c + 4, d = c + 8;
        if (ret.at(a) != CardReturn::Returned ||
            ret.at(b) != CardReturn::Returned ||
            ret.at(d) != CardReturn::Returned)
            continue;
        if (deck.at(a) != deck.at(b) || deck.at(b) != deck.at(d))
            continue;

        this->discardPile.push_back(deck.at(a));
        this->discardPile.push_back(deck.at(b));
        this->discardPile.push_back(deck.at(d));

        ret.at(a) = CardReturn::Cleared;
        ret.at(b) = CardReturn::Cleared;
        ret.at(d) = CardReturn::Cleared;

        if (playerIdx == 0)
        {
            // Start the fade-out: keep the matched face visible and let
            // AnimateHandSprites shrink/fade it before hiding the slot.
            for (int s : {a, b, d})
            {
                NEA_SpriteSetMaterial(this->myPacket[s],
                                      sharedAssetsGameParty.GetCardMat(deck.at(s)));
                NEA_SpriteVisible(this->myPacket[s], true);
                this->clearTimer[s] = kClearFrames;
                this->popTimer[s] = 0;
            }
        }
        if (playerIdx == this->topScreenViewPlayerIdx)
            this->RefreshTopScreen();
        this->RefreshDiscardSprite();
    }
}

bool GameParty::HandFullyRevealed(int playerIdx) const
{
    for (const auto& r : this->cardReturns.at(playerIdx))
        if (r == CardReturn::Unreturned) return false;
    return true;
}

void GameParty::AdvanceToNextPlayer()
{
    // Reserved for future use; turn advancement is currently inlined in TickTurn.
}

void GameParty::RenderGameParty()
{
    while (1)
    {
        // The pause menu and the end-game leaderboard are both touch overlays:
        // they need the GUI updated and they suppress gameplay input/rendering.
        const bool overlay = this->StartMenu || this->EndMenu;
        if (overlay)
        {
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D | NEA_UPDATE_GUI));
            if (this->StartMenu)
                this->PauseMenuGUIlogic();
            else
                this->EndGameGUIlogic();

            if (this->Quited)
            {
                process.classstates = ClassStates::Init;
                process.menustates = MenusStates::MainMenu;
                break;
            }
            // Replay: CallInitializationOnePlayerParty already set the process
            // states to re-init a fresh party; just leave the render loop.
            if (this->Restarted)
                break;
        }
        else
        {
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
        }

        scanKeys();
        this->keydown = keysDown();
        touchRead(&this->touchData);
        if (!overlay)
        {
            if (this->keydown & KEY_START)
            {
                this->pausephase = PausePhase::PauseMenuMain;
                this->StartMenu = true;
                this->InitPauseMenuGUIbutton();
            }
            else
            {
                this->GamePartyLogic();
            }
        }



        NEA_Process([](){
            gameparty.GamePartyLogicRender();
        });
    }
}
GameParty gameparty;
