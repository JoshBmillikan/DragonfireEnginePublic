//
// Created by josh on 11/17/22.
//
#include "util.h"

namespace df {

const char* getFileExtension(const char* path) noexcept
{
    const char* it = nullptr;
    for (const char* c = path; *c != '\0'; c++) {
        if (*c == '.')
            it = c + 1;
    }
    if (*it == '\0')
        return nullptr;
    return it;
}
}   // namespace df