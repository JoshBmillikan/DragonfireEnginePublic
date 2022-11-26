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

inline constexpr Size constStrlen(const char* str) noexcept {
    Size len = 0;
    for (char c = *str; c != '\0'; c = *++str)
        len++;
    return len;
}
}   // namespace df