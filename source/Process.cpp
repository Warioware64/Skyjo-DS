#include "Process.hpp"
#include "DebugPrint.hpp"


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
    
}
void Process::ProcessGame()
{
    //std::terminate();
}


Process process;