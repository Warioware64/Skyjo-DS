#pragma once

#include "PlayerController.hpp"
#include "../Net/NetProtocol.hpp"

#include <optional>

// Host-side controller standing in for a networked (client) player. It never
// reads local input; instead it returns whatever decision the matching client
// most recently sent over WiFi. One instance per remote seat. Polling for the
// actual intent bytes happens in the .cpp via NetLink.
class RemoteController : public IPlayerController
{
    public:
        explicit RemoteController(int seat);

        std::optional<int>
            ChooseInitialReveal(const GameParty& g, int playerIdx) override;

        std::optional<DrawSource>
            ChooseDrawSource(const GameParty& g, int playerIdx) override;

        std::optional<StackAction>
            ChooseStackAction(const GameParty& g, int playerIdx, CardType drawn) override;

        std::optional<int>
            ChooseDiscardReplaceSlot(const GameParty& g, int playerIdx, CardType drawn) override;

    private:
        int seat;
        std::optional<GameNetIntent> pending;

        // Refill `pending` from the network if empty; returns whether we hold one.
        bool Pump();
};
