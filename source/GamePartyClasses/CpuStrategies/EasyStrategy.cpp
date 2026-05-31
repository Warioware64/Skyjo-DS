#include "../CpuController.hpp"
#include "../../GameParty.hpp"

namespace
{
    // Cards with a value at or below this are worth keeping in hand.
    constexpr int kKeepThreshold = 4;

    int Val(CardType c) { return static_cast<int>(c); }

    // Slot of the player's worst (highest-value) currently revealed card.
    std::optional<int> HighestRevealedSlot(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        const auto& deck = g.playerDeck.at(p);
        std::optional<int> best;
        int bestVal = INT32_MIN;
        for (int i = 0; i < 12; ++i)
        {
            if (ret.at(i) != CardReturn::Returned) continue;
            if (Val(deck.at(i)) > bestVal) { bestVal = Val(deck.at(i)); best = i; }
        }
        return best;
    }

    std::optional<int> FirstUnrevealedSlot(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Unreturned) return i;
        return std::nullopt;
    }

    std::optional<int> FirstNonClearedSlot(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) != CardReturn::Cleared) return i;
        return std::nullopt;
    }

    // If two cards of a column are revealed and equal `drawn`, return the third
    // slot: placing `drawn` there makes the column three-of-a-kind and clears it.
    // Clearing always scores 0, so this is desirable regardless of the value.
    std::optional<int> ColumnClearSlot(const GameParty& g, int p, CardType drawn)
    {
        const auto& ret = g.cardReturns.at(p);
        const auto& deck = g.playerDeck.at(p);
        for (int c = 0; c < 4; ++c)
        {
            int slots[3] = { c, c + 4, c + 8 };
            int matches = 0;
            int third = -1;
            for (int s : slots)
            {
                if (ret.at(s) == CardReturn::Returned && deck.at(s) == drawn)
                    ++matches;
                else
                    third = s;
            }
            if (matches == 2 && third != -1 && ret.at(third) != CardReturn::Cleared)
                return third;
        }
        return std::nullopt;
    }
}

namespace EasyStrategy
{
    int ChooseInitialReveal(const GameParty& g, int playerIdx)
    {
        const auto& ret = g.cardReturns.at(playerIdx);

        // Find the column already touched (if any) so the second reveal lands in
        // a different column, spreading the two opening cards out.
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
        return DrawSource::Stack;
    }

    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
    {
        // 1. Complete and clear a column if we can.
        if (auto s = ColumnClearSlot(g, playerIdx, drawn))
            return StackAction{StackAction::Kind::Replace, *s};

        auto worst = HighestRevealedSlot(g, playerIdx);

        // 2. Replace a revealed card that is worse than what we drew.
        if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst)))
            return StackAction{StackAction::Kind::Replace, *worst};

        // 3. Good card: lock it into a hidden slot (or over our worst card).
        if (Val(drawn) <= kKeepThreshold)
        {
            if (auto u = FirstUnrevealedSlot(g, playerIdx))
                return StackAction{StackAction::Kind::Replace, *u};
            if (worst)
                return StackAction{StackAction::Kind::Replace, *worst};
        }

        // 4. Bad card we cannot use: discard it and flip a hidden card. The forced
        //    reveal picks the slot via ChooseInitialReveal, so this slot is unused.
        if (auto u = FirstUnrevealedSlot(g, playerIdx))
            return StackAction{StackAction::Kind::FlipOnly, *u};

        // 5. No hidden slot left: replace our worst card, else any live slot.
        if (worst) return StackAction{StackAction::Kind::Replace, *worst};
        if (auto s = FirstNonClearedSlot(g, playerIdx))
            return StackAction{StackAction::Kind::Replace, *s};
        return StackAction{StackAction::Kind::Replace, 0};
    }

    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
    {
        // We already committed to taking the discard card; place it well.
        if (auto s = ColumnClearSlot(g, playerIdx, drawn)) return *s;

        auto worst = HighestRevealedSlot(g, playerIdx);
        if (worst && Val(drawn) < Val(g.playerDeck.at(playerIdx).at(*worst))) return *worst;

        if (auto u = FirstUnrevealedSlot(g, playerIdx)) return *u;
        if (worst) return *worst;
        if (auto s = FirstNonClearedSlot(g, playerIdx)) return *s;
        return 0;
    }
}
