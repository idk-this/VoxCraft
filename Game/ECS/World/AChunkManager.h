//
// Created by IDKTHIS on 12.10.2025.
//

#pragma once

#include <unordered_map>
#include <glm/vec3.hpp>
#include "AChunk.h"

class UWorldGenerator;

struct ChunkKey {
    glm::ivec3 coord;

    bool operator==(const ChunkKey& other) const {
        return coord == other.coord;
    }
};

struct ChunkKeyHash {
    size_t operator()(const ChunkKey& key) const {
        return std::hash<int64_t>()(
            (static_cast<int64_t>(key.coord.x) << 42) ^
            (static_cast<int64_t>(key.coord.y) << 21) ^
            static_cast<int64_t>(key.coord.z)
        );
    }
};
struct RaycastResult {
    bool hit = false;
    glm::ivec3 blockCoord;
    glm::ivec3 normal;
    AChunk* chunk = nullptr;
    float distance = 0.0f;
};

class AChunkManager : public AActor{
public:
    AChunkManager();
    ~AChunkManager();

    void LoadChunk(const glm::ivec3& chunkCoord);
    void UnloadChunk(const glm::ivec3& chunkCoord);
    void LoadChunksAround(const glm::vec3& centerPos, int horizontalRadius, int verticalRadius);
    glm::ivec3 WorldToBlockCoord(const glm::vec3& worldPos) const;
    std::shared_ptr<AChunk> GetChunk(const glm::ivec3& chunkCoord) const;
    uint8_t GetBlock(const glm::vec3& worldPos) const;
    void SetBlock(const glm::vec3& worldPos, uint8_t blockId);
    glm::ivec3 GetLastCenterChunk() const { return m_lastCenterChunk; }
    glm::ivec3 WorldToChunkCoord(const glm::vec3& worldPos) const;
    void Update(float deltaTime, const glm::vec3& playerPos);
    RaycastResult PerformRaycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance);
    void Cleanup();

private:
    int m_chunkSize_w = 16;
    int m_chunkSize_h = 128;
    int m_chunkSize_d = 16;
    std::shared_ptr<UWorldGenerator> m_worldGenerator;
    int m_renderRadius = 3;

    std::unordered_map<ChunkKey, std::shared_ptr<AChunk>, ChunkKeyHash> m_loadedChunks;
    glm::ivec3 m_lastCenterChunk;


    glm::vec3 ChunkToWorldCoord(const glm::ivec3& chunkCoord) const;
};