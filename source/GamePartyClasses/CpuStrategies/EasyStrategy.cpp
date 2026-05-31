#include "../CpuController.hpp"
#include "../../GameParty.hpp"
#include "StrategyCommon.hpp"

// Easy is the naive floor: it reasons only about its own cards with loose
// thresholds, reveals cards in order, rarely takes the discard, and does no
// planning beyond finishing a column that is already two-thirds matched.
namespace EasyStrategy
{
    using namespace StrategyUtil;

    namespace { constexpr int kKeepThreshold = 5; }

    int ChooseInitialReveal(const GameParty& g, int playerIdx)
    {
        if (auto u = FirstUnrevealedSlot(g, playerIdx)) return *u;
        return 0;
    }

    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx)
    {
        if (g.discardPile.empty()) return DrawSource::Stack;

        CardType top = g.discardPile.back();
        if (ColumnClearSlot(g, playerIdx, top)) return DrawSource::Discard;
        if (Val(top) <= 2) return DrawSource::Discard;
        return DrawSource::Stack;
    }

    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
    {
        if (auto s = ColumnClearSlot(g, playerIdx, drawn))
            return StackAction{StackAction::Kind::Replace, *s};

        auto worst = HighestRevealedSlot(g, playerIdx);

        if (Val(drawn) <= kKeepThreshold)
        {
            if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst)))
                return StackAction{StackAction::Kind::Replace, *worst};
            if (auto u = FirstUnrevealedSlot(g, playerIdx))
                return StackAction{StackAction::Kind::Replace, *u};
        }

        if (auto u = FirstUnrevealedSlot(g, playerIdx))
            return StackAction{StackAction::Kind::FlipOnly, *u};

        if (worst) return StackAction{StackAction::Kind::Replace, *worst};
        if (auto s = FirstNonClearedSlot(g, playerIdx))
            return StackAction{StackAction::Kind::Replace, *s};
        return StackAction{StackAction::Kind::Replace, 0};
    }

    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
    {
        if (auto s = ColumnClearSlot(g, playerIdx, drawn)) return *s;

        auto worst = HighestRevealedSlot(g, playerIdx);
        if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst))) return *worst;

        if (auto u = FirstUnrevealedSlot(g, playerIdx)) return *u;
        if (worst) return *worst;
        if (auto s = FirstNonClearedSlot(g, playerIdx)) return *s;
        return 0;
    }
}
