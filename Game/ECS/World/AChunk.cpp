//
// Created by IDKTHIS on 28.09.2025.
//

#include "AChunk.h"

#include "Application/VoxCraftGame.h"
#include "Core/ECS/Components/UMeshComponent.h"
#include "Core/Log/Logger.h"
#include "Core/Physics/Components/UCollisionComponent.h"
#include "Core/Physics/Components/UPhysicComponent.h"
#include "Core/Utils/FileLoaders/ImageLoader.h"
#include "Generators/UWorldGenerator.h"

struct Face { int idx[4]; glm::ivec3 normal; };
struct BlockFaceUV {
    glm::vec2 uv0;
    glm::vec2 uv1;
};
struct BlockAtlasInfo {
    BlockFaceUV faces[6];
};

static std::vector<std::string> g_blockAtlases = {
    "Textures/Blocks/Bedrock/Block.png",
    "Textures/Blocks/Dirt/Block.png",
    "Textures/Blocks/Grass/Block.png",
    "Textures/Blocks/Stone/Block.png"
};
static bool BuildCombinedAtlas(
    std::vector<uint8_t>& outPixels,
    int& outWidth,
    int& outHeight,
    int& outChannels,
    std::vector<BlockAtlasInfo>& outBlockInfos)
{

    outPixels.clear();
    outBlockInfos.clear();
    struct Img {
        int w=0,h=0,c=0;
        std::vector<uint8_t> pixels;
    };

    std::vector<Img> images;
    images.reserve(g_blockAtlases.size());
    for (auto &path : g_blockAtlases) {
        std::vector<uint8_t> fileData = Engine::GetCurrentContext().GetPak("VoxCraftRes")->ReadFileWithOverride(path);
        if (fileData.empty()) {
            LOG_WARN("AChunk", "Failed to read atlas file {}", path.c_str());
            return false;
        }
        std::shared_ptr<UTexture> tmpTex = nullptr;
        if (!Engine::FileLoaders::ImageLoader::Load(fileData, tmpTex)) {
            LOG_WARN("AChunk", "ImageLoader::Load failed for {}", path.c_str());
            return false;
        }

        int w = tmpTex->GetWidth();
        int h = tmpTex->GetHeight();
        int c = tmpTex->GetChannels();
        const uint8_t* dataPtr = tmpTex->GetData();

        if (!dataPtr || w <= 0 || h <= 0) {
            LOG_WARN("AChunk", "Invalid image loaded %s", path.c_str());
            return false;
        }

        Img img;
        img.w = w; img.h = h; img.c = c;
        img.pixels.resize(static_cast<size_t>(w) * h * c);
        std::copy(dataPtr, dataPtr + (size_t)w * h * c, img.pixels.data());
        images.push_back(std::move(img));
    }

    outChannels = 4;
    for (auto &img : images) {
        if (img.c != 4) {
            std::vector<uint8_t> conv;
            conv.reserve((size_t)img.w * img.h * 4);
            for (int i = 0; i < img.w * img.h; ++i) {
                uint8_t r = img.pixels[i * img.c + 0];
                uint8_t g = (img.c > 1) ? img.pixels[i * img.c + 1] : r;
                uint8_t b = (img.c > 2) ? img.pixels[i * img.c + 2] : r;
                conv.push_back(r); conv.push_back(g); conv.push_back(b); conv.push_back(255);
            }
            img.pixels.swap(conv);
            img.c = 4;
        }
    }
    outHeight = 0;
    outWidth = 0;
    for (auto &img : images) {
        outWidth += img.w;
        outHeight = std::max(outHeight, img.h);
    }

    if (outWidth == 0 || outHeight == 0) return false;
    outPixels.assign((size_t)outWidth * outHeight * outChannels, 0);

    int offsetX = 0;
    outBlockInfos.resize(images.size());
    for (size_t idx = 0; idx < images.size(); ++idx) {
        const Img &img = images[idx];

        for (int y = 0; y < img.h; ++y) {
            for (int x = 0; x < img.w; ++x) {
                size_t srcIndex = (size_t)(y * img.w + x) * img.c;
                size_t dstIndex = (size_t)((y * outWidth) + (x + offsetX)) * outChannels;
                outPixels[dstIndex + 0] = img.pixels[srcIndex + 0];
                outPixels[dstIndex + 1] = img.pixels[srcIndex + 1];
                outPixels[dstIndex + 2] = img.pixels[srcIndex + 2];
                outPixels[dstIndex + 3] = img.pixels[srcIndex + 3];
            }
        }
        const int atlasParts = 6;
        int facePixelW = img.w / atlasParts;
        float invW = 1.0f / static_cast<float>(outWidth);
        float invH = 1.0f / static_cast<float>(outHeight);

        for (int face = 0; face < atlasParts; ++face) {
            int px0 = offsetX + face * facePixelW;
            int px1 = px0 + facePixelW;
            float u0 = px0 * invW;
            float v0 = 0.0f;
            float u1 = px1 * invW;
            float v1 = img.h * invH;
            outBlockInfos[idx].faces[face].uv0 = glm::vec2(u0, v0);
            outBlockInfos[idx].faces[face].uv1 = glm::vec2(u1, v1);
        }

        offsetX += img.w;
    }

    return true;
}

