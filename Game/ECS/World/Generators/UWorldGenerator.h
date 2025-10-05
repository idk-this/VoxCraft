//
// Created by IDKTHIS on 29.09.2025.
//

#pragma once
#include <vector>

#include "Core/ECS/Base/UObject.h"
#include "glm/vec3.hpp"

struct BiomeCell {
    int height;
    uint8_t surface;
    uint8_t filler;
    uint8_t stone;
};
class UWorldGenerator : public UObject {
    UCLASS(UWorldGenerator);
public:
    UWorldGenerator(uint64_t seed = 1337);

    void SetSeed(uint64_t seed);
    uint64_t GetSeed() const;
    float HashNoise(int x, int z) const;
    void GenerateWorld(int worldSizeX, int worldSizeZ, int worldHeight) {};

    std::vector<uint8_t> GenerateChunkBlocks(glm::ivec3 chunkCoord, int chunkSize, int worldHeight);

private:
    uint64_t m_seed;

    std::unordered_map<int64_t, BiomeCell> m_biomeMap;

    static int64_t Key(int x, int z) {
        return (static_cast<int64_t>(x) << 32) ^ (z & 0xffffffff);
    }

    BiomeCell GenerateCell(int x, int z) const;
};
