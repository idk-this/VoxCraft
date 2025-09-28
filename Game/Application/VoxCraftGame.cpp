//
// Created by IDKTHIS on 02.09.2025.
//

#include "VoxCraftGame.h"

#include "ECS/Player/AVoxCraftPlayerController.h"
#include "Core/CVar/CVar.h"
#include "Core/ECS/Base/UWorld.h"
#include "Core/ECS/Components/UMeshComponent.h"

#include "Core/ECS/Components/UTransformComponent.h"
#include "Core/ECS/Player/ULocalPlayer.h"
#include "Core/Log/Logger.h"
#include "Core/Utils/FileLoaders/ImageLoader.h"
#include "ECS/Player/AVoxCraftPlayer.h"
#include "Platform/Window/SDL3/SDL3Window.h"
#include "Platform/Window/Components/WindowInputComponent.h"


class AChunk : public AActor {
    UCLASS(AChunk);
public:
    AChunk(int chunkSize = 16) {
        AddComponent(std::make_shared<UTransformComponent>());
        auto mesh = std::make_shared<UMeshComponent>();
        mesh->Mesh = std::make_shared<UMesh>();
        Engine::FileLoaders::ImageLoader::Load("DirtBlock (1).png", mesh->Texture);
        AddComponent(mesh);

        GenerateChunk(chunkSize);
    }
    void GenerateChunk(int chunkSize = 16) {
        if (!GetComponent<UMeshComponent>()) return;
        UMesh* mesh = GetComponent<UMeshComponent>()->Mesh.get();
        if (!mesh)
        {
            GetComponent<UMeshComponent>()->Mesh = std::make_shared<UMesh>();
            mesh = GetComponent<UMeshComponent>()->Mesh.get();
        }
        mesh->Clear();

        const float blockSize = 1.0f;
        std::vector<uint8_t> blocks(chunkSize * chunkSize * chunkSize, 0);

        for (int x = 0; x < chunkSize; x++)
            for (int y = 0; y < chunkSize; y++)
                for (int z = 0; z < chunkSize; z++)
                    blocks[x + y * chunkSize + z * chunkSize * chunkSize] = 1;

        auto hasBlock = [&](int x, int y, int z) -> bool {
            if (x < 0 || y < 0 || z < 0 ||
                x >= chunkSize || y >= chunkSize || z >= chunkSize)
                return false;
            return blocks[x + y * chunkSize + z * chunkSize * chunkSize] != 0;
        };

        const glm::vec3 cubeVertices[8] = {
            {-0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f},
        };

            struct Face { int idx[4]; glm::ivec3 normal; };
            const Face faces[6] = {
                {{0,3,2,1}, { 0, 0,-1}}, // back  (-Z)
                {{4,5,6,7}, { 0, 0, 1}}, // front (+Z)
                {{0,1,5,4}, { 0,-1, 0}}, // bottom(-Y)
                {{3,7,6,2}, { 0, 1, 0}}, // top   (+Y)
                {{0,4,7,3}, {-1, 0, 0}}, // left  (-X)
                {{1,2,6,5}, { 1, 0, 0}}, // right (+X)
            };

        const int atlasParts = 6;

        for (int x = 0; x < chunkSize; x++) {
            for (int y = 0; y < chunkSize; y++) {
                for (int z = 0; z < chunkSize; z++) {
                    if (!hasBlock(x,y,z)) continue;

                    glm::vec3 offset(x * blockSize, y * blockSize, z * blockSize);

                    for (int f = 0; f < 6; f++) {
                        glm::ivec3 n = faces[f].normal;
                        if (hasBlock(x + n.x, y + n.y, z + n.z)) continue;

                        float u0 = (float)f / atlasParts;
                        float u1 = (float)(f + 1) / atlasParts;
                        float v0 = 0.0f;
                        float v1 = 1.0f;

                        glm::vec2 b0(u0, v0), b1(u1, v0), b2(u1, v1), b3(u0, v1);
                        glm::vec2 a,b,c,d;
                        switch (f) {
                            case 0: // back  -> rotate 180
                                a = b2; b = b1; c = b0; d = b3;
                                break;
                            case 1: // front -> rotate 180 + flipH
                                // rotate180 => [b2,b3,b0,b1], flipH => [b3,b2,b1,b0]
                                a = b3; b = b2; c = b1; d = b0;
                                break;
                            case 2: // bottom -> no transform
                                a = b0; b = b1; c = b2; d = b3;
                                break;
                            case 3: // top -> no transform
                                a = b0; b = b1; c = b2; d = b3;
                                break;
                            case 4: // left -> rotate 90cw
                                a = b3; b = b2; c = b1; d = b0;
                                break;
                            case 5: // right -> rotate 90cw + flipV
                                // rotate90cw => [b3,b0,b1,b2], flipV => [b2,b1,b0,b3]
                                a = b2; b = b1; c = b0; d = b3;
                                break;
                            default:
                                a = b0; b = b1; c = b2; d = b3;
                                break;
                        }

                        uint32_t baseIndex = static_cast<uint32_t>(mesh->vertices.size());

                        mesh->vertices.push_back(cubeVertices[faces[f].idx[0]] * blockSize + offset); mesh->texCoords.push_back(a); mesh->colors.push_back(glm::vec3(1.0f));
                        mesh->vertices.push_back(cubeVertices[faces[f].idx[1]] * blockSize + offset); mesh->texCoords.push_back(b); mesh->colors.push_back(glm::vec3(1.0f));
                        mesh->vertices.push_back(cubeVertices[faces[f].idx[2]] * blockSize + offset); mesh->texCoords.push_back(c); mesh->colors.push_back(glm::vec3(1.0f));
                        mesh->vertices.push_back(cubeVertices[faces[f].idx[3]] * blockSize + offset); mesh->texCoords.push_back(d); mesh->colors.push_back(glm::vec3(1.0f));

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
    }
};

VoxCraftGame::VoxCraftGame()
: Engine::Application()
{
    SET_CVAR("w_title", "VoxCraft Beta");
    SET_CVAR("sv_allow_modding", true);

}

VoxCraftGame::~VoxCraftGame()
{
}

void VoxCraftGame::Init()
{
    Application::Init();
    m_world = std::make_shared<UWorld>();
    m_localPlayer = std::make_shared<ULocalPlayer>();

    auto playerContoller = m_world->SpawnActor<AVoxCraftPlayerController>();
    playerContoller->SetPlayer(m_localPlayer);
    auto playerPawn = m_world->SpawnActor<AVoxCraftPlayer>();
    m_localPlayer->GetController()->Possess(playerPawn);


    const int worldSize = 1;
    const int chunkSize = 16;
    const float blockSize = 1.0f;

    for (int cx = 0; cx < worldSize; ++cx) {
        for (int cz = 0; cz < worldSize; ++cz) {
            auto chunk = m_world->SpawnActor<AChunk>(chunkSize);
            glm::vec3 pos = glm::vec3(
                cx * chunkSize * blockSize,
                0,
                cz * chunkSize * blockSize
            );
            chunk->GetComponent<UTransformComponent>()->SetPosition(pos);
        }
    }
}


void VoxCraftGame::Update(float deltaTime)
{
    Application::Update(deltaTime);

    static bool lastRightButton = false;
    bool rightButton = window->GetInputComponent()->GetMouseState().buttons[3];
    if (rightButton && !lastRightButton) {
        window->ToggleRelativeMouseMode();
    }
    lastRightButton = rightButton;

    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_F)) {
        if (!Engine::GetCurrentContext().GetWorld()->GetActors().empty()) {
            int idx = std::rand() % Engine::GetCurrentContext().GetWorld()->GetActors().size();
            std::shared_ptr<AActor> victim = Engine::GetCurrentContext().GetWorld()->GetActors()[idx];
            Engine::GetCurrentContext().GetWorld()->DestroyActor(victim);
        }
    }
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_Q)) {
        if (!Engine::GetCurrentContext().GetWorld()->GetActors().empty()) {
            int idx = std::rand() % Engine::GetCurrentContext().GetWorld()->GetActors().size();
            AActor* actor = Engine::GetCurrentContext().GetWorld()->GetActors()[idx].get();

            if (auto transform = actor->GetComponent<UTransformComponent>()) {
                glm::vec3 offset(
                    (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f,
                    (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f,
                    (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f
                );
                transform->SetPosition(transform->position + offset);
            }
        }
    }
}

void VoxCraftGame::Run()
{
    Application::Run();
    m_logSystem->add_output("*", std::cout);
    LOG_INFO("Application", "Initializing engine.");
    Init();
    LOG_INFO("Application", "Initialization successful.");
    LOG_INFO("Application", "Creating window.");
    window = std::make_unique<SDL3Window>();
    if (!window->Create(GET_CVAR(int, "w_size_width"), GET_CVAR(int, "w_size_height"), GET_CVAR(std::string, "w_title"))) {
        LOG_INFO("Application", "Window creation failed. Parameters: width({}) height({}) title({})", GET_CVAR(int, "w_size_width"), GET_CVAR(int, "w_size_height"), GET_CVAR(std::string, "w_title"));
        return;
    }
    LOG_INFO("Application", "Creating Vulkan renderer.");

    renderer = std::make_unique<VulkanRenderer>();

    if (!renderer->Init(window.get(), m_world.get())) {
        LOG_FATAL("Application", "Failed to initialize Vulkan renderer.");
        return;
    }


    LOG_INFO("Application", "Starting main loop.");
    MainLoop();
    LOG_INFO("Application", "Main loop terminated. Shutting down renderer.");
    renderer->Cleanup();
    LOG_INFO("Application", "Renderer cleaned up. Executing shutdown procedures.");
    Shutdown();
    LOG_INFO("Application", "Shutdown complete.");

}
