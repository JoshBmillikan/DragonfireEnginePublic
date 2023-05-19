//
// Created by josh on 5/17/23.
//

#include "file.h"

namespace dragonfire {
File::File(const char* path, File::Mode mode)
{
    switch (mode) {
        case Mode::read: fp = PHYSFS_openRead(path); break;
        case Mode::write: fp = PHYSFS_openWrite(path); break;
        case Mode::append: fp = PHYSFS_openAppend(path); break;
    }
    if (fp == nullptr)
        throw PhysFSError();
}

File::~File() noexcept
{
    if (fp) {
        if (PHYSFS_close(fp) == 0)
            spdlog::error("Error closing file: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
}

void File::close()
{
    if (fp) {
        if (PHYSFS_close(fp) == 0)
            throw PhysFSError();
        fp = nullptr;
    }
}

File::File(File&& other) noexcept
{
    if (this != &other) {
        if (fp) {
            if (PHYSFS_close(fp) == 0)
                spdlog::error(
                        "Error closing file: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
                );
        }
        fp = other.fp;
        other.fp = nullptr;
    }
}

File& File::operator=(File&& other) noexcept
{
    if (this != &other) {
        if (fp) {
            if (PHYSFS_close(fp) == 0)
                spdlog::error(
                        "Error closing file: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
                );
        }
        fp = other.fp;
        other.fp = nullptr;
    }
    return *this;
}

USize File::length()
{
    if (fp) {
        auto len = PHYSFS_fileLength(fp);
        if (len < 0)
            throw PhysFSError();
        return len;
    }
    return 0;
}

USize File::readData(void* buf, USize size)
{
    if (fp) {
        size = std::min(size, length());
        auto read = PHYSFS_readBytes(fp, buf, size);
        if (read < 0)
            throw PhysFSError();
        return read;
    }
    throw PhysFSError(PHYSFS_ERR_NOT_INITIALIZED);
}

const char* PhysFSError::what() const noexcept
{
    return PHYSFS_getErrorByCode(err);
}
}   // namespace dragonfire