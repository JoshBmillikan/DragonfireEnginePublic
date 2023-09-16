//
// Created by josh on 6/12/23.
//

#pragma once
#include "voxel.h"
#include <future>
#include "vertex.h"

namespace dragonfire {
using MeshHandle = std::uintptr_t;

class Chunk {
public:
    static constexpr UInt CHUNK_DIM = 16;
    static constexpr UInt CHUNK_SIZE = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

    Chunk(float xIn, float yIn);

    std::future<std::vector<Vertex>> genVertices();

private:
    Voxel voxels[CHUNK_DIM][CHUNK_DIM][CHUNK_DIM];
};

}   // namespace dragonfire