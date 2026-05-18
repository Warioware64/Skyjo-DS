#pragma once
#include "globalHeader.hpp"

#ifdef DEBUG_BUILD
class DebugPrint
{
    public:
        DebugPrint();
        ~DebugPrint();

        void DebugPrintf(const std::string &txt);
};
extern DebugPrint debugprint;
#endif