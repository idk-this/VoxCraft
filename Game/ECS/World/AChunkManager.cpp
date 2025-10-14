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
    : m_chunkSize(16), m_blockSize(1.0f), m_lastCenterChunk(INT_MAX)
{
    m_worldGenerator = std::make_shared<UWorldGenerator>();
}

AChunkManager::~AChunkManager() {
    Cleanup();
}

glm::ivec3 AChunkManager::WorldToChunkCoord(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / (m_chunkSize * m_blockSize))),
        static_cast<int>(std::floor(worldPos.y / (m_chunkSize * m_blockSize))),
        static_cast<int>(std::floor(worldPos.z / (m_chunkSize * m_blockSize)))
    );
}

glm::vec3 AChunkManager::ChunkToWorldCoord(const glm::ivec3& chunkCoord) const {
    return glm::vec3(
        chunkCoord.x * m_chunkSize * m_blockSize,
        chunkCoord.y * m_chunkSize * m_blockSize,
        chunkCoord.z * m_chunkSize * m_blockSize
    );
}

glm::ivec3 AChunkManager::WorldToBlockCoord(const glm::vec3& worldPos) const {
    glm::ivec3 chunkCoord = WorldToChunkCoord(worldPos);
    glm::vec3 chunkWorldPos = ChunkToWorldCoord(chunkCoord);

    glm::ivec3 result = glm::ivec3(
        static_cast<int>(std::floor((worldPos.x - chunkWorldPos.x) / m_blockSize)),
        static_cast<int>(std::floor((worldPos.y - chunkWorldPos.y) / m_blockSize)),
        static_cast<int>(std::floor((worldPos.z - chunkWorldPos.z) / m_blockSize))
    );
    return result;
}

void AChunkManager::LoadChunk(const glm::ivec3& chunkCoord) {
    ChunkKey key{chunkCoord};

    if (m_loadedChunks.find(key) != m_loadedChunks.end()) {
        return;
    }

    auto chunkActor = Engine::GetCurrentContext().GetWorld()->SpawnActor<AChunk>(chunkCoord, m_worldGenerator.get());
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

    return 0; // Air если чанк не загружен
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

    // Проверяем, изменился ли центр чанков
    if (centerChunk != m_lastCenterChunk) {
        m_lastCenterChunk = centerChunk;
        LoadChunksAround(playerPos, 3, 1);
    }
}


void AChunkManager::Cleanup() {
    for (auto& kv : m_loadedChunks) {
        if (kv.second) {
            Engine::GetCurrentContext().GetWorld()->DestroyActor(kv.second);
        }
    }
    m_loadedChunks.clear();
}