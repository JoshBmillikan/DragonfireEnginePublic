//
// Created by josh on 11/30/22.
//

#include "voxel/chunk.h"

namespace df::voxel {
Chunk::Chunk()
{
    memset(voxels, 0, sizeof(Voxel) * VOXEL_COUNT);
}
}   // namespace df::voxel