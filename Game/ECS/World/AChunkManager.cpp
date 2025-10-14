//
// Created by IDKTHIS on 12.10.2025.
//

#include "AChunkManager.h"

#include <unordered_set>

#include "Application/VoxCraftGame.h"
#include "Core/ECS/Base/UWorld.h"
#include "Core/Log/Logger.h"
#include "Generators/UWorldGenerator.h"


AChunkManager::AChunkManager()
    : m_lastCenterChunk(INT_MAX)
{
    m_worldGenerator = std::make_shared<UWorldGenerator>();
}

AChunkManager::~AChunkManager() {
    Cleanup();
}

glm::ivec3 AChunkManager::WorldToChunkCoord(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / m_chunkSize_w)),
        static_cast<int>(std::floor(worldPos.y / m_chunkSize_h)),
        static_cast<int>(std::floor(worldPos.z / m_chunkSize_d))
    );
}

glm::vec3 AChunkManager::ChunkToWorldCoord(const glm::ivec3& chunkCoord) const {
    return glm::vec3(
        chunkCoord.x * m_chunkSize_w,
        chunkCoord.y * m_chunkSize_h,
        chunkCoord.z * m_chunkSize_d
    );
}

glm::ivec3 AChunkManager::WorldToBlockCoord(const glm::vec3& worldPos) const {
    glm::ivec3 chunkCoord = WorldToChunkCoord(worldPos);
    glm::vec3 chunkWorldPos = ChunkToWorldCoord(chunkCoord);

    glm::vec3 local = worldPos - chunkWorldPos;

    return glm::ivec3(
        static_cast<int>(std::floor(local.x)),
        static_cast<int>(std::floor(local.y)),
        static_cast<int>(std::floor(local.z))
    );
}

void AChunkManager::LoadChunk(const glm::ivec3& chunkCoord) {
    ChunkKey key{chunkCoord};

    if (m_loadedChunks.find(key) != m_loadedChunks.end()) {
        return;
    }

    auto chunkActor = Engine::GetCurrentContext().GetWorld()->SpawnActor<AChunk>(chunkCoord, m_chunkSize_w, m_chunkSize_h, m_chunkSize_d, m_worldGenerator.get());
    if (chunkActor) {
        glm::vec3 worldPos = ChunkToWorldCoord(chunkCoord);
        if (auto tr = chunkActor->GetComponent<UTransformComponent>()) {
            tr->SetPosition(worldPos);
        }
        m_loadedChunks[key] = chunkActor;
        LOG_INFO("AChunkManager", "Loaded chunk at ({}, {}, {})", chunkCoord.x, chunkCoord.y, chunkCoord.z);
    }
}

void AChunkManager::UnloadChunk(const glm::ivec3& chunkCoord) {
    ChunkKey key{chunkCoord};
    auto it = m_loadedChunks.find(key);
    if (it == m_loadedChunks.end()) return;

    if (it->second) {
         Engine::GetCurrentContext().GetWorld()->DestroyActor(it->second);
    }
    m_loadedChunks.erase(it);
    LOG_INFO("AChunkManager", "Unloaded chunk at ({}, {}, {})", chunkCoord.x, chunkCoord.y, chunkCoord.z);
}

void AChunkManager::LoadChunksAround(const glm::vec3& centerPos, int horizontalRadius, int verticalRadius) {
    if (!m_worldGenerator) return;

    glm::ivec3 centerChunk = WorldToChunkCoord(centerPos);

    std::unordered_set<ChunkKey, ChunkKeyHash> desired;
    for (int dz = -m_renderRadius; dz <= m_renderRadius; ++dz) {
        for (int dx = -m_renderRadius; dx <= m_renderRadius; ++dx) {
            desired.emplace(ChunkKey{ glm::ivec3(centerChunk.x + dx, 0, centerChunk.z + dz) });
        }
    }

    std::unordered_set<ChunkKey, ChunkKeyHash> loadedKeys;
    for (const auto &kv : m_loadedChunks) loadedKeys.insert(kv.first);

    for (const auto &k : desired) {
        if (loadedKeys.find(k) == loadedKeys.end()) LoadChunk(k.coord);
    }
    for (const auto &k : loadedKeys) {
        if (desired.find(k) == desired.end()) UnloadChunk(k.coord);
    }
}


std::shared_ptr<AChunk> AChunkManager::GetChunk(const glm::ivec3& chunkCoord) const {
    ChunkKey key{chunkCoord};
    auto it = m_loadedChunks.find(key);
    return it != m_loadedChunks.end() ? it->second : nullptr;
}

uint8_t AChunkManager::GetBlock(const glm::vec3& worldPos) const {
    glm::ivec3 chunkCoord = WorldToChunkCoord(worldPos);
    glm::ivec3 blockCoord = WorldToBlockCoord(worldPos);

    auto chunk = GetChunk(chunkCoord);
    if (chunk) {
        return chunk->GetBlock(blockCoord.x, blockCoord.y, blockCoord.z);
    }

    return 0;
}

