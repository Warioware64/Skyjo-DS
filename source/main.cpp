#include <stdio.h>
#include <stdlib.h>
#include <nds.h>


#include <vector>
#include <exception>
#include "ErrorHandler.hpp"
#include "Process.hpp"
#include "DebugPrint.hpp"


int main(int argc, char **argv)
{
#ifdef DEBUG_BUILD
    // Print a full register dump on a CPU exception instead of just the
    // "Data abort" title. Debug builds only: it takes over the screens.
    defaultExceptionHandler();
#endif

    DEBUG_PRINT("STARTUP");
    std::set_terminate([]()
    {
        consoleDemoInit();
        //std::exception exceptionStr;
        std::println("Unhandled exception!");
        std::println("{}", error.errorExecp.what());
        std::println("{}", error.errorReason.data());

        for (int i = 0; i < 5; i++)
        {
            std::print(".");
            for (int j = 0; j < 20; j++)
                swiWaitForVBlank();
        }
        std::println();
        std::println("Press START to exit");

        while(1)
        {
            scanKeys();
            uint32_t keys = keysDown();
            if (keys & KEY_START)
            {
                std::println("Prepare to exit");
                for (int i = 0; i < 5; i++)
                {
                    std::print(".");
                    for (int j = 0; j < 60; j++)
                        swiWaitForVBlank();
                }
                exit(0);
            }

        }
    });
    
    process.ProcessInit();

    while(1)
    {
        process.ProcessGame();
    }
    return 0;
}