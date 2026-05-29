#pragma once

#include "../globalHeader.hpp"

class GameParty;

enum class DrawSource
{
    Stack,
    Discard
};

struct StackAction
{
    enum class Kind { Replace, FlipOnly } kind;
    int slot;
};

enum class GamePhase
{
    InitialReveal,
    Turns,
    LastRound,
    Scoring,
    Ended
};

class IPlayerController
{
    public:
        virtual ~IPlayerController() = default;

        virtual std::optional<int>
            ChooseInitialReveal(const GameParty& g, int playerIdx) = 0;

        virtual std::optional<DrawSource>
            ChooseDrawSource(const GameParty& g, int playerIdx) = 0;

        virtual std::optional<StackAction>
            ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn) = 0;

        virtual std::optional<int>
            ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn) = 0;
};
