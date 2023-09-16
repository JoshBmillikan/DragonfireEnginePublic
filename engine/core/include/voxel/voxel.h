//
// Created by josh on 6/12/23.
//

#pragma once

namespace dragonfire {

class Voxel {
    static constexpr USize ID_BIT_COUNT = 14;

public:
    static constexpr USize MAX_VOXEL_ID = 1 << ID_BIT_COUNT;

    UInt16 id : ID_BIT_COUNT;
    UInt16 sold : 1;
};

}   // namespace dragonfire