AChunk::AChunk(glm::ivec3 chunkCoord, int chunkWidth, int chunkHeight, int chunkDepth , UWorldGenerator* worldGenerator)
{
    m_chunkCoord = chunkCoord;
    m_worldGenerator = worldGenerator;
    m_chunkSize_w = chunkWidth;
    m_chunkSize_h = chunkHeight;
    m_chunkSize_d = chunkDepth;
    AddComponent(std::make_shared<UTransformComponent>());

    m_blocks.resize(static_cast<size_t>(m_chunkSize_w) * static_cast<size_t>(m_chunkSize_d) * static_cast<size_t>(chunkHeight), 0);


    auto physicsComp = std::make_shared<UPhysicComponent>();
    physicsComp->SetUseGravity(false);
    physicsComp->SetMass(0.0f);
    AddComponent(physicsComp);

    auto collisionComp = std::make_shared<UCollisionComponent>();
    collisionComp->SetCollisionEnabled(true);
    collisionComp->SetIsTrigger(false);
    AddComponent(collisionComp);

    auto meshComp = std::make_shared<UMeshComponent>();
    meshComp->Mesh = std::make_shared<UMesh>();
    AddComponent(meshComp);
    VoxCraftGame* gameCtx = static_cast<VoxCraftGame*>(Engine::GetCurrentContext().Get());

    std::vector<uint8_t> bigPixels;
    int bigW=0, bigH=0, bigC=0;
    std::vector<BlockAtlasInfo> atlasInfos;

    if (BuildCombinedAtlas(bigPixels, bigW, bigH, bigC, atlasInfos)) {
        if (!meshComp->Texture) meshComp->Texture = nullptr;
        std::shared_ptr<UTexture> atlasTex = std::make_shared<UTexture>(
            "CombinedAtlas",
            bigW,
            bigH,
            bigC,
            std::move(bigPixels)
        );

        meshComp->Texture = atlasTex;

    } else {
        LOG_WARN("AChunk", "Failed to build combined atlas");
    }
    GenerateChunk();
}

static std::vector<BlockAtlasInfo> s_blockAtlasInfos;
static int s_bigAtlasWidth = 0;
static int s_bigAtlasHeight = 0;
void AChunk::SetBlock(int x, int y, int z, uint8_t blockId) {
    if (x < 0 || y < 0 || z < 0 ||
           x >= m_chunkSize_w || y >= m_chunkSize_h || z >= m_chunkSize_d) {
        return;
           }

    int idx = x
            + y * m_chunkSize_w
            + z * (m_chunkSize_w * m_chunkSize_h);

    m_blocks[idx] = blockId;
    UpdateMesh();

}

