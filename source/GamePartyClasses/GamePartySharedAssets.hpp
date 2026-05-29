#include "../globalHeader.hpp"

class GamePartySharedAssets
{
    private:
        NEA_Material *card_0_mat;
        NEA_Palette *card_0_pal;
        NEA_Hw2DOBJAsset *card_0_obj;

        NEA_Material *card_1_mat;
        NEA_Palette *card_1_pal;
        NEA_Hw2DOBJAsset *card_1_obj;

        NEA_Material *card_2_mat;
        NEA_Palette *card_2_pal;
        NEA_Hw2DOBJAsset *card_2_obj;

        NEA_Material *card_3_mat;
        NEA_Palette *card_3_pal;
        NEA_Hw2DOBJAsset *card_3_obj;

        NEA_Material *card_4_mat;
        NEA_Palette *card_4_pal;
        NEA_Hw2DOBJAsset *card_4_obj;

        NEA_Material *card_5_mat;
        NEA_Palette *card_5_pal;
        NEA_Hw2DOBJAsset *card_5_obj;

        NEA_Material *card_6_mat;
        NEA_Palette *card_6_pal;
        NEA_Hw2DOBJAsset *card_6_obj;

        NEA_Material *card_7_mat;
        NEA_Palette *card_7_pal;
        NEA_Hw2DOBJAsset *card_7_obj;

        NEA_Material *card_8_mat;
        NEA_Palette *card_8_pal;
        NEA_Hw2DOBJAsset *card_8_obj;

        NEA_Material *card_9_mat;
        NEA_Palette *card_9_pal;
        NEA_Hw2DOBJAsset *card_9_obj;

        NEA_Material *card_10_mat;
        NEA_Palette *card_10_pal;
        NEA_Hw2DOBJAsset *card_10_obj;

        NEA_Material *card_11_mat;
        NEA_Palette *card_11_pal;
        NEA_Hw2DOBJAsset *card_11_obj;

        NEA_Material *card_12_mat;
        NEA_Palette *card_12_pal;
        NEA_Hw2DOBJAsset *card_12_obj;

        NEA_Material *card_n1_mat;
        NEA_Palette *card_n1_pal;
        NEA_Hw2DOBJAsset *card_n1_obj;

        NEA_Material *card_n2_mat;
        NEA_Palette *card_n2_pal;
        NEA_Hw2DOBJAsset *card_n2_obj;

        NEA_Material *card_back_mat;
        NEA_Palette *card_back_pal;
        NEA_Hw2DOBJAsset *card_back_obj;
    public:
        GamePartySharedAssets();
        ~GamePartySharedAssets();

        NEA_Material*& GetCardMat(std::optional<CardType> card_type);
        NEA_Palette*& GetCardPal(std::optional<CardType> card_type);
        NEA_Hw2DOBJAsset*& GetCardOBJ(std::optional<CardType> card_type);
        std::string GetCardGRFpath(std::optional<CardType> card_type);
        std::string GetHwCardGRFpath(std::optional<CardType> card_type);
};

extern GamePartySharedAssets sharedAssetsGameParty;
