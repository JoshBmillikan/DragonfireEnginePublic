//
// Created by josh on 11/30/22.
//

#pragma once

namespace df {

class Rng {
    ULong state[4]{};

public:
    Rng(ULong seed);
    ULong nextULong();
    Long nextLong() { return std::bit_cast<Long>(nextULong()); }
    UInt nextUInt() { return (Long) (nextULong() >> 32); }
    Int nextInt() { return (Int) (nextLong() >> 32); }
    UShort nextUShort() { return (UShort) (nextULong() >> 16); }
    Short nextShort() { return (Short) (nextLong() >> 16); }
    UByte nextUByte() { return (UByte) (nextULong() >> 8); }
    Byte nextByte() { return (Byte) (nextLong() >> 8); }
    bool nextBool() { return nextLong() < 0; }

    double nextDouble();
    float nextFloat() { return (float) nextDouble(); }
};

}   // namespace df