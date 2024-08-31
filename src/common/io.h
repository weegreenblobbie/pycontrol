#pragma once

#include <iostream>
#include <common/types.h>

#define INFO_LOG std::cout << __FILE__ << "(" << __LINE__ << "): INFO: "
#define DEBUG_LOG std::cout << __FILE__ << "(" << __LINE__ << "): DEBUG: "
#define ERROR_LOG std::cerr << __FILE__ << "(" << __LINE__ << "): ERROR: "

#define ABORT_IF(expr, message, return_val) \
    do { \
        if ((expr)) { \
            ERROR_LOG << "ABORT_IF: " << #expr << ": " << message << std::endl; \
        } \
    } while (0)


#define ABORT_IF_NOT(expr, message, return_val) \
    do { \
        if (not (expr)) { \
            ERROR_LOG << "ABORT_IF_NOT: " << #expr << ": " << message << std::endl; \
        } \
    } while (0)

#define ABORT_ON_FAILURE(expr, message, return_val) \
    do { \
        if (pycontrol::result::failure == (expr)) { \
            ERROR_LOG << "ABORT_ON_FILURE: " << #expr << ": " << message << std::endl; \
        } \
    } while (0)
