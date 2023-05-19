//
// Created by josh on 5/17/23.
//

#pragma once
#include <exception>
#include <physfs.h>

namespace dragonfire {

class File {
    PHYSFS_File* fp = nullptr;

public:
    File() = default;
    enum class Mode {
        read,
        write,
        append,
    };
    explicit File(const char* path, Mode mode = Mode::read);

    explicit File(const std::string& path, Mode mode = Mode::read) : File(path.c_str(), mode) {}

    ~File() noexcept;
    void close();

    File(File&) = delete;
    File& operator=(File&) = delete;
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;

    USize length();
    USize readData(void* buf, USize size);

    template<class Alloc = std::allocator<char>>
    std::basic_string<char, std::char_traits<char>, Alloc> readString()
    {
        USize len = length();
        std::basic_string<char, std::char_traits<char>, Alloc> str(len + 1, '\0');
        readData(str.data(), len);
        return str;
    }

    template<class Alloc = std::allocator<UInt&>>
    std::vector<UInt8, Alloc> readData()
    {
        USize len = length();
        std::vector<UInt8, Alloc> data(len);
        USize read = readData(data.data(), len);
        data.resize(read);
        return data;
    }
};

class PhysFSError : public std::exception {
    PHYSFS_ErrorCode err;

public:
    PhysFSError(PHYSFS_ErrorCode errorCode = PHYSFS_getLastErrorCode()) : err(errorCode) {}

    [[nodiscard]] const char* what() const noexcept override;

    [[nodiscard]] PHYSFS_ErrorCode getErr() const { return err; }
};

}   // namespace dragonfire