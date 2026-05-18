#pragma once
#include "globalHeader.hpp"

#ifdef DEBUG_BUILD
#define DEBUG_PRINT(txt) nocashMessage(txt)
#else
#define DEBUG_PRINT(txt) ((void)0)
#endif