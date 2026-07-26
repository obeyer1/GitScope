#pragma once

#include <cstdio>
#include <cstdlib>

// Minimal assertion macro for the headless test executables (no framework
// dependency, so CI needs nothing beyond the build itself).
#define REQUIRE(cond)                                                                              \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__);              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