uint8_t AChunk::GetBlock(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 ||
        x >= m_chunkSize_w || y >= m_chunkSize_h || z >= m_chunkSize_d) {
        return 0;
        }

    int idx = x
            + y * m_chunkSize_w
            + z * (m_chunkSize_w * m_chunkSize_h);

    return m_blocks[idx];
}
void AChunk::UpdateCollision()
{
    auto collisionComp = GetComponent<UCollisionComponent>();
    if (!collisionComp) return;

    const float blockSize = 1.0f;

    auto hasBlock = [&](int x, int y, int z) -> bool {
        if (x < 0 || y < 0 || z < 0) return false;
        if (x >= m_chunkSize_w || y >= m_chunkSize_h || z >= m_chunkSize_d) return false;
        return GetBlock(x, y, z) != 0;
    };

    collisionComp->ClearCollisionShapes();

    for (int x = 0; x < m_chunkSize_w; x++) {
        for (int y = 0; y < m_chunkSize_h; y++) {
            for (int z = 0; z < m_chunkSize_d; z++) {
                if (!hasBlock(x, y, z)) continue;

                bool hasNeighborXPos = hasBlock(x + 1, y, z);
                bool hasNeighborXNeg = hasBlock(x - 1, y, z);
                bool hasNeighborYPos = hasBlock(x, y + 1, z);
                bool hasNeighborYNeg = hasBlock(x, y - 1, z);
                bool hasNeighborZPos = hasBlock(x, y, z + 1);
                bool hasNeighborZNeg = hasBlock(x, y, z - 1);

                // если со всех сторон соседи — блок полностью закрыт, пропускаем
                if (hasNeighborXPos && hasNeighborXNeg &&
                    hasNeighborYPos && hasNeighborYNeg &&
                    hasNeighborZPos && hasNeighborZNeg) {
                    continue;
                    }

                glm::vec3 blockPos(x * blockSize, y * blockSize, z * blockSize);
                glm::vec3 extents(blockSize * 0.5f);

                collisionComp->AddBoxCollision(blockPos + extents, extents);
            }
        }
    }

    collisionComp->UpdateBoundingBox();
}


void AChunk::GenerateChunk()
{

    if (s_blockAtlasInfos.empty()) {
        VoxCraftGame* gameCtx = static_cast<VoxCraftGame*>(Engine::GetCurrentContext().Get());
        std::vector<uint8_t> bigPixels;
        int bigW=0, bigH=0, bigC=0;
        std::vector<BlockAtlasInfo> atlasInfos;
        if (BuildCombinedAtlas(bigPixels, bigW, bigH, bigC, atlasInfos)) {
            s_blockAtlasInfos = std::move(atlasInfos);
            s_bigAtlasWidth = bigW;
            s_bigAtlasHeight = bigH;
            if (GetComponent<UMeshComponent>()) {
                auto meshComp = GetComponent<UMeshComponent>();

                std::shared_ptr<UTexture> atlasTex = std::make_shared<UTexture>(
     "CombinedAtlas",
     bigW,
     bigH,
     bigC,
     std::move(bigPixels)
 );

                meshComp->Texture = atlasTex;
            }
        } else {
            LOG_WARN("AChunk", "GenerateChunk: could not build atlas");
            return;
        }
    }

    m_blocks = m_worldGenerator->GenerateChunkBlocks(m_chunkCoord, m_chunkSize_w, m_chunkSize_h);
    UpdateMesh();
}

