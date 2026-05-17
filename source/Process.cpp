#include "Process.hpp"


Process::Process()
{

}
Process::~Process()
{

}

void Process::ProcessInit()
{

    if (!nitroFSInit(NULL))
    {
        error.errorReason.assign("NitroFile filesystem failed!");
        std::terminate();
    }

}
void Process::ProcessGame()
{
    
}


Process process;