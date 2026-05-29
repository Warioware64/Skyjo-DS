#include "HumanTouchController.hpp"
#include "../GameParty.hpp"

namespace
{
    constexpr int kStackX0 = 25,  kStackY0 = 41;
    constexpr int kDiscardX0 = 25, kDiscardY0 = 81;
    constexpr int kPullW = 24, kPullH = 36;

    bool InRect(int px, int py, int x0, int y0, int w, int h)
    {
        return (px >= x0) && (px <= x0 + w) && (py >= y0) && (py <= y0 + h);
    }

    std::optional<int> TouchedHandSlot(const GameParty& g)
    {
        for (int i = 0; i < 12; ++i)
        {
            const auto& r = g.MyCardPos.at(i);
            if (g.touchData.px >= r.x_min && g.touchData.px <= r.x_max &&
                g.touchData.py >= r.y_min && g.touchData.py <= r.y_max)
            {
                return i;
            }
        }
        return std::nullopt;
    }
}

std::optional<int>
HumanTouchController::ChooseInitialReveal(const GameParty& g, int playerIdx)
{
    if (!(g.keydown & KEY_TOUCH)) return std::nullopt;
    auto slot = TouchedHandSlot(g);
    if (!slot) return std::nullopt;
    if (g.cardReturns.at(playerIdx).at(*slot) == CardReturn::Returned) return std::nullopt;
    return slot;
}

std::optional<DrawSource>
HumanTouchController::ChooseDrawSource(const GameParty& g, int playerIdx)
{
    (void)playerIdx;
    if (!(g.keydown & KEY_TOUCH)) return std::nullopt;
    if (InRect(g.touchData.px, g.touchData.py, kStackX0, kStackY0, kPullW, kPullH))
        return DrawSource::Stack;
    if (InRect(g.touchData.px, g.touchData.py, kDiscardX0, kDiscardY0, kPullW, kPullH))
        return DrawSource::Discard;
    return std::nullopt;
}

std::optional<StackAction>
HumanTouchController::ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
{
    (void)drawn;
    if (!(g.keydown & KEY_TOUCH)) return std::nullopt;

    if (InRect(g.touchData.px, g.touchData.py, kDiscardX0, kDiscardY0, kPullW, kPullH))
    {
        waitingFlipOnlyDecision = true;
        return std::nullopt;
    }

    auto slot = TouchedHandSlot(g);
    if (!slot) return std::nullopt;

    if (waitingFlipOnlyDecision)
    {
        if (g.cardReturns.at(playerIdx).at(*slot) == CardReturn::Returned)
            return std::nullopt;
        waitingFlipOnlyDecision = false;
        return StackAction{StackAction::Kind::FlipOnly, *slot};
    }

    return StackAction{StackAction::Kind::Replace, *slot};
}

std::optional<int>
HumanTouchController::ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
{
    (void)playerIdx;
    (void)drawn;
    if (!(g.keydown & KEY_TOUCH)) return std::nullopt;
    return TouchedHandSlot(g);
}
