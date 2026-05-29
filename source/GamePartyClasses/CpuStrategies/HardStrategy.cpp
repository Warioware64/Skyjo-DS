#include "../CpuController.hpp"
#include "../../GameParty.hpp"

namespace HardStrategy
{
    // TODO Hard AI: track opponent revealed cards, optimize column clears,
    //               push for game-end when winning, defend when losing.
    int ChooseInitialReveal(const GameParty& g, int playerIdx)
    {
        return EasyStrategy::ChooseInitialReveal(g, playerIdx);
    }

    DrawSource ChooseDrawSource(const GameParty& g, int playerIdx)
    {
        return EasyStrategy::ChooseDrawSource(g, playerIdx);
    }

    StackAction ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
    {
        return EasyStrategy::ChooseStackAction(g, playerIdx, drawn);
    }

    int ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
    {
        return EasyStrategy::ChooseDiscardReplaceSlot(g, playerIdx, drawn);
    }
}
