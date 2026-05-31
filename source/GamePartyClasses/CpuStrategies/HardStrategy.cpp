#include "../CpuController.hpp"
#include "../../GameParty.hpp"
#include "StrategyCommon.hpp"

// Hard plays the mature Medium policy and adds fair end-of-round timing: using
// only revealed information, when it is clearly ahead and nearly done it pushes
// to reveal its last cards and end the round; otherwise it just minimizes score.
namespace HardStrategy
{
    using namespace StrategyUtil;

    namespace
    {
        // True when we have a small, finishable hand and our (revealed-only)
        // estimate is no worse than every opponent's.
        bool ShouldPushToFinish(const GameParty& g, int self)
        {
            if (UnrevealedCount(g, self) > 2) return false;

            int selfTotal = EstimatedTotal(g, self);
            int playerCount = static_cast<int>(g.playerDeck.size());
            for (int p = 0; p < playerCount; ++p)
            {
                if (p == self) continue;
                if (EstimatedTotal(g, p) < selfTotal) return false;
            }
            return true;
        }
    }

    int ChooseInitialReveal(const GameParty& g, int playerIdx)
    {
        return MediumStrategy::ChooseInitialReveal(g, playerIdx);
    }

    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx)
    {
        return MediumStrategy::ChooseDrawSource(g, playerIdx);
    }

    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
    {
        // Ahead and nearly done: prefer revealing remaining cards so the hand
        // completes and the last round is triggered while we are winning.
        if (ShouldPushToFinish(g, playerIdx))
        {
            if (auto s = ColumnClearSlot(g, playerIdx, drawn))
                return StackAction{StackAction::Kind::Replace, *s};
            // Both reveal one hidden card (advancing toward the finish); keep a
            // good draw by replacing a hidden slot, otherwise discard and flip.
            if (auto u = FirstUnrevealedSlot(g, playerIdx))
            {
                if (Val(drawn) <= 4)
                    return StackAction{StackAction::Kind::Replace, *u};
                return StackAction{StackAction::Kind::FlipOnly, *u};
            }
        }

        return MediumStrategy::ChooseStackAction(g, playerIdx, drawn);
    }

    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
    {
        return MediumStrategy::ChooseDiscardReplaceSlot(g, playerIdx, drawn);
    }
}
