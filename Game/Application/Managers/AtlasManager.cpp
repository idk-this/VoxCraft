//
// Created by IDKTHIS on 28.09.2025.
//

#include "AtlasManager.h"

#include <cstring>
#include <nlohmann/json.hpp>

#include "Application/VoxCraftGame.h"
#include "Core/ECS/Resources/UTexture.h"
#include "Core/Log/Logger.h"

AtlasManager& AtlasManager::Get() {
    static AtlasManager instance;
    return instance;
}

bool AtlasManager::Initialize() {
    if (m_initialized) return true;

    m_initialized = true;
    return true;
}

void AtlasManager::RegisterBlock(const std::string& blockId, const std::string& texturePath) {
    if (m_blockNameToId.find(blockId) != m_blockNameToId.end()) {
        LOG_WARN("AtlasManager", "Block {} already registered", blockId);
        return;
    }

    uint8_t newId = static_cast<uint8_t>(m_blockDefinitions.size() + 1);
    m_blockDefinitions.push_back({blockId, blockId, texturePath});
    m_blockNameToId[blockId] = newId;
    m_idToBlockName[newId] = blockId;

    LOG_INFO("AtlasManager", "Registered block {} with ID {} and texture {}",
             blockId, newId, texturePath);
}

uint8_t AtlasManager::GetBlockId(const std::string& blockId) const {
    auto it = m_blockNameToId.find(blockId);
    return (it != m_blockNameToId.end()) ? it->second : 0;
}

std::string AtlasManager::GetBlockName(uint8_t id) const {
    auto it = m_idToBlockName.find(id);
    return (it != m_idToBlockName.end()) ? it->second : "base:air";
}

bool AtlasManager::LoadFromFolder(const std::string& folderPath) {

    try {
        for (const auto& filePath :  Engine::GetCurrentContext().GetPak("VoxCraftRes")->ListFiles(folderPath)) {
            std::filesystem::path entry(filePath);
            if (entry.extension() == ".json") {
                nlohmann::json json;
                try {
                    json = nlohmann::json::parse( Engine::GetCurrentContext().GetPak("VoxCraftRes")->ReadFileWithOverrideString(filePath));
                } catch (const std::exception& e) {
                    LOG_WARN("AtlasManager", "Failed to parse JSON {}: {}", entry.string(), e.what());
                    continue;
                }

                std::string blockName;
                if (json.contains("block_name")) {
                    blockName = json["block_name"];
                } else {
                    blockName = entry.parent_path().filename().string();
                }

                std::string namespaceName = "base";
                if (json.contains("namespace")) {
                    namespaceName = json["namespace"];
                }

                std::string blockId = namespaceName + ":" + blockName;

                std::filesystem::path texturePath = entry.parent_path() / "Block.png";

                RegisterBlock(blockId, texturePath.string());
            }
        }

        return BuildCombinedAtlas();

    } catch (const std::exception& e) {
        LOG_ERROR("AtlasManager", "Failed to load blocks from folder {}: {}", folderPath, e.what());
        return false;
    }
}

