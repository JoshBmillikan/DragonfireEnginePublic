//
// Created by josh on 11/30/22.
//

#include "voxel/voxel.h"

namespace df::voxel {
std::vector<VoxelType> VoxelType::registry;

class VoxelType& Voxel::type() const
{
    return VoxelType::get(id);
}

UShort VoxelType::registerVoxelType(VoxelType&& voxel)
{
    if (registry.size() < Voxel::MAX_ID) {
        registry.emplace_back(voxel);
        return registry.size();
    }
    throw std::out_of_range(
            fmt::format("To many voxel types registered, no more than {} voxels may be registered", Voxel::MAX_ID)
    );
}
}   // namespace df::voxel