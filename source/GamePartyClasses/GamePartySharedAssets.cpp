#include "GamePartySharedAssets.hpp"
#include "globalHeader.hpp"


GamePartySharedAssets::GamePartySharedAssets()
{

}

GamePartySharedAssets::~GamePartySharedAssets()
{

}

NEA_Material*& GamePartySharedAssets::GetCardMat(std::optional<CardType> card_type)
{
    if (card_type == std::nullopt)
    {
        return this->card_back_mat;
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:   return this->card_0_mat;
        case CardType::Positive_1:  return this->card_1_mat;
        case CardType::Positive_2:  return this->card_2_mat;
        case CardType::Positive_3:  return this->card_3_mat;
        case CardType::Positive_4:  return this->card_4_mat;
        case CardType::Positive_5:  return this->card_5_mat;
        case CardType::Positive_6:  return this->card_6_mat;
        case CardType::Positive_7:  return this->card_7_mat;
        case CardType::Positive_8:  return this->card_8_mat;
        case CardType::Positive_9:  return this->card_9_mat;
        case CardType::Positive_10: return this->card_10_mat;
        case CardType::Positive_11: return this->card_11_mat;
        case CardType::Positive_12: return this->card_12_mat;
        case CardType::Negative_1:  return this->card_n1_mat;
        case CardType::Negative_2:  return this->card_n2_mat;
        default:                    return this->card_0_mat;
    }
}

NEA_Palette*& GamePartySharedAssets::GetCardPal(std::optional<CardType> card_type)
{
    if (card_type == std::nullopt)
    {
        return this->card_back_pal;
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:   return this->card_0_pal;
        case CardType::Positive_1:  return this->card_1_pal;
        case CardType::Positive_2:  return this->card_2_pal;
        case CardType::Positive_3:  return this->card_3_pal;
        case CardType::Positive_4:  return this->card_4_pal;
        case CardType::Positive_5:  return this->card_5_pal;
        case CardType::Positive_6:  return this->card_6_pal;
        case CardType::Positive_7:  return this->card_7_pal;
        case CardType::Positive_8:  return this->card_8_pal;
        case CardType::Positive_9:  return this->card_9_pal;
        case CardType::Positive_10: return this->card_10_pal;
        case CardType::Positive_11: return this->card_11_pal;
        case CardType::Positive_12: return this->card_12_pal;
        case CardType::Negative_1:  return this->card_n1_pal;
        case CardType::Negative_2:  return this->card_n2_pal;
        default:                    return this->card_0_pal;
    }
}

NEA_Hw2DOBJAsset*& GamePartySharedAssets::GetCardOBJ(std::optional<CardType> card_type)
{
    if (card_type == std::nullopt)
    {
        return this->card_back_obj;
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:   return this->card_0_obj;
        case CardType::Positive_1:  return this->card_1_obj;
        case CardType::Positive_2:  return this->card_2_obj;
        case CardType::Positive_3:  return this->card_3_obj;
        case CardType::Positive_4:  return this->card_4_obj;
        case CardType::Positive_5:  return this->card_5_obj;
        case CardType::Positive_6:  return this->card_6_obj;
        case CardType::Positive_7:  return this->card_7_obj;
        case CardType::Positive_8:  return this->card_8_obj;
        case CardType::Positive_9:  return this->card_9_obj;
        case CardType::Positive_10: return this->card_10_obj;
        case CardType::Positive_11: return this->card_11_obj;
        case CardType::Positive_12: return this->card_12_obj;
        case CardType::Negative_1:  return this->card_n1_obj;
        case CardType::Negative_2:  return this->card_n2_obj;
        default:                    return this->card_0_obj;
    }
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



std::string GamePartySharedAssets::GetHwCardGRFpath(std::optional<CardType> card_type)
{
    std::string tempStr;

    if (card_type == std::nullopt)
    {
        tempStr.assign("cards2/card_back_png.grf");
        return tempStr;
    }

    switch (card_type.value())
    {
        case CardType::Neutral_0:
        {
            tempStr.assign("cards2/card_0_png.grf");
            break;
        }

        case CardType::Positive_1:
        {
            tempStr.assign("cards2/card_1_png.grf");
            break;
        }

        case CardType::Positive_2:
        {
            tempStr.assign("cards2/card_2_png.grf");
            break;
        }
        case CardType::Positive_3:
        {
            tempStr.assign("cards2/card_3_png.grf");
            break;
        }

        case CardType::Positive_4:
        {
            tempStr.assign("cards2/card_4_png.grf");
            break;
        }

        case CardType::Positive_5:
        {
            tempStr.assign("cards2/card_5_png.grf");
            break;
        }

        case CardType::Positive_6:
        {
            tempStr.assign("cards2/card_6_png.grf");
            break;
        }

        case CardType::Positive_7:
        {
            tempStr.assign("cards2/card_7_png.grf");
            break;
        }

        case CardType::Positive_8:
        {
            tempStr.assign("cards2/card_8_png.grf");
            break;
        }

        case CardType::Positive_9:
        {
            tempStr.assign("cards2/card_9_png.grf");
            break;
        }

        case CardType::Positive_10:
        {
            tempStr.assign("cards2/card_10_png.grf");
            break;
        }

        case CardType::Positive_11:
        {
            tempStr.assign("cards2/card_11_png.grf");
            break;
        }

        case CardType::Positive_12:
        {
            tempStr.assign("cards2/card_12_png.grf");
            break;
        }

        case CardType::Negative_1:
        {
            tempStr.assign("cards2/card_n1_png.grf");
            break;
        }

        case CardType::Negative_2:
        {
            tempStr.assign("cards2/card_n2_png.grf");
            break;
        }
 
        default:
        {
            tempStr.assign("cards2/card_0_png.grf");
            break;
        }
    }

    return tempStr;
}
GamePartySharedAssets sharedAssetsGameParty;