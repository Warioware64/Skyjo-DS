#include "../CpuController.hpp"
#include "../../GameParty.hpp"
#include "StrategyCommon.hpp"

// Medium is a mature greedy player: it plans toward column clears, takes the
// discard when it meaningfully helps, keeps a tighter value sense than Easy, and
// opens on two different columns. It does not reason about opponents.
namespace MediumStrategy
{
    using namespace StrategyUtil;

    namespace { constexpr int kKeepThreshold = 4; }

    int ChooseInitialReveal(const GameParty& g, int playerIdx)
    {
        const auto& ret = g.cardReturns.at(playerIdx);

        // Spread the two opening reveals across different columns to seed clears.
        int revealedCol = -1;
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Returned) { revealedCol = i % 4; break; }

        if (revealedCol != -1)
            for (int i = 0; i < 12; ++i)
                if (ret.at(i) == CardReturn::Unreturned && (i % 4) != revealedCol) return i;

        if (auto u = FirstUnrevealedSlot(g, playerIdx)) return *u;
        return 0;
    }

    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx)
    {
        if (g.discardPile.empty()) return DrawSource::Stack;

        CardType top = g.discardPile.back();
        if (ColumnClearSlot(g, playerIdx, top)) return DrawSource::Discard;
        if (Val(top) <= kKeepThreshold) return DrawSource::Discard;

        // Take a mediocre card only if it clearly improves our worst card.
        auto worst = HighestRevealedSlot(g, playerIdx);
        if (worst && Val(top) < Val(g.playerDeck.at(playerIdx).at(*worst)) - 1)
            return DrawSource::Discard;

        return DrawSource::Stack;
    }

    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
    {
        if (auto s = ColumnClearSlot(g, playerIdx, drawn))
            return StackAction{StackAction::Kind::Replace, *s};

        auto worst = HighestRevealedSlot(g, playerIdx);

        if (Val(drawn) <= kKeepThreshold)
        {
            // Prefer building toward a future clear over a generic hidden slot.
            if (auto s = ColumnSetupSlot(g, playerIdx, drawn))
                return StackAction{StackAction::Kind::Replace, *s};
            if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst)))
                return StackAction{StackAction::Kind::Replace, *worst};
            if (auto u = FirstUnrevealedSlot(g, playerIdx))
                return StackAction{StackAction::Kind::Replace, *u};
        }
        else if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst)))
        {
            // High-ish card that still beats our worst revealed card.
            return StackAction{StackAction::Kind::Replace, *worst};
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
        if (auto s = ColumnSetupSlot(g, playerIdx, drawn)) return *s;

        auto worst = HighestRevealedSlot(g, playerIdx);
        if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst))) return *worst;

        if (auto u = FirstUnrevealedSlot(g, playerIdx)) return *u;
        if (worst) return *worst;
        if (auto s = FirstNonClearedSlot(g, playerIdx)) return *s;
        return 0;
    }
}
