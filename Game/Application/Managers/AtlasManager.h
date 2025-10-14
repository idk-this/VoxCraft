//
// Created by IDKTHIS on 28.09.2025.
//

#pragma once
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Core/Utils/FileLoaders/ImageLoader.h"

class UTexture;
struct BlockFaceUV {
    glm::vec2 uv0;
    glm::vec2 uv1;
};

struct BlockAtlasInfo {
    BlockFaceUV faces[6];
};

struct BlockDefinition {
    std::string blockId;
    std::string displayName;
    std::string texturePath;
};

class AtlasManager {
public:
    static AtlasManager& Get();

    bool Initialize();

    void RegisterBlock(const std::string& blockId, const std::string& texturePath);
    uint8_t GetBlockId(const std::string& blockId) const;
    std::string GetBlockName(uint8_t id) const;

    bool LoadFromFolder(const std::string& folderPath);

    const BlockAtlasInfo& GetBlockAtlasInfo(uint8_t blockId) const;
    std::vector<std::shared_ptr<UTexture>> GetAtlasTextures() const { return m_atlasTextures; }
    uint32_t GetAtlasPageCount() const { return m_atlasTextures.size(); }
    int GetAtlasWidth() const { return m_atlasWidth; }
    int GetAtlasHeight() const { return m_atlasHeight; }

private:
    AtlasManager() = default;

    bool BuildCombinedAtlas();

    std::vector<BlockDefinition> m_blockDefinitions;
    std::unordered_map<std::string, uint8_t> m_blockNameToId;
    std::unordered_map<uint8_t, std::string> m_idToBlockName;

    std::vector<BlockAtlasInfo> m_blockAtlasInfos;
    std::vector<std::shared_ptr<UTexture>> m_atlasTextures;
    int m_atlasWidth = 0;
    int m_atlasHeight = 0;
    int m_atlasChannels = 0;

    bool m_initialized = false;
};
