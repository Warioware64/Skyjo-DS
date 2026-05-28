#pragma once

#include "globalHeader.hpp"
#include "ErrorHandler.hpp"
#include "DebugPrint.hpp"
#include "GamePartyClasses/GamePartySharedAssets.hpp"


using PlayerGames = std::array<CardType, 12>;
using CardReturnType = std::array<CardReturn, 12>;
typedef struct
{
    int x_min, x_max, y_min, y_max;
} TouchPoseMyCard;

class GameParty
{
    private:
        void GamePartyLogic();
        void GamePartyLogicRender();
        void LoadGamePartyAssets();
        void InitCardStack();

        std::vector<PlayerGames> playerDeck;
        std::vector<CardType> cardPreStack;
        std::vector<CardType> cardStack;
        std::vector<CardReturnType> cardReturns;

        uint32_t keydown;
        touchPosition touchData;

        std::array<TouchPoseMyCard, 12> MyCardPos;
        NEA_Material *NotPossibleIconMat;
        NEA_Palette *NotPossibleIconPal;

        bool partyFirstTwoDraw;

        NEA_Sprite *myPacket[12];
        NEA_Sprite *pullpacket[2];
        NEA_Sprite *pullpacketIconNot[2];

        NEA_Hw2DOBJ *viewGame[12];
    public:
        GameParty();
        ~GameParty();

        void InitGamePartySituation(int number_arg, CPULevel cpu_arg, PartyType party_arg);
        void RenderGameParty();
};

extern GameParty gameparty;