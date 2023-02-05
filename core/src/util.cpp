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

std::string nameFromPath(const char* path)
{
    if (path == nullptr || strlen(path) == 0)
        return path;
    const char* dot = strrchr(path, '.');
    const char* slash = strrchr(path, '/');
    if (slash == nullptr)
        slash = strrchr(path, '\\');
    if (slash == nullptr)
        slash = path;

    auto distance = std::distance(slash, dot) - 1;
    std::string str;
    str.resize(distance);
    strncpy(str.data(), slash + 1, distance);

    return str;
}
}   // namespace df