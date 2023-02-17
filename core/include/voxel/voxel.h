//
// Created by josh on 11/30/22.
//

#pragma once

namespace df::voxel {

class Voxel {
    static constexpr int BITS = 15;

public:
    static constexpr UShort MAX_ID = (1 << 15) - 1;
    UShort id : BITS;
    bool smooth : 1;
    [[nodiscard]] class VoxelType& type() const;
    Voxel(UShort id, bool smooth = false) : id(id), smooth(smooth) {}
};

class VoxelType {
public:
    static VoxelType& get(UShort id) { return registry[id]; }
    static VoxelType& get(Voxel id) { return get(id.id); }
    static UShort registerVoxelType(VoxelType&& voxel);

private:
    static std::vector<VoxelType> registry;
};

}   // namespace df::voxel