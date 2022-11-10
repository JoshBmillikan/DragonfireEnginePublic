//
// Created by josh on 11/10/22.
//

#pragma once
#include <physfs.h>

namespace df {

class File {
    PHYSFS_File* fp = nullptr;
public:
    File() = default;
    enum Mode {
        read,
        write,
        append,
    };
    explicit File(const char* path, Mode mode = Mode::read);
    operator bool() const noexcept {return fp != nullptr;}
};

}   // namespace df