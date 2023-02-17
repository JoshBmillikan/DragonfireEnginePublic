//
// Created by josh on 11/30/22.
//

#include "voxel/chunk.h"

namespace df::voxel {
Chunk::Chunk()
{
    voxels = Allocator::instance.allocate();
}

Chunk::~Chunk()
{
    Allocator::instance.free(voxels);
}

Chunk::Allocator Chunk::Allocator::instance = Chunk::Allocator();

Chunk::Allocator::Allocator(UInt maxChunkCount) : maxChunkCount(maxChunkCount)
{
}

Voxel*** Chunk::Allocator::allocate()
{
    return nullptr;
}

void Chunk::Allocator::free(Voxel*** ptr)
{
    if (ptr) {
        memset(ptr, 0, CHUNK_SIZE * sizeof(Voxel));
        // todo
    }
}

void Chunk::Allocator::init()
{
    mem = calloc(CHUNK_SIZE * maxChunkCount, sizeof(Voxel));
    if (mem == nullptr)
        throw std::bad_alloc();
}

void Chunk::Allocator::shutdown()
{
    ::free(mem);
}

}   // namespace df::voxel