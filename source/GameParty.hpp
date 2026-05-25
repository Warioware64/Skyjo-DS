#pragma once

#include "globalHeader.hpp"
#include "ErrorHandler.hpp"

#include "GamePartyClasses/GamePartySharedAssets.hpp"

using PlayerGames = std::array<CardType, 12>;
class GameParty
{
    private:
        void LoadGamePartyAssets();
        void InitCardStack();

        std::vector<PlayerGames> playerDeck;
        std::vector<CardType> cardPreStack;
        std::vector<CardType> cardStack;
    public:
        GameParty();
        ~GameParty();

        void InitGamePartySituation(int number_arg, CPULevel cpu_arg, PartyType party_arg);
        void RenderGameParty();
};

extern GameParty gameparty;