void AChunk::UpdateMesh()
{
    const float blockSize = 1.0f;
    if (!GetComponent<UMeshComponent>()) return;
    UMesh* mesh = GetComponent<UMeshComponent>()->Mesh.get();
    if (!mesh)
    {
        GetComponent<UMeshComponent>()->Mesh = std::make_shared<UMesh>();
        mesh = GetComponent<UMeshComponent>()->Mesh.get();
    }
    mesh->Clear();
     auto hasBlock = [&](int x, int y, int z) -> bool {
        if (x < 0 || y < 0 || z < 0) return false;
        if (x >= m_chunkSize_w || y >= m_chunkSize_h || z >= m_chunkSize_d) return false;
        return GetBlock(x,y,z) != 0;
    };

    // ИСПРАВЛЕНИЕ: Вершины куба от угла, а не от центра
    const glm::vec3 cubeVertices[8] = {
        {0.0f, 0.0f, 0.0f},  // 0: левый-нижний-задний угол
        {1.0f, 0.0f, 0.0f},  // 1: правый-нижний-задний угол
        {1.0f, 1.0f, 0.0f},  // 2: правый-верхний-задний угол
        {0.0f, 1.0f, 0.0f},  // 3: левый-верхний-задний угол
        {0.0f, 0.0f, 1.0f},  // 4: левый-нижний-передний угол
        {1.0f, 0.0f, 1.0f},  // 5: правый-нижний-передний угол
        {1.0f, 1.0f, 1.0f},  // 6: правый-верхний-передний угол
        {0.0f, 1.0f, 1.0f}   // 7: левый-верхний-передний угол
    };

    const Face faces[6] = {
        {{0,3,2,1}, { 0, 0,-1}}, // back  (-Z)
        {{4,5,6,7}, { 0, 0, 1}}, // front (+Z)
        {{0,1,5,4}, { 0,-1, 0}}, // bottom(-Y)
        {{3,7,6,2}, { 0, 1, 0}}, // top   (+Y)
        {{0,4,7,3}, {-1, 0, 0}}, // left  (-X)
        {{1,2,6,5}, { 1, 0, 0}}, // right (+X)
    };

    for (int x = 0; x < m_chunkSize_w; x++) {
        for (int y = 0; y < m_chunkSize_h; y++) {
            for (int z = 0; z < m_chunkSize_d; z++) {
                if (!hasBlock(x,y,z)) continue;

                // ИСПРАВЛЕНИЕ: offset теперь указывает на угол блока
                glm::vec3 offset(x * blockSize, y * blockSize, z * blockSize);
                uint8_t blockId = GetBlock(x,y,z);
                if (blockId == 0) continue;
                size_t atlasIndex = static_cast<size_t>(blockId - 1);
                if (atlasIndex >= s_blockAtlasInfos.size()) continue;
                const BlockAtlasInfo &binfo = s_blockAtlasInfos[atlasIndex];

                for (int f = 0; f < 6; f++) {
                    glm::ivec3 n = faces[f].normal;
                    if (hasBlock(x + n.x, y + n.y, z + n.z)) continue;
                    glm::vec2 b0 = binfo.faces[f].uv0;
                    glm::vec2 b1 = glm::vec2(binfo.faces[f].uv1.x, binfo.faces[f].uv0.y);
                    glm::vec2 b2 = binfo.faces[f].uv1;
                    glm::vec2 b3 = glm::vec2(binfo.faces[f].uv0.x, binfo.faces[f].uv1.y);

                    glm::vec2 a,b,c,d;
                    switch (f) {
                        case 0:
                            a = b2; b = b1; c = b0; d = b3;
                            break;
                        case 1:
                            a = b3; b = b2; c = b1; d = b0;
                            break;
                        case 2:
                            a = b0; b = b1; c = b2; d = b3;
                            break;
                        case 3:
                            a = b0; b = b1; c = b2; d = b3;
                            break;
                        case 4:
                            a = b3; b = b2; c = b1; d = b0;
                            break;
                        case 5:
                            a = b2; b = b1; c = b0; d = b3;
                            break;
                        default:
                            a = b0; b = b1; c = b2; d = b3;
                            break;
                    }

                    uint32_t baseIndex = static_cast<uint32_t>(mesh->vertices.size());

                    // ИСПРАВЛЕНИЕ: Убираем умножение на blockSize, т.к. вершины уже в правильном масштабе
                    mesh->vertices.push_back(cubeVertices[faces[f].idx[0]] + offset);
                    mesh->texCoords.push_back(a);
                    mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->normals.push_back(glm::vec3(n));

                    mesh->vertices.push_back(cubeVertices[faces[f].idx[1]] + offset);
                    mesh->texCoords.push_back(b);
                    mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->normals.push_back(glm::vec3(n));

                    mesh->vertices.push_back(cubeVertices[faces[f].idx[2]] + offset);
                    mesh->texCoords.push_back(c);
                    mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->normals.push_back(glm::vec3(n));

                    mesh->vertices.push_back(cubeVertices[faces[f].idx[3]] + offset);
                    mesh->texCoords.push_back(d);
                    mesh->colors.push_back(glm::vec3(1.0f));
                    mesh->normals.push_back(glm::vec3(n));

                    mesh->indices.push_back(baseIndex + 0);
                    mesh->indices.push_back(baseIndex + 1);
                    mesh->indices.push_back(baseIndex + 2);
                    mesh->indices.push_back(baseIndex + 0);
                    mesh->indices.push_back(baseIndex + 2);
                    mesh->indices.push_back(baseIndex + 3);
                }
            }
        }
    }
    mesh->meshDirty = true;
    UpdateCollision();
}