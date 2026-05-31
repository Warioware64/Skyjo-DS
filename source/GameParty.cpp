#include "GameParty.hpp"
#include "globalHeader.hpp"
#include "GamePartyClasses/HumanTouchController.hpp"
#include "GamePartyClasses/CpuController.hpp"


namespace
{
    constexpr int kHeldCardX = 60;
    constexpr int kHeldCardY = 60;
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

void GameParty::GamePartyLogicRender()
{
    NEA_2DViewInit();

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
    NEA_SpriteDrawAll();
}

void GameParty::LoadGamePartyAssets()
{   if (!(NEA_Hw2DGetClaimedBanks() & NEA_VRAM_D))
    {
        std::terminate();
    }

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
    this->partyFirstTwoDraw = true;
    this->awaitingDiscardReveal = false;
    this->phase = GamePhase::InitialReveal;
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

    x = 12;
    y = 12;

    for (size_t i = 0; i < 12; i++)
    {
        this->viewGame[i] = NEA_Hw2DOBJCreateFromAsset(sharedAssetsGameParty.GetCardOBJ(std::nullopt));
        NEA_Hw2DOBJSetPos(this->viewGame[i], x, y);
        NEA_Hw2DOBJSetVisible(this->viewGame[i], true);
        x += 28;
        if (((i + 1) % 4 == 0) && ( i != 0))
        {
            y += 40;
            x = 12;
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

        if (p == 0) this->RefreshMyHandSprite(*slot);
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
            if (p == 0) this->RefreshMyHandSprite(*slot);
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
        if (src) this->drawSource = src;
        return;
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

    if (p == 0) this->RefreshMyHandSprite(actedSlot);
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
    // TODO: full scoring screen (per-hand totals, doubling rule when the
    //       trigger player is not strictly lowest). Skeleton just flips
    //       everything and waits for an input to end the game.
    for (int p = 0; p < this->playerCount; ++p)
        for (int i = 0; i < 12; ++i)
            if (this->cardReturns.at(p).at(i) != CardReturn::Cleared)
                this->cardReturns.at(p).at(i) = CardReturn::Returned;

    for (int i = 0; i < 12; ++i) this->RefreshMyHandSprite(i);
    this->RefreshTopScreen();

    if (this->keydown & (KEY_A | KEY_B | KEY_START | KEY_TOUCH))
        this->phase = GamePhase::Ended;
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
            this->RefreshMyHandSprite(a);
            this->RefreshMyHandSprite(b);
            this->RefreshMyHandSprite(d);
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
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
        scanKeys();
        this->keydown = keysDown();
        touchRead(&this->touchData);
        this->GamePartyLogic();

        NEA_Process([](){
            gameparty.GamePartyLogicRender();
        });
    }
}
GameParty gameparty;
