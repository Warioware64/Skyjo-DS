#include "RemoteController.hpp"
#include "../Net/NetLink.hpp"

RemoteController::RemoteController(int seat) : seat(seat) {}

bool RemoteController::Pump()
{
    if (this->pending) return true;
    GameNetIntent in;
    if (NetLink::HostPollIntent(this->seat, in))
    {
        this->pending = in;
        return true;
    }
    return false;
}

std::optional<int>
RemoteController::ChooseInitialReveal(const GameParty&, int)
{
    if (!this->Pump()) return std::nullopt;
    if (static_cast<NetIntentKind>(this->pending->kind) == NetIntentKind::InitialReveal)
    {
        int slot = this->pending->arg;
        this->pending.reset();
        return slot;
    }
    // Stale/wrong-kind intent: drop it so the link can't deadlock.
    this->pending.reset();
    return std::nullopt;
}

std::optional<DrawSource>
RemoteController::ChooseDrawSource(const GameParty&, int)
{
    if (!this->Pump()) return std::nullopt;
    auto kind = static_cast<NetIntentKind>(this->pending->kind);
    if (kind == NetIntentKind::DrawStack)   { this->pending.reset(); return DrawSource::Stack; }
    if (kind == NetIntentKind::DrawDiscard) { this->pending.reset(); return DrawSource::Discard; }
    this->pending.reset();
    return std::nullopt;
}

std::optional<StackAction>
RemoteController::ChooseStackAction(const GameParty&, int, CardType)
{
    if (!this->Pump()) return std::nullopt;
    auto kind = static_cast<NetIntentKind>(this->pending->kind);
    if (kind == NetIntentKind::StackReplace)
    {
        int slot = this->pending->arg;
        this->pending.reset();
        return StackAction{StackAction::Kind::Replace, slot};
    }
    if (kind == NetIntentKind::StackFlipOnly)
    {
        this->pending.reset();
        return StackAction{StackAction::Kind::FlipOnly, -1};
    }
    this->pending.reset();
    return std::nullopt;
}

std::optional<int>
RemoteController::ChooseDiscardReplaceSlot(const GameParty&, int, CardType)
{
    if (!this->Pump()) return std::nullopt;
    if (static_cast<NetIntentKind>(this->pending->kind) == NetIntentKind::DiscardReplace)
    {
        int slot = this->pending->arg;
        this->pending.reset();
        return slot;
    }
    this->pending.reset();
    return std::nullopt;
}
