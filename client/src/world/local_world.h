//
// Created by josh on 12/1/22.
//

#pragma once

#include "world/world.h"
namespace df {

class LocalWorld : public World {
public:
    LocalWorld(std::string&& name, ULong seed);
    ~LocalWorld() noexcept override;
    DF_NO_MOVE_COPY(LocalWorld);
    void update(double delta) override;
};

}   // namespace df