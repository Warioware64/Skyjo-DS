#include "GamePartySharedAssets.hpp"
#include "globalHeader.hpp"


GamePartySharedAssets::GamePartySharedAssets()
{

}

GamePartySharedAssets::~GamePartySharedAssets()
{

}

NEA_Material* GamePartySharedAssets::GetCardMat(std::optional<CardType> card_type)
{
    NEA_Material *MatRes;
    if (card_type == std::nullopt)
    {
        MatRes = this->card_back_mat;
        return MatRes;
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:
        {
            MatRes = this->card_0_mat;
            break;
        }

        case CardType::Positive_1:
        {
            MatRes = this->card_1_mat;
            break;
        }

        case CardType::Positive_2:
        {
            MatRes = this->card_2_mat;
            break;
        }
        case CardType::Positive_3:
        {
            MatRes = this->card_3_mat;
            break;
        }

        case CardType::Positive_4:
        {
            MatRes = this->card_4_mat;
            break;
        }

        case CardType::Positive_5:
        {
            MatRes = this->card_5_mat;
            break;
        }

        case CardType::Positive_6:
        {
            MatRes = this->card_6_mat;
            break;
        }

        case CardType::Positive_7:
        {
            MatRes = this->card_7_mat;
            break;
        }

        case CardType::Positive_8:
        {
            MatRes = this->card_8_mat;
            break;
        }

        case CardType::Positive_9:
        {
            MatRes = this->card_9_mat;
            break;
        }

        case CardType::Positive_10:
        {
            MatRes = this->card_10_mat;
            break;
        }

        case CardType::Positive_11:
        {
            MatRes = this->card_11_mat;
            break;
        }

        case CardType::Positive_12:
        {
            MatRes = this->card_12_mat;
            break;
        }

        case CardType::Negative_1:
        {
            MatRes = this->card_n1_mat;
            break;
        }

        case CardType::Negative_2:
        {
            MatRes = this->card_n2_mat;
            break;
        }


        default:
        {
            MatRes = this->card_0_mat;
            break;
        }
    }

    return MatRes;
}

NEA_Palette* GamePartySharedAssets::GetCardPal(std::optional<CardType> card_type)
{
    NEA_Palette *palRes;

    if (card_type == std::nullopt)
    {
        palRes = this->card_back_pal;
        return palRes; 
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:
        {
            palRes = this->card_0_pal;
            break;
        }

        case CardType::Positive_1:
        {
            palRes = this->card_1_pal;
            break;
        }

        case CardType::Positive_2:
        {
            palRes = this->card_2_pal;
            break;
        }
        case CardType::Positive_3:
        {
            palRes = this->card_3_pal;
            break;
        }

        case CardType::Positive_4:
        {
            palRes = this->card_4_pal;
            break;
        }

        case CardType::Positive_5:
        {
            palRes = this->card_5_pal;
            break;
        }

        case CardType::Positive_6:
        {
            palRes = this->card_6_pal;
            break;
        }

        case CardType::Positive_7:
        {
            palRes = this->card_7_pal;
            break;
        }

        case CardType::Positive_8:
        {
            palRes = this->card_8_pal;
            break;
        }

        case CardType::Positive_9:
        {
            palRes = this->card_9_pal;
            break;
        }

        case CardType::Positive_10:
        {
            palRes = this->card_10_pal;
            break;
        }

        case CardType::Positive_11:
        {
            palRes = this->card_11_pal;
            break;
        }

        case CardType::Positive_12:
        {
            palRes = this->card_12_pal;
            break;
        }

        case CardType::Negative_1:
        {
            palRes = this->card_n1_pal;
            break;
        }

        case CardType::Negative_2:
        {
            palRes = this->card_n2_pal;
            break;
        }
        
        default:
        {
            palRes = this->card_0_pal;
            break;
        }
    }

    return palRes;
}

std::string GamePartySharedAssets::GetCardGRFpath(std::optional<CardType> card_type)
{
    std::string tempStr;

    if (card_type == std::nullopt)
    {
        tempStr.assign("cards/card_back_png.grf");
        return tempStr;
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:
        {
            tempStr.assign("cards/card_0_png.grf");
            break;
        }

        case CardType::Positive_1:
        {
            tempStr.assign("cards/card_1_png.grf");
            break;
        }

        case CardType::Positive_2:
        {
            tempStr.assign("cards/card_2_png.grf");
            break;
        }
        case CardType::Positive_3:
        {
            tempStr.assign("cards/card_3_png.grf");
            break;
        }

        case CardType::Positive_4:
        {
            tempStr.assign("cards/card_4_png.grf");
            break;
        }

        case CardType::Positive_5:
        {
            tempStr.assign("cards/card_5_png.grf");
            break;
        }

        case CardType::Positive_6:
        {
            tempStr.assign("cards/card_6_png.grf");
            break;
        }

        case CardType::Positive_7:
        {
            tempStr.assign("cards/card_7_png.grf");
            break;
        }

        case CardType::Positive_8:
        {
            tempStr.assign("cards/card_8_png.grf");
            break;
        }

        case CardType::Positive_9:
        {
            tempStr.assign("cards/card_9_png.grf");
            break;
        }

        case CardType::Positive_10:
        {
            tempStr.assign("cards/card_10_png.grf");
            break;
        }

        case CardType::Positive_11:
        {
            tempStr.assign("cards/card_11_png.grf");
            break;
        }

        case CardType::Positive_12:
        {
            tempStr.assign("cards/card_12_png.grf");
            break;
        }

        case CardType::Negative_1:
        {
            tempStr.assign("cards/card_n1_png.grf");
            break;
        }

        case CardType::Negative_2:
        {
            tempStr.assign("cards/card_n2_png.grf");
            break;
        }
 
        default:
        {
            tempStr.assign("cards/card_0_png.grf");
            break;
        }
    }

    return tempStr;
}
GamePartySharedAssets sharedAssetsGameParty;