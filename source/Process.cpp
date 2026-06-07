#include "Process.hpp"
#include "DebugPrint.hpp"
#include "GameParty.hpp"
#include "MainMenu.hpp"
#include "globalHeader.hpp"


Process::Process()
{

}
Process::~Process()
{

}

void Process::CallInitializationOnePlayerParty(int cpu_number, CPULevel cpu_level)
{
   this->classstates = ClassStates::Init;
   this->menustates = MenusStates::PartyGameOnePlayer; 

   this->cpu_number_arg = cpu_number;
   this->cpu_level_arg = cpu_level;
   this->party_type_arg = PartyType::OnePlayerCPU;

}

void Process::ProcessInit()
{
    irqEnable(IRQ_HBLANK);
    irqSet(IRQ_VBLANK, NEA_VBLFunc);
    irqSet(IRQ_HBLANK, NEA_HBLFunc);
    
    if (!nitroFSInit(NULL))
    {
        error.errorReason.assign("NitroFile filesystem failed!");
        std::terminate();
    }
    DEBUG_PRINT("Nitrofiles successful");

    NEA_Init3D();
    NEA_MainScreenSetOnBottom();

    NEA_SetTexPaletteBank(static_cast<NEA_VRAMBankFlags>(NEA_VRAM_F | NEA_VRAM_G));
    NEA_TextureSystemReset(0, 0, static_cast<NEA_VRAMBankFlags>(NEA_VRAM_AB));

    // Explicit 2D bank layout. Sub OBJ goes on bank D (128 KB) instead of the
    // default I (16 KB) so we can hold every per-CardType OBJ asset at once
    // without exhausting tile VRAM.
    NEA_Hw2DVRAMConfig hw2dCfg = {};
    hw2dCfg.main_bg  = NEA_VRAM_E;
    hw2dCfg.main_obj = static_cast<NEA_VRAMBankFlags>(0);
    hw2dCfg.sub_bg   = NEA_VRAM_H;
    hw2dCfg.sub_obj  = static_cast<NEA_VRAMBankFlags>(NEA_VRAM_D);
    if (NEA_Hw2DInit(&hw2dCfg) != 0)
    {
        error.errorReason.assign("NEA_Hw2DInit failed: bad bank config");
        std::terminate();
    }

    swiWaitForVBlank();
    swiWaitForVBlank();
    char name[50];
    utf16_to_utf8(name, sizeof(name), (char16_t *)PersonalData->name,
                    PersonalData->nameLen * sizeof(char16_t));
    consoleUserName = name;
    DEBUG_PRINT("Enter into intro sequence");

    intro.LoadAssetsIntro();
    intro.RenderIntro();
    intro.UnloadAssetsIntro();
    
    DEBUG_PRINT("Exit intro sequence");
    this->classstates = ClassStates::Init;
    this->menustates = MenusStates::MainMenu;

    
}
void Process::ProcessGame()
{
    if (this->classstates == ClassStates::Init)
    {
        if (this->menustates == MenusStates::MainMenu)
        {
            mainmenu.LoadAssetsMainMenu();
            this->classstates = ClassStates::Playing;
        }
        else if (this->menustates == MenusStates::PartyGameOnePlayer)
        {
            //error.errorReason.assign("HERE IS CRASH LOL");
            //std::terminate();
            gameparty.InitGamePartySituation(this->cpu_number_arg, this->cpu_level_arg, this->party_type_arg);
            this->classstates = ClassStates::Playing;
        }
    }
    else if(this->classstates == ClassStates::Playing)
    {
        if (this->menustates == MenusStates::MainMenu)
        {
            mainmenu.RenderMainMenu();
        }
        else if (this->menustates == MenusStates::PartyGameOnePlayer)
        {
            gameparty.RenderGameParty();
        }
    }
    //std::terminate();
}


Process process;
