//
// Created by josh on 5/23/23.
//

#pragma once
#include <model.h>
#include "allocation.h"
#include <shared_mutex>

namespace dragonfire {
class MeshHandle {
    UInt32 begin, end;
};

class MeshRegistry {
    Buffer vertexBuffer, indexBuffer;
    std::shared_mutex mutex;
};

}   // namespace dragonfire