bool AtlasManager::BuildCombinedAtlas() {
    if (m_blockDefinitions.empty()) {
        LOG_ERROR("AtlasManager", "No blocks registered");
        return false;
    }

    struct Img {
        int w = 0, h = 0, c = 0;
        std::vector<uint8_t> pixels;
    };

    std::vector<Img> images;
    images.reserve(m_blockDefinitions.size());

    for (const auto& blockDef : m_blockDefinitions) {
        std::vector<uint8_t> fileData =  Engine::GetCurrentContext().GetPak("VoxCraftRes")->ReadFileWithOverride(blockDef.texturePath);
        if (fileData.empty()) {
            LOG_WARN("AtlasManager", "Failed to read atlas file {}", blockDef.texturePath);
            return false;
        }

        std::shared_ptr<UTexture> tmpTex = nullptr;
        if (!Engine::FileLoaders::ImageLoader::Load(fileData, tmpTex)) {
            LOG_WARN("AtlasManager", "ImageLoader::Load failed for {}", blockDef.texturePath);
            return false;
        }

        int w = tmpTex->GetWidth();
        int h = tmpTex->GetHeight();
        int c = tmpTex->GetChannels();
        const uint8_t* dataPtr = tmpTex->GetData();

        if (!dataPtr || w <= 0 || h <= 0) {
            LOG_WARN("AtlasManager", "Invalid image loaded {}", blockDef.texturePath);
            return false;
        }

        Img img;
        img.w = w;
        img.h = h;
        img.c = c;
        img.pixels.resize(static_cast<size_t>(w) * h * c);
        std::copy(dataPtr, dataPtr + (size_t)w * h * c, img.pixels.data());
        images.push_back(std::move(img));
    }
    m_atlasChannels = 4;
    for (auto& img : images) {
        if (img.c != 4) {
            std::vector<uint8_t> conv;
            conv.reserve((size_t)img.w * img.h * 4);
            for (int i = 0; i < img.w * img.h; ++i) {
                uint8_t r = img.pixels[i * img.c + 0];
                uint8_t g = (img.c > 1) ? img.pixels[i * img.c + 1] : r;
                uint8_t b = (img.c > 2) ? img.pixels[i * img.c + 2] : r;
                conv.push_back(r);
                conv.push_back(g);
                conv.push_back(b);
                conv.push_back(255);
            }
            img.pixels.swap(conv);
            img.c = 4;
        }
    }

    const int MAX_ATLAS_HEIGHT = 7680;

    struct AtlasPage {
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
        std::vector<BlockAtlasInfo> blockInfos;
    };

    std::vector<AtlasPage> pages;
    AtlasPage currentPage;

    int currentX = 0;
    int currentY = 0;
    int currentRowHeight = 0;

    for (size_t idx = 0; idx < images.size(); ++idx) {
        const Img& img = images[idx];

        if (currentY + img.h > MAX_ATLAS_HEIGHT) {
            if (currentPage.width > 0) {
                pages.push_back(std::move(currentPage));
            }

            currentPage = AtlasPage();
            currentX = 0;
            currentY = 0;
            currentRowHeight = 0;
        }

        if (currentPage.pixels.empty()) {
            currentPage.width = 0;
            currentPage.height = std::min(img.h, MAX_ATLAS_HEIGHT);
            currentPage.pixels.resize((size_t)currentPage.width * currentPage.height * m_atlasChannels, 0);
        }

        int requiredWidth = currentX + img.w;
        if (requiredWidth > currentPage.width) {
            std::vector<uint8_t> newPixels((size_t)requiredWidth * currentPage.height * m_atlasChannels, 0);

            for (int y = 0; y < currentPage.height; ++y) {
                for (int x = 0; x < currentPage.width; ++x) {
                    size_t oldIndex = (size_t)(y * currentPage.width + x) * m_atlasChannels;
                    size_t newIndex = (size_t)(y * requiredWidth + x) * m_atlasChannels;
                    for (int c = 0; c < m_atlasChannels; ++c) {
                        newPixels[newIndex + c] = currentPage.pixels[oldIndex + c];
                    }
                }
            }

            currentPage.width = requiredWidth;
            currentPage.pixels = std::move(newPixels);
        }

        for (int y = 0; y < img.h; ++y) {
            for (int x = 0; x < img.w; ++x) {
                size_t srcIndex = (size_t)(y * img.w + x) * img.c;
                size_t dstIndex = (size_t)((currentY + y) * currentPage.width + (x + currentX)) * m_atlasChannels;

                currentPage.pixels[dstIndex + 0] = img.pixels[srcIndex + 0];
                currentPage.pixels[dstIndex + 1] = img.pixels[srcIndex + 1];
                currentPage.pixels[dstIndex + 2] = img.pixels[srcIndex + 2];
                currentPage.pixels[dstIndex + 3] = img.pixels[srcIndex + 3];
            }
        }

        BlockAtlasInfo info;
        const int atlasParts = 6;
        int facePixelW = img.w / atlasParts;
        float invW = 1.0f / static_cast<float>(currentPage.width);
        float invH = 1.0f / static_cast<float>(currentPage.height);

        for (int face = 0; face < atlasParts; ++face) {
            int px0 = currentX + face * facePixelW;
            int px1 = px0 + facePixelW;
            float u0 = px0 * invW;
            float v0 = currentY * invH;
            float u1 = px1 * invW;
            float v1 = (currentY + img.h) * invH;

            info.faces[face].uv0 = glm::vec2(u0, v0);
            info.faces[face].uv1 = glm::vec2(u1, v1);
        }

        currentPage.blockInfos.push_back(info);

        currentX += img.w;
        currentRowHeight = std::max(currentRowHeight, img.h);

        if (currentX > 7680) {
            currentX = 0;
            currentY += currentRowHeight;
            currentRowHeight = 0;
        }
    }

    if (currentPage.width > 0) {
        pages.push_back(std::move(currentPage));
    }

    m_blockAtlasInfos.clear();
    m_atlasTextures.clear();

    for (size_t pageIdx = 0; pageIdx < pages.size(); ++pageIdx) {
        const auto& page = pages[pageIdx];

        for (const auto& blockInfo : page.blockInfos) {
            m_blockAtlasInfos.push_back(blockInfo);
        }

        auto texture = std::make_shared<UTexture>(
            "CombinedAtlas_" + std::to_string(pageIdx),
            page.width,
            page.height,
            m_atlasChannels,
            std::vector<unsigned char>(page.pixels)
        );

        m_atlasTextures.push_back(texture);

        LOG_INFO("AtlasManager", "Built atlas page {}: {}x{} with {} blocks",
                 pageIdx, page.width, page.height, page.blockInfos.size());
    }

    LOG_INFO("AtlasManager", "Built {} atlas pages with total {} blocks",
             pages.size(), m_blockDefinitions.size());

    return true;
}

const BlockAtlasInfo& AtlasManager::GetBlockAtlasInfo(uint8_t blockId) const {
    static BlockAtlasInfo empty;

    if (blockId == 0 || blockId > m_blockAtlasInfos.size()) {
        return empty;
    }

    return m_blockAtlasInfos[blockId - 1];
}