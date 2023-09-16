//
// Created by josh on 6/12/23.
//

#include "voxel/chunk.h"

#include "utility.h"
#include <noise.h>

namespace dragonfire {
Chunk::Chunk(float xIn, float yIn) : voxels{}
{
    for (Int x = 0; x < CHUNK_DIM; x++) {
        for (Int y = 0; y < CHUNK_DIM; y++) {
            for (Int z = 0; z < CHUNK_DIM; z++) {
                float perlin = noise::perlin(float(x) + xIn, float(y) + yIn) * 0.5f + 0.5f;
                perlin *= CHUNK_DIM;
                voxels[x][y][z].id = perlin > float(z) > 0 ? 1 : 0;
            }
        }
    }
}

std::future<std::vector<Vertex>> Chunk::genVertices()
{
    return GLOBAL_THREAD_POOL.submit([this] {
        std::vector<Vertex> vertices;



        return vertices;
    });
}

}   // namespace dragonfire