#include "GameParty.hpp"
#include "globalHeader.hpp"
#include <NEA2D.h>
#include <NEAGeneral.h>
#include <NEAHw2D.h>
#include <NEARichText.h>
#include <NEATexture.h>



GameParty::GameParty()
{

}

GameParty::~GameParty()
{

}

void GameParty::GamePartyLogic()
{
    
    if (std::ranges::count(this->cardReturns.at(0), CardReturn::Returned) >= 2 )
    {
        this->partyFirstTwoDraw = false;
        NEA_SpriteVisible(this->pullpacketIconNot[0], false);
        NEA_SpriteVisible(this->pullpacketIconNot[1], false);
    }
    if (this->keydown & KEY_TOUCH)
    {
        int temp_i = 0;
        for (auto const& item : this->MyCardPos)
        {
            if ( ((this->touchData.px >= item.x_min) && (this->touchData.px <= item.x_max)) && ((this->touchData.py >= item.y_min) && (this->touchData.py <= item.y_max)) ) 
            {
                this->cardReturns.at(0).at(temp_i) = CardReturn::Returned;
                NEA_SpriteSetMaterial(this->myPacket[temp_i], sharedAssetsGameParty.GetCardMat(this->playerDeck.at(0).at(temp_i)));
                break;
            }

            temp_i++;
        }    
    }

}

void GameParty::GamePartyLogicRender()
{
    NEA_2DViewInit();
    

    if (this->partyFirstTwoDraw)
    {
        NEA_SpriteVisible(this->pullpacketIconNot[0], true);
        NEA_SpriteVisible(this->pullpacketIconNot[1], true);
        NEA_RichTextRender3D(0, "Reveal two card \n", 120, 15);
    }
    NEA_SpriteDrawAll();
}
void GameParty::LoadGamePartyAssets()
{

    for (int n = static_cast<int>(CardType::Negative_2); n <= static_cast<int>(CardType::Positive_12); ++n)
    {
        CardType i = static_cast<CardType>(n);

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

    this->NotPossibleIconMat = NEA_MaterialCreate();
    this->NotPossibleIconPal = NEA_PaletteCreate();

    NEA_MaterialTexLoadGRF(this->NotPossibleIconMat, this->NotPossibleIconPal, NEA_TEXGEN_TEXCOORD, "ingame/clear_png.grf");
}

void GameParty::InitCardStack()
{
    this->cardReturns.clear();

    for (auto& item : this->cardReturns)
    {
        for (auto& intern : item)
        {
            intern = CardReturn::Unreturned;


        }
    }
    


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
    this->partyFirstTwoDraw = true;
    this->LoadGamePartyAssets();
    this->InitCardStack();
    this->playerDeck.resize(number_arg);

    std::array<CardReturn, 12> unreturnedRow;
    unreturnedRow.fill(CardReturn::Unreturned);
    this->cardReturns.assign(number_arg, unreturnedRow);
    //consoleDemoInit();
    for (auto& hand : this->playerDeck)
    {
        auto first = this->cardStack.end() - 12;
        std::copy(first, this->cardStack.end(), hand.begin());
        this->cardStack.erase(first, this->cardStack.end());
        //std::println("Curent card pull is {}", this->cardStack.size());
    }
    
    
    
    int x = 112;
    int y = 25;
    for (size_t i = 0; i < 12; i++)
    {
        this->myPacket[i] = NEA_SpriteCreate();
        NEA_SpriteSetMaterial(this->myPacket[i], sharedAssetsGameParty.GetCardMat(std::nullopt));
        NEA_SpriteSetPos(this->myPacket[i], x, y);
        this->MyCardPos.at(i) = {
            .x_min = x,
            .x_max = x + 24,
            .y_min = y,
            .y_max = y + 36,
        };
        x += 28;
        if (((i + 1) % 4 == 0) && ( i != 0))
        {
            y += 40;
            x = 112;
        }
    }

    x = 12;
    y = 12;
    NEA_Hw2DOBJAsset *asset2dtest = NEA_Hw2DOBJAssetCreate(NEA_ENGINE_SUB, NEA_OBJ_SIZE_32x64, NEA_OBJ_COLOR_16);
    NEA_Hw2DOBJAssetLoadGRFFAT(asset2dtest, "cards2/card_back_png.grf");

    for (size_t i = 0; i < 12; i++)
    {
        
        this->viewGame[i] = NEA_Hw2DOBJCreateFromAsset(asset2dtest);
        //NEA_Hw2DOBJLoadGRFFAT(this->viewGame[i], "cards2/card_back_png.grf", 0);
        NEA_Hw2DOBJSetPos(this->viewGame[i], x, y);
        NEA_Hw2DOBJSetVisible(this->viewGame[i], true);
        x += 28;
        if (((i + 1) % 4 == 0) && ( i != 0))
        {
            y += 40;
            x = 12;
        }
    }
    this->pullpacket[0] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacket[0], sharedAssetsGameParty.GetCardMat(std::nullopt));
    NEA_SpriteSetPos(this->pullpacket[0], 25, 41);
    NEA_SpriteSetPriority(this->pullpacket[0], 1);

    this->pullpacket[1] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacket[1], sharedAssetsGameParty.GetCardMat(CardType::Positive_3));
    NEA_SpriteSetPos(this->pullpacket[1], 25, 81);
    NEA_SpriteSetPriority(this->pullpacket[1], 1);

    this->pullpacketIconNot[0] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacketIconNot[0], this->NotPossibleIconMat);
    NEA_SpriteSetPos(this->pullpacketIconNot[0], 25, 56);
    NEA_SpriteVisible(this->pullpacketIconNot[0], false);
    NEA_SpriteSetPriority(this->pullpacketIconNot[0], 0);

    this->pullpacketIconNot[1] = NEA_SpriteCreate();
    NEA_SpriteSetMaterial(this->pullpacketIconNot[1], this->NotPossibleIconMat);
    NEA_SpriteSetPos(this->pullpacketIconNot[1], 25, 96);
    NEA_SpriteVisible(this->pullpacketIconNot[1], false);
    NEA_SpriteSetPriority(this->pullpacketIconNot[1], 0);

    setBrightness(3, 0);


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
    while (1)
    {
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
        scanKeys();
        this->keydown = keysDown();
        touchRead(&this->touchData);
        this->GamePartyLogic();

        NEA_Process([](){
            gameparty.GamePartyLogicRender();
        });
    }
}
GameParty gameparty;