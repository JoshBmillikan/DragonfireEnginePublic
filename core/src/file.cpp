//
// Created by josh on 11/10/22.
//

#include "file.h"

namespace df {
File::File(const char* path, Mode mode)
{
    switch (mode) {
        case read: fp = PHYSFS_openRead(path); break;
        case write: fp = PHYSFS_openWrite(path); break;
        case append: fp = PHYSFS_openAppend(path); break;
    }
    if (fp == nullptr)
        throw std::runtime_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
}

std::string File::readToString()
{
    auto len = length();
    std::string str;
    str.resize(len + 1, '\0');
    if (PHYSFS_readBytes(fp, str.data(), len) < 0)
        throw PhysFsException();
    return str;
}

ULong File::length() const
{
    auto len = PHYSFS_fileLength(fp);
    if (len < 0)
        throw PhysFsException();
    return len;
}

void File::writeString(const char* str)
{
    auto len = strlen(str);
    if (PHYSFS_writeBytes(fp, str, len) < 0)
        throw PhysFsException();
}

void File::close()
{
    if (PHYSFS_close(fp) == 0)
        throw PhysFsException();
    fp = nullptr;
}

File::~File() noexcept
{
    if (fp) {
        if (PHYSFS_close(fp) == 0)
            spdlog::error("Error closing PhysFs file: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
}

File::File(File&& other) noexcept
{
    if (this != &other) {
        fp = other.fp;
        other.fp = nullptr;
    }
}

File& File::operator=(File&& other) noexcept
{
    if (this != &other) {
        fp = other.fp;
        other.fp = nullptr;
    }
    return *this;
}

void File::readBytes(void* buffer, Size len)
{
    auto readLength = std::min(length(), len);
    if (PHYSFS_readBytes(fp, buffer, readLength) < 0)
        throw PhysFsException();
}

std::vector<UByte> File::readBytes()
{
    auto len = length();
    std::vector<UByte> data;
    data.resize(len);
    if (PHYSFS_readBytes(fp, data.data(), len) < 0)
        throw PhysFsException();
    return data;
}

void File::writeBytes(void* data, Size len)
{
    if (PHYSFS_writeBytes(fp, data, len) < 0)
        throw PhysFsException();
}

}   // namespace df