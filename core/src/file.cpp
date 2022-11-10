//
// Created by josh on 11/10/22.
//

#include "file.h"

namespace df {
File::File(const char* path, Mode mode)
{
    switch (mode) {
        case read:
            fp = PHYSFS_openRead(path);
            break;
        case write:
            fp = PHYSFS_openWrite(path);
            break;
        case append:
            fp = PHYSFS_openAppend(path);
            break;
    }
    if (fp == nullptr)
        throw std::runtime_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
}
}   // namespace df