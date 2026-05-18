#include "DebugPrint.hpp"

#ifdef DEBUG_BUILD
DebugPrint::DebugPrint()
{
    consoleDebugInit(DebugDevice_NOCASH);
}

DebugPrint::~DebugPrint()
{

}

void DebugPrint::DebugPrintf(const std::string &txt)
{
    std::println(txt);
}
DebugPrint debugprint;
#endif