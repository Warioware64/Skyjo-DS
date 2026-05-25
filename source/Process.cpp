#include "Process.hpp"
#include "DebugPrint.hpp"
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


    NEA_Hw2DAutoInit();

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
            error.errorReason.assign("HERE IS CRASH LOL");
            std::terminate();
        }
    }
    else if(this->classstates == ClassStates::Playing)
    {
        if (this->menustates == MenusStates::MainMenu)
        {
            mainmenu.RenderMainMenu();
        }
    }
    //std::terminate();
}


Process process;