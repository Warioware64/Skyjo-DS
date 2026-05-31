#pragma once

// Shared, fair (revealed-only) primitives for the CPU strategies. All functions
// are header-only inline helpers so Easy/Medium/Hard can build their policies on
// top of the same building blocks without a separate translation unit.

#include "../../GameParty.hpp"

namespace StrategyUtil
{
    inline int Val(CardType c) { return static_cast<int>(c); }

    // Slot of the player's worst (highest-value) currently revealed card.
    inline std::optional<int> HighestRevealedSlot(const GameParty& g, int p)
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

    inline std::optional<int> FirstUnrevealedSlot(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Unreturned) return i;
        return std::nullopt;
    }

    inline std::optional<int> FirstNonClearedSlot(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) != CardReturn::Cleared) return i;
        return std::nullopt;
    }

    // If two cards of a column are revealed and equal `drawn`, return the third
    // slot: placing `drawn` there makes the column three-of-a-kind and clears it.
    // Clearing always scores 0, so this is desirable regardless of the value.
    inline std::optional<int> ColumnClearSlot(const GameParty& g, int p, CardType drawn)
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

    // A hidden slot in a column that already shows exactly one card equal to
    // `drawn` (and whose third slot is not cleared). Placing `drawn` there builds
    // a 2-of-3 toward a future column clear. Used by Medium/Hard.
    inline std::optional<int> ColumnSetupSlot(const GameParty& g, int p, CardType drawn)
    {
        const auto& ret = g.cardReturns.at(p);
        const auto& deck = g.playerDeck.at(p);
        for (int c = 0; c < 4; ++c)
        {
            int slots[3] = { c, c + 4, c + 8 };
            int equalRevealed = 0;
            bool anyCleared = false;
            std::optional<int> hidden;
            for (int s : slots)
            {
                if (ret.at(s) == CardReturn::Cleared) { anyCleared = true; continue; }
                if (ret.at(s) == CardReturn::Returned && deck.at(s) == drawn) ++equalRevealed;
                else if (ret.at(s) == CardReturn::Unreturned && !hidden) hidden = s;
            }
            if (!anyCleared && equalRevealed == 1 && hidden) return hidden;
        }
        return std::nullopt;
    }

    // Sum of the player's revealed card values (fair: ignores face-down cards).
    inline int RevealedSum(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        const auto& deck = g.playerDeck.at(p);
        int sum = 0;
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Returned) sum += Val(deck.at(i));
        return sum;
    }

    inline int UnrevealedCount(const GameParty& g, int p)
    {
        const auto& ret = g.cardReturns.at(p);
        int n = 0;
        for (int i = 0; i < 12; ++i)
            if (ret.at(i) == CardReturn::Unreturned) ++n;
        return n;
    }

    // Rough standing: known revealed points plus a deck-average estimate (5) for
    // each still-hidden card. Lets Hard compare itself to opponents fairly.
    inline int EstimatedTotal(const GameParty& g, int p)
    {
        return RevealedSum(g, p) + 5 * UnrevealedCount(g, p);
    }
}
