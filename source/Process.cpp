#include "Process.hpp"


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

    NEA_InitDual3D_DMA();
    NEA_TextureSystemReset(0, 0, static_cast<NEA_VRAMBankFlags>(NEA_VRAM_AB));

    intro.LoadAssetsIntro();
    intro.RenderIntro();
    
}
void Process::ProcessGame()
{
    
}


Process process;