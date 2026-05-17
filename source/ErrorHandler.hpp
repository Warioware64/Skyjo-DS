#pragma once

#include "globalHeader.hpp"

class ErrorHandler
{
    public:
        ErrorHandler();
        ~ErrorHandler();

        std::exception errorExecp;

        std::string errorReason;
};

extern ErrorHandler error;