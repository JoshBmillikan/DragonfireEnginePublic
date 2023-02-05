//
// Created by josh on 11/17/22.
//

#pragma once

namespace df {

/**
 * @brief Finds the extension of a file path
 *
 * Gets a pointer to the substring after the last '.' in the string.
 * If no '.' is found in the string returns null
 * @param path path to get the extension of
 * @return pointer to the substring after the '.' or null of failed
 */
const char* getFileExtension(const char* path) noexcept;
inline const char* getFileExtension(const std::string_view path) noexcept
{
    return getFileExtension(path.data());
};

inline constexpr Size constStrlen(const char* str) noexcept
{
    Size len = 0;
    for (char c = *str; c != '\0'; c = *++str)
        len++;
    return len;
}

/**
 * @brief Rounds towards -infinity then converts to a integer
 * @param value The value to round
 * @return The rounded value
 */
inline constexpr Int intFloor(const float value) noexcept
{
    Int i = static_cast<Int>(value);
    return value < i ? i - 1 : i;
}

std::string nameFromPath(const char* path);
inline std::string nameFromPath(const std::string& path)
{
    return nameFromPath(path.c_str());
}
}   // namespace df