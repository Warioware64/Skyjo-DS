#include "../CpuController.hpp"
#include "../../GameParty.hpp"

namespace MediumStrategy
{
    // TODO Medium AI: take from discard when its value is low (<= 3),
    //                 attempt column-of-three matches with own revealed cards.
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
