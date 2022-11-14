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
    operator bool() const noexcept { return fp != nullptr; }
    [[nodiscard]] ULong length() const;
    std::string readToString();
    void writeString(const char* str);
    void writeString(const std::string& str) {
        writeString(str.c_str());
    }
    void close();
    ~File() noexcept;
    DF_NO_COPY(File);
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;
};

class PhysFsException : public std::runtime_error {
    PHYSFS_ErrorCode errorCode;

public:
    PhysFsException(const char* msg, PHYSFS_ErrorCode errorCode = PHYSFS_getLastErrorCode())
        : std::runtime_error(fmt::format("{}, PhysFs error: {}", msg, PHYSFS_getErrorByCode(errorCode))), errorCode(errorCode)
    {
    }
    PhysFsException(PHYSFS_ErrorCode errorCode = PHYSFS_getLastErrorCode())
        : std::runtime_error(PHYSFS_getErrorByCode(errorCode)), errorCode(errorCode)
    {
    }
    [[nodiscard]] PHYSFS_ErrorCode code() const noexcept {return errorCode;}
};
}   // namespace df