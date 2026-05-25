#include "GameParty.hpp"
#include "globalHeader.hpp"
#include <NEAGeneral.h>
#include <NEAPalette.h>
#include <NEATexture.h>


GameParty::GameParty()
{

}

GameParty::~GameParty()
{

}

void GameParty::LoadGamePartyAssets()
{

    for ( CardType i = CardType::Negative_2 ; i != CardType::Positive_12; i = static_cast<CardType>( static_cast<int>(i) + 1))
    {

        sharedAssetsGameParty.GetCardMat(i) = NEA_MaterialCreate();
        sharedAssetsGameParty.GetCardPal(i) = NEA_PaletteCreate();

        NEA_MaterialTexLoadGRF(sharedAssetsGameParty.GetCardMat(i),
                                 sharedAssetsGameParty.GetCardPal(i),
                                  NEA_TEXGEN_TEXCOORD, sharedAssetsGameParty.GetCardGRFpath(i).c_str());
    }

    sharedAssetsGameParty.GetCardMat(std::nullopt) = NEA_MaterialCreate();
    sharedAssetsGameParty.GetCardPal(std::nullopt) = NEA_PaletteCreate();
    NEA_MaterialTexLoadGRF(sharedAssetsGameParty.GetCardMat(std::nullopt),
                            sharedAssetsGameParty.GetCardPal(std::nullopt),
                            NEA_TEXGEN_TEXCOORD, sharedAssetsGameParty.GetCardGRFpath(std::nullopt).c_str());
}

void GameParty::InitCardStack()
{
    this->cardPreStack.clear();
    this->cardStack.clear();
    this->cardPreStack.reserve(150);
    this->cardStack.reserve(150);

    for (auto const& item : cardPackage)
    {
        CardType typeToAdd = item.first;
        uint8_t iterateTimes = item.second;
        for (uint8_t i = 0; i < iterateTimes; i++)
            this->cardPreStack.push_back(typeToAdd);
    }

    this->cardStack = this->cardPreStack;
    std::mt19937 rng{static_cast<std::mt19937::result_type>(time(nullptr))};
    std::shuffle(this->cardStack.begin(), this->cardStack.end(), rng);

    this->cardPreStack.clear();
}

void GameParty::InitGamePartySituation(int number_arg, CPULevel cpu_arg, PartyType party_arg)
{
    this->LoadGamePartyAssets();
    this->InitCardStack();
    this->playerDeck.resize(number_arg);
    //consoleDemoInit();
    for (auto& hand : this->playerDeck)
    {
        auto first = this->cardStack.end() - 12;
        std::copy(first, this->cardStack.end(), hand.begin());
        this->cardStack.erase(first, this->cardStack.end());
        //std::println("Curent card pull is {}", this->cardStack.size());
    }
    
    

    /*
    for (auto const& item : this->playerDeck)
    {


        for (auto const& handP : item)
        {
            std::print("card {} , ", static_cast<int>(handP));
        }
        std::println();
        std::println("That was a player");
        
        swiWaitForVBlank();
    }
        */
    
    


}

void GameParty::RenderGameParty()
{
    NEA_WaitForVBL(NEA_CAN_SKIP_VBL);
}
GameParty gameparty;