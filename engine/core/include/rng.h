//
// Created by josh on 5/31/23.
//

#pragma once

namespace dragonfire {

class Rng {
    UInt64 s[4]{};
public:
    explicit Rng(UInt64 seed);
    Rng();
    UInt64 next();
    double nextDouble();
};

}   // namespace dragonfire