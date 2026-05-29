#include "../CpuController.hpp"
#include "../../GameParty.hpp"

namespace EasyStrategy
{
    int ChooseInitialReveal(const GameParty& g, int playerIdx)
    {
        const auto& ret = g.cardReturns.at(playerIdx);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Unreturned) return i;
        return 0;
    }

    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx)
    {
        (void)g; (void)playerIdx;
        // TODO Easy: always pull from the stack.
        return DrawSource::Stack;
    }

    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
    {
        (void)drawn;
        // TODO Easy: flip the first unrevealed slot, keep the drawn card discarded.
        const auto& ret = g.cardReturns.at(playerIdx);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Unreturned)
                return StackAction{StackAction::Kind::FlipOnly, i};
        return StackAction{StackAction::Kind::Replace, 0};
    }

    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
    {
        (void)drawn;
        const auto& ret = g.cardReturns.at(playerIdx);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Unreturned) return i;
        return 0;
    }
}
