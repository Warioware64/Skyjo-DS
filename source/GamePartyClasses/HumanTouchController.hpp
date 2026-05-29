#pragma once

#include "PlayerController.hpp"

class HumanTouchController : public IPlayerController
{
    public:
        std::optional<int>
            ChooseInitialReveal(const GameParty& g, int playerIdx) override;

        std::optional<DrawSource>
            ChooseDrawSource(const GameParty& g, int playerIdx) override;

        std::optional<StackAction>
            ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn) override;

        std::optional<int>
            ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn) override;

    private:
        bool waitingFlipOnlyDecision = false;
};