void AChunkManager::SetBlock(const glm::vec3& worldPos, uint8_t blockId) {
    glm::ivec3 chunkCoord = WorldToChunkCoord(worldPos);
    glm::ivec3 blockCoord = WorldToBlockCoord(worldPos);

    auto chunk = GetChunk(chunkCoord);
    if (chunk) {
        chunk->SetBlock(blockCoord.x, blockCoord.y, blockCoord.z, blockId);
    }
}

void AChunkManager::Update(float deltaTime, const glm::vec3& playerPos) {
    glm::ivec3 centerChunk = WorldToChunkCoord(playerPos);
    if (centerChunk != m_lastCenterChunk) {
        m_lastCenterChunk = centerChunk;
        LoadChunksAround(playerPos, 3, 1);
    }
}

RaycastResult AChunkManager::PerformRaycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance)
{
    RaycastResult result;

    glm::vec3 rayDir = glm::normalize(direction);
    glm::vec3 rayPos = start;

    glm::ivec3 voxel(
        static_cast<int>(std::floor(rayPos.x)),
        static_cast<int>(std::floor(rayPos.y)),
        static_cast<int>(std::floor(rayPos.z))
    );

    glm::vec3 rayStep(
        (rayDir.x > 0) ? 1 : -1,
        (rayDir.y > 0) ? 1 : -1,
        (rayDir.z > 0) ? 1 : -1
    );

    glm::vec3 nextBoundary = glm::vec3(
        (rayStep.x > 0) ? (voxel.x + 1) : voxel.x,
        (rayStep.y > 0) ? (voxel.y + 1) : voxel.y,
        (rayStep.z > 0) ? (voxel.z + 1) : voxel.z
    );

    glm::vec3 tMax = glm::vec3(
        (rayDir.x != 0) ? (nextBoundary.x - rayPos.x) / rayDir.x : std::numeric_limits<float>::max(),
        (rayDir.y != 0) ? (nextBoundary.y - rayPos.y) / rayDir.y : std::numeric_limits<float>::max(),
        (rayDir.z != 0) ? (nextBoundary.z - rayPos.z) / rayDir.z : std::numeric_limits<float>::max()
    );

    glm::vec3 tDelta = glm::vec3(
        (rayDir.x != 0) ? rayStep.x / rayDir.x : std::numeric_limits<float>::max(),
        (rayDir.y != 0) ? rayStep.y / rayDir.y : std::numeric_limits<float>::max(),
        (rayDir.z != 0) ? rayStep.z / rayDir.z : std::numeric_limits<float>::max()
    );

    glm::ivec3 lastVoxel = voxel;
    float traveled = 0.0f;

    while (traveled < maxDistance) {
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            traveled = tMax.x;
            tMax.x += tDelta.x;
            voxel.x += static_cast<int>(rayStep.x);
        } else if (tMax.y < tMax.z) {
            traveled = tMax.y;
            tMax.y += tDelta.y;
            voxel.y += static_cast<int>(rayStep.y);
        } else {
            traveled = tMax.z;
            tMax.z += tDelta.z;
            voxel.z += static_cast<int>(rayStep.z);
        }

        if (traveled > maxDistance) break;

        rayPos = start + rayDir * traveled;
        glm::ivec3 chunkCoord = WorldToChunkCoord(glm::vec3(voxel.x, voxel.y, voxel.z));

        auto chunk = GetChunk(chunkCoord);
        if (!chunk) continue;

        glm::ivec3 blockCoord = WorldToBlockCoord(glm::vec3(voxel.x, voxel.y, voxel.z));

        if (blockCoord.x >= 0 && blockCoord.x < m_chunkSize_w &&
            blockCoord.y >= 0 && blockCoord.y < m_chunkSize_h &&
            blockCoord.z >= 0 && blockCoord.z < m_chunkSize_d) {

            if (chunk->GetBlock(blockCoord.x, blockCoord.y, blockCoord.z) != 0) {
                result.hit = true;
                result.blockCoord = blockCoord;
                result.chunk = chunk.get();
                result.distance = traveled;
                if (voxel.x != lastVoxel.x) {
                    result.normal = glm::ivec3((voxel.x > lastVoxel.x) ? -1 : 1, 0, 0);
                } else if (voxel.y != lastVoxel.y) {
                    result.normal = glm::ivec3(0, (voxel.y > lastVoxel.y) ? -1 : 1, 0);
                } else {
                    result.normal = glm::ivec3(0, 0, (voxel.z > lastVoxel.z) ? -1 : 1);
                }

                return result;
            }
        }

        lastVoxel = voxel;
    }

    return result;
}


void AChunkManager::Cleanup() {
    for (auto& kv : m_loadedChunks) {
        if (kv.second) {
            Engine::GetCurrentContext().GetWorld()->DestroyActor(kv.second);
        }
    }
    m_loadedChunks.clear();
}
