#include "CpuController.hpp"
#include "../GameParty.hpp"

CpuController::CpuController(CPULevel l) : level(l)
{
    thinkingFrames = DelayFor(level);
}

int CpuController::DelayFor(CPULevel l) const
{
    switch (l)
    {
        case CPULevel::Easy:   return 80;
        case CPULevel::Medium: return 60;
        case CPULevel::Hard:   return 45;
    }
    return 30;
}

bool CpuController::TickReady()
{
    if (thinkingFrames > 0) { --thinkingFrames; return false; }
    thinkingFrames = DelayFor(level);
    return true;
}

std::optional<int>
CpuController::ChooseInitialReveal(const GameParty& g, int playerIdx)
{
    if (!TickReady()) return std::nullopt;
    switch (level)
    {
        case CPULevel::Easy:   return EasyStrategy::ChooseInitialReveal(g, playerIdx);
        case CPULevel::Medium: return MediumStrategy::ChooseInitialReveal(g, playerIdx);
        case CPULevel::Hard:   return HardStrategy::ChooseInitialReveal(g, playerIdx);
    }
    return std::nullopt;
}

std::optional<DrawSource>
CpuController::ChooseDrawSource(const GameParty& g, int playerIdx)
{
    if (!TickReady()) return std::nullopt;
    switch (level)
    {
        case CPULevel::Easy:   return EasyStrategy::ChooseDrawSource(g, playerIdx);
        case CPULevel::Medium: return MediumStrategy::ChooseDrawSource(g, playerIdx);
        case CPULevel::Hard:   return HardStrategy::ChooseDrawSource(g, playerIdx);
    }
    return std::nullopt;
}

std::optional<StackAction>
CpuController::ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn)
{
    if (!TickReady()) return std::nullopt;
    switch (level)
    {
        case CPULevel::Easy:   return EasyStrategy::ChooseStackAction(g, playerIdx, drawn);
        case CPULevel::Medium: return MediumStrategy::ChooseStackAction(g, playerIdx, drawn);
        case CPULevel::Hard:   return HardStrategy::ChooseStackAction(g, playerIdx, drawn);
    }
    return std::nullopt;
}

std::optional<int>
CpuController::ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn)
{
    if (!TickReady()) return std::nullopt;
    switch (level)
    {
        case CPULevel::Easy:   return EasyStrategy::ChooseDiscardReplaceSlot(g, playerIdx, drawn);
        case CPULevel::Medium: return MediumStrategy::ChooseDiscardReplaceSlot(g, playerIdx, drawn);
        case CPULevel::Hard:   return HardStrategy::ChooseDiscardReplaceSlot(g, playerIdx, drawn);
    }
    return std::nullopt;
}
