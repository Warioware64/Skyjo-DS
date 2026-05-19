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

    NEA_InitDual3D_DMA();
    NEA_TextureSystemReset(0, 0, static_cast<NEA_VRAMBankFlags>(NEA_VRAM_AB));

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