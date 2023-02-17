//
// Created by josh on 11/30/22.
//

#pragma once

#include "transform.h"
#include "voxel.h"

namespace df::voxel {

class Chunk {
public:
    static constexpr UInt CHUNK_DIM = 32;
    static constexpr UInt CHUNK_SIZE = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;
    Chunk();
    ~Chunk();
    Voxel** operator[](UInt index) { return voxels[index]; }

private:
    Voxel*** voxels = nullptr;

    class Allocator {
        UInt maxChunkCount;
        void* mem = nullptr;
    public:
        static Allocator instance;
        Allocator(UInt maxChunkCount = 1024);
        Voxel*** allocate();
        void free(Voxel*** ptr);
        void init();
        void shutdown();
    };
};

class ChunkManager {
    HashMap<Transform, Chunk> chunks;
};

}   // namespace df::voxel