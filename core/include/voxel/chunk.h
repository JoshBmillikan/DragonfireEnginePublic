//
// Created by josh on 11/30/22.
//

#pragma once

#include "transform.h"
#include "voxel.h"

namespace df::voxel {

class Chunk {
public:
    static constexpr Int CHUNK_DIM = 32;
    static constexpr Int VOXEL_COUNT = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;
    Chunk();

private:
    Voxel voxels[CHUNK_DIM][CHUNK_DIM][CHUNK_DIM];
};

class ChunkManager {
    HashMap<Transform, Chunk> chunks;
};

}   // namespace df::voxel