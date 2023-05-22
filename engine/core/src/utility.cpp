//
// Created by josh on 5/18/23.
//

#include "utility.h"
#include "allocators.h"
#include "file.h"
#include <nlohmann/json.hpp>

namespace dragonfire {
BS::thread_pool GLOBAL_THREAD_POOL;

nlohmann::json loadJson(const char* path)
{
    File file(path);
    TempString str = file.readString<FrameAllocator<char>>();
    return nlohmann::json::parse(str);
}
}   // namespace dragonfire