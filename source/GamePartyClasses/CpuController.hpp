#pragma once

#include "PlayerController.hpp"

class CpuController : public IPlayerController
{
    public:
        explicit CpuController(CPULevel level);

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
