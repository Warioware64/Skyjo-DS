#include "../globalHeader.hpp"

class GamePartySharedAssets
{
    private:
        NEA_Material *card_0_mat;
        NEA_Palette *card_0_pal;

        NEA_Material *card_1_mat;
        NEA_Palette *card_1_pal;

        NEA_Material *card_2_mat;
        NEA_Palette *card_2_pal;

        NEA_Material *card_3_mat;
        NEA_Palette *card_3_pal;

        NEA_Material *card_4_mat;
        NEA_Palette *card_4_pal;

        NEA_Material *card_5_mat;
        NEA_Palette *card_5_pal;

        NEA_Material *card_6_mat;
        NEA_Palette *card_6_pal;

        NEA_Material *card_7_mat;
        NEA_Palette *card_7_pal;

        NEA_Material *card_8_mat;
        NEA_Palette *card_8_pal;

        NEA_Material *card_9_mat;
        NEA_Palette *card_9_pal;

        NEA_Material *card_10_mat;
        NEA_Palette *card_10_pal;

        NEA_Material *card_11_mat;
        NEA_Palette *card_11_pal;

        NEA_Material *card_12_mat;
        NEA_Palette *card_12_pal;

        NEA_Material *card_n1_mat;
        NEA_Palette *card_n1_pal;

        NEA_Material *card_n2_mat;
        NEA_Palette *card_n2_pal;

        NEA_Material *card_back_mat;
        NEA_Palette *card_back_pal;
    public:
        GamePartySharedAssets();
        ~GamePartySharedAssets();

        NEA_Material* GetCardMat(std::optional<CardType> card_type);
        NEA_Palette* GetCardPal(std::optional<CardType> card_type);
        std::string GetCardGRFpath(std::optional<CardType> card_type);
};

extern GamePartySharedAssets sharedAssetsGameParty;