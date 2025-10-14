#include "UWorldGenerator.h"
#include <cmath>
#include <random>

UWorldGenerator::UWorldGenerator(uint64_t seed) : m_seed(seed) {}

void UWorldGenerator::SetSeed(uint64_t seed) {
    m_seed = seed;
}

uint64_t UWorldGenerator::GetSeed() const {
    return m_seed;
}

float UWorldGenerator::HashNoise(int x, int z) const {
    uint64_t h = std::hash<uint64_t>()(((uint64_t)(int64_t)x << 32) ^ (uint64_t)(int64_t)z ^ m_seed);
    h ^= (h >> 33);
    h *= 0xff51afd7ed558ccdULL;
    h ^= (h >> 33);
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= (h >> 33);
    return (h & 0xFFFFFF) / float(0xFFFFFF);
}

BiomeCell UWorldGenerator::GenerateCell(int x, int z) const {
    auto smoothNoise = [&](float fx, float fz, float scale) {
        int xi = int(std::floor(fx / scale));
        int zi = int(std::floor(fz / scale));

        float dx = (fx / scale) - xi;
        float dz = (fz / scale) - zi;

        float n00 = HashNoise(xi,     zi);
        float n10 = HashNoise(xi + 1, zi);
        float n01 = HashNoise(xi,     zi + 1);
        float n11 = HashNoise(xi + 1, zi + 1);

        auto lerp = [](float a, float b, float t) {
            return a + (b - a) * t;
        };

        float nx0 = lerp(n00, n10, dx);
        float nx1 = lerp(n01, n11, dx);
        return lerp(nx0, nx1, dz);
    };

    // несколько октав шума
    float h = 0.0f;
    h += smoothNoise((float)x, (float)z, 64.0f) * 0.6f;
    h += smoothNoise((float)x, (float)z, 32.0f) * 0.3f;
    h += smoothNoise((float)x, (float)z, 16.0f) * 0.1f;

    if (h < 0) h = 0;
    if (h > 1) h = 1;

    int worldHeight = 32;
    int height = std::clamp(1 + int(h * (worldHeight - 2)), 1, worldHeight - 1);

    BiomeCell cell;
    cell.height  = height;
    cell.surface = 3; // Grass
    cell.filler  = 2; // Dirt
    cell.stone   = 4; // Stone
    return cell;
}

std::vector<uint8_t> UWorldGenerator::GenerateChunkBlocks(glm::ivec3 chunkCoord, int chunkSize, int worldHeight) {
    std::vector<uint8_t> blocks((size_t)chunkSize * (size_t)chunkSize * (size_t)worldHeight, 0);

    int baseX = chunkCoord.x * chunkSize;
    int baseZ = chunkCoord.z * chunkSize;
    int baseY = chunkCoord.y * worldHeight;

    for (int x = 0; x < chunkSize; ++x) {
        for (int z = 0; z < chunkSize; ++z) {
            BiomeCell cell = GenerateCell(baseX + x, baseZ + z);

            for (int y = 0; y < worldHeight; ++y) {
                int worldY = baseY + y;
                if (worldY < 0 || worldY >= worldHeight) continue;

                uint8_t blockId = 0;
                if (worldY == 0) {
                    blockId = 1;
                } else if (worldY > cell.height) {
                    blockId = 0;
                } else if (worldY < cell.height - 3) {
                    blockId = cell.stone;
                } else if (worldY < cell.height) {
                    blockId = cell.filler;
                } else {
                    blockId = cell.surface;
                }

                size_t idx = (size_t)x + (size_t)chunkSize * ((size_t)z * (size_t)worldHeight + (size_t)y);
                blocks[idx] = blockId;
            }
        }
    }

    return blocks;
}
