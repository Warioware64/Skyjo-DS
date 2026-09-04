#pragma once

#include "PlayerController.hpp"

class CpuController : public IPlayerController
{
    public:
        // `stagger` offsets this CPU's first decision by that many frames. Every
        // CpuController is built in the same call with the same think delay, so
        // without it they all act on the same frame -- which during the initial
        // reveal (the one phase where every player acts in the same frame) means
        // several copies of the same sound effect start sample-aligned and sum
        // into a clipped, flanged mess. The offset survives every later decision
        // because TickReady() restarts the delay from the moment each CPU acted.
        explicit CpuController(CPULevel level, int stagger = 0);

        std::optional<int>
            ChooseInitialReveal(const GameParty& g, int playerIdx) override;

        std::optional<DrawSource>
            ChooseDrawSource(const GameParty& g, int playerIdx) override;

        std::optional<StackAction>
            ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn) override;

        std::optional<int>
            ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn) override;

    private:
        CPULevel level;
        int thinkingFrames = 0;

        int DelayFor(CPULevel l) const;
        bool TickReady();
};

namespace EasyStrategy
{
    int ChooseInitialReveal(const GameParty& g, int playerIdx);
    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx);
    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn);
    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn);
}

namespace MediumStrategy
{
    int ChooseInitialReveal(const GameParty& g, int playerIdx);
    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx);
    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn);
    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn);
}

namespace HardStrategy
{
    int ChooseInitialReveal(const GameParty& g, int playerIdx);
    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx);
    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn);
    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn);
}
