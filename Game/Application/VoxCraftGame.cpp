//
// Created by IDKTHIS on 02.09.2025.
//

#include "VoxCraftGame.h"

#include <unordered_set>
#include <set>
#include <utility>

#include "imgui.h"
#include "Core/CVar/Console.h"
#include "ECS/Player/AVoxCraftPlayerController.h"
#include "Core/CVar/CVar.h"
#include "Core/ECS/Base/UWorld.h"
#include "Core/ECS/Components/UMeshComponent.h"

#include "Core/ECS/Components/UTransformComponent.h"
#include "Core/ECS/Player/ULocalPlayer.h"
#include "Core/Log/Logger.h"
#include "Core/GameInfo.h"
#include "Core/Physics/Components/UCollisionComponent.h"
#include "Core/Physics/Components/UPhysicComponent.h"
#include "Core/UI/ConsoleUI.h"
#include "Core/UI/Core/XMLParser.h"
#include "Core/Utils/CameraUtils.h"
#include "Core/Utils/FileSystem.h"
#include "Core/Utils/FileLoaders/ImageLoader.h"
#include "ECS/Player/AVoxCraftPlayer.h"
#include "Platform/Window/SDL3/SDL3Window.h"
#include "Platform/Window/Components/WindowInputComponent.h"
#include "ECS/World/AChunk.h"
#include "ECS/World/AChunkManager.h"
#include "ECS/World/Generators/UWorldGenerator.h"
#include "Managers/AtlasManager.h"
#include "UI/Debug/DebugOverlay.h"

REGISTER_COMMAND_CALLBACK("say", "Print text to chat", [](const CommandArgs& args){
                          if (args.empty()) return;
                          std::string msg;
                          for (auto& a : args) msg += a + " ";
                          LOG_INFO("Say", "{}", msg);
                          });

VoxCraftGame::VoxCraftGame()
: Engine::Application()
{
    SET_CVAR("w_title", "VoxCraft Beta");
    SET_CVAR("sv_allow_modding", true);
}

VoxCraftGame::~VoxCraftGame() = default;

ConsoleUI console;

void VoxCraftGame::Init()
{
    Application::Init();
    console.InitializeLoggerHook(m_logSystem.get());
    LOG_INFO("VoxCraft", "VoxCraft version: {} (Build number: {})", VOXCRAFT_VERSION_STR, VOXCRAFT_BUILD_NUMBER);
    LOG_INFO("VoxCraft", "VoxCraft build date: {}", VOXCRAFT_BUILD_DATE);
    LOG_INFO("VoxCraft", "VoxCraft build type: {}", VOXCRAFT_BUILD_TYPE);
    if (!RegisterPak("VoxCraftRes"))
    {
        LOG_FATAL("VoxCraft", "Main resource pak NOT FOUND!!!");
        return;
    }

    m_world = std::make_shared<UWorld>();

    auto& atlasManager = AtlasManager::Get();
    atlasManager.Initialize();
    atlasManager.LoadFromFolder("Textures/Blocks");


    m_debugOverlay = std::make_shared<DebugOverlay>();
    m_debugOverlay->Init();
    m_localPlayer = std::make_shared<ULocalPlayer>();

    auto playerContoller = m_world->SpawnActor<AVoxCraftPlayerController>();
    playerContoller->SetPlayer(m_localPlayer);
    auto playerPawn = m_world->SpawnActor<AVoxCraftPlayer>();
    playerPawn->GetComponent<UTransformComponent>()->position = glm::vec3(0, 25, 0);
    m_localPlayer->GetController()->Possess(playerPawn);
    m_chunkManager_id = m_world->SpawnActor<AChunkManager>()->GetObjectID();
    m_blockSize = 1.0f;
}

void VoxCraftGame::DrawBlockBounds() {
    auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
    UCameraComponent* camera = player->GetComponent<UCameraComponent>();

    glm::vec3 rayStart = camera->GetWorldPosition();
    glm::vec3 rayDir = camera->GetForwardVector();

    RaycastResult hit = m_world->GetActor<AChunkManager>()->PerformRaycast(rayStart, rayDir, 50.0f);


    if (hit.hit && hit.chunk) {
        float blockSize = 1.0f;
        glm::vec3 chunkPos = hit.chunk->GetComponent<UTransformComponent>()->position;
        glm::vec3 min = chunkPos + glm::vec3(hit.blockCoord) * blockSize;
        glm::vec3 max = min + glm::vec3(blockSize);
        glm::vec3 vertices[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {max.x, max.y, min.z}, {min.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z}
        };
        int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7}
        };
        if (hit.hit && hit.chunk) {
            float blockSize = 1.0f;
            glm::vec3 chunkPos = hit.chunk->GetComponent<UTransformComponent>()->position;
            glm::vec3 blockWorldPos = chunkPos + glm::vec3(hit.blockCoord) * blockSize + glm::vec3(blockSize/2); // центр блока

            glm::vec2 screenPos;
            if (CameraUtils::WorldToScreen(camera->GetViewMatrix(), camera->GetProjectionMatrix(), blockWorldPos, screenPos)) {
                ImGui::GetBackgroundDrawList()->AddCircleFilled(
                    ImVec2(screenPos.x, screenPos.y),
                    5.0f,
                    IM_COL32(0, 255, 0, 255)
                );
            }
        }
        for (int i = 0; i < 12; i++) {
            glm::vec2 p1, p2;
            if (CameraUtils::WorldToScreen(camera->GetViewMatrix(), camera->GetProjectionMatrix(), vertices[edges[i][0]], p1) &&
                CameraUtils::WorldToScreen(camera->GetViewMatrix(), camera->GetProjectionMatrix(), vertices[edges[i][1]], p2)) {

                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(p1.x, p1.y),
                    ImVec2(p2.x, p2.y),
                    IM_COL32(0, 255, 0, 255),
                    2.0f
                );
            }
        }
    }
}



void VoxCraftGame::Update(float deltaTime)
{
    console.Draw();

    Application::Update(deltaTime);
    if (window->GetInputComponent()->IsKeyPressed(KeyCode::KEY_GRAVE))
    {
        console.Toggle();
    }
    glm::vec3 playerPos = m_localPlayer->GetController()->GetPawn()->GetComponent<UTransformComponent>()->position;
    m_debugOverlay->SetProperty(
    "CurrentPlayerPos",
     std::format("Local Position: X: {:.2f}  Y: {:.2f}  Z: {:.2f}",
                 playerPos.x, playerPos.y, playerPos.z)
     );
    m_debugOverlay->SetProperty(
        "CurrentPlayerBlockPos",
        std::format(
            "Local Block Position: X: {}  Y: {}  Z: {}",
            static_cast<int>(playerPos.x),
            static_cast<int>(playerPos.y),
            static_cast<int>(playerPos.z)
        )
    );
    m_debugOverlay->SetProperty(
        "CurrentPlayerChunkPos",
        std::format(
            "Local Chunk Position: X: {}  Y: {} Z: {}",
                m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk().x,
                m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk().y,
                m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk().z
        )
    );
    auto playerRot = m_localPlayer->GetController()->GetPawn()->GetComponent<UCameraComponent>()->RelativeRotation;
    m_debugOverlay->SetProperty(
        "PlayerViewAngle",
        std::format(
            "View Angle (PYR): P: {:.1f}  Y: {:.1f}  R: {:.1f}",
            playerRot.x,  // Pitch
            playerRot.y,  // Yaw
            playerRot.z   // Roll
        )
    );
    float fps = 1.0f / deltaTime;
    float dtMs = deltaTime * 1000.0f;

    m_debugOverlay->SetProperty(
        "CurrentFps",
        std::format("FPS: {:.1f}  (dt: {:.2f} ms)", fps, dtMs)
    );
    m_debugOverlay->Render();


    static bool lastRightButton = false;
    bool rightButton = window->GetInputComponent()->GetMouseState().buttons[3];
    if (rightButton && !lastRightButton) {
        window->ToggleRelativeMouseMode();
    }
    lastRightButton = rightButton;
    if (window->GetInputComponent()->IsKeyDown(KeyCode::KEY_LEFT_ALT))
    {
        auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
        UCameraComponent* camera = player->GetComponent<UCameraComponent>();

        glm::vec3 rayStart = camera->GetWorldPosition();
        glm::vec3 rayDir = camera->GetForwardVector();

        RaycastResult hit = m_world->GetActor<AChunkManager>()->PerformRaycast(rayStart, rayDir, 50.0f);

        if (hit.hit && hit.chunk) {
            glm::ivec3 newBlockPos = hit.blockCoord + hit.normal;
            if (newBlockPos.x >= 0 && newBlockPos.x < 16 &&
                newBlockPos.y >= 0 && newBlockPos.y < 32 &&
                newBlockPos.z >= 0 && newBlockPos.z < 16) {
                if (hit.chunk->GetBlock(newBlockPos.x, newBlockPos.y, newBlockPos.z) == 0) {
                    hit.chunk->SetBlock(newBlockPos.x, newBlockPos.y, newBlockPos.z, 1);
                    LOG_INFO("PLACE", "Placed block at: {} {} {} (Chunk: {},{},{})",
                            newBlockPos.x, newBlockPos.y, newBlockPos.z,
                            hit.chunk->GetChunkCoord().x, hit.chunk->GetChunkCoord().y, hit.chunk->GetChunkCoord().z);
                }
            } else {
                glm::ivec3 chunkCoord = hit.chunk->GetChunkCoord();
                glm::ivec3 worldVoxel = chunkCoord * 16 + newBlockPos;
                glm::ivec3 newChunkCoord = m_world->GetActor<AChunkManager>(m_chunkManager_id)->WorldToChunkCoord(glm::vec3(worldVoxel));
                glm::ivec3 newBlockCoord = m_world->GetActor<AChunkManager>(m_chunkManager_id)->WorldToBlockCoord(glm::vec3(worldVoxel));

                auto newChunk = m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetChunk(newChunkCoord);
                if (newChunk) {
                    if (newChunk->GetBlock(newBlockCoord.x, newBlockCoord.y, newBlockCoord.z) == 0) {
                        newChunk->SetBlock(newBlockCoord.x, newBlockCoord.y, newBlockCoord.z, 1);
                        LOG_INFO("PLACE", "Placed block in neighbor chunk at: {} {} {} (Chunk: {},{},{})",
                                newBlockCoord.x, newBlockCoord.y, newBlockCoord.z,
                                newChunkCoord.x, newChunkCoord.y, newChunkCoord.z);
                    }
                }
            }
        }
    }

    if (window->GetInputComponent()->GetMouseState().buttons[1]) {
        auto* player = static_cast<AVoxCraftPlayer*>(m_localPlayer->GetController()->GetPawn().get());
        UCameraComponent* camera = player->GetComponent<UCameraComponent>();

        glm::vec3 rayStart = camera->GetWorldPosition();
        glm::vec3 rayDir = camera->GetForwardVector();

        RaycastResult hit = m_world->GetActor<AChunkManager>()->PerformRaycast(rayStart, rayDir, 50.0f);

        if (hit.hit && hit.chunk) {
            LOG_INFO("HIT", "Block: {} {} {} (Type: {})",
                    hit.blockCoord.x, hit.blockCoord.y, hit.blockCoord.z,
                    hit.chunk->GetBlock(hit.blockCoord.x, hit.blockCoord.y, hit.blockCoord.z));

            hit.chunk->SetBlock(hit.blockCoord.x, hit.blockCoord.y, hit.blockCoord.z, 0);
        }
    }
    {
        auto* playerPawn = m_localPlayer->GetController()->GetPawn().get();
        if (playerPawn) {
            glm::vec2 screenPos;
            glm::vec3 cameraPos = playerPawn->GetComponent<UTransformComponent>()->position;

            m_world->GetActor<AChunkManager>(m_chunkManager_id)->Update(deltaTime, playerPos);
            DrawBlockBounds(
                );
            auto* camera = playerPawn->GetComponent<UCameraComponent>();

            if (playerPawn->HasComponent(typeid(UPhysicComponent)))
            {
                playerPos.y -= 1;
                playerPawn->GetComponent<UPhysicComponent>()->SetGrounded(m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetBlock(playerPos) != 0);
                auto* chunk = m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetChunk(m_world->GetActor<AChunkManager>(m_chunkManager_id)->GetLastCenterChunk()).get();


            }
        }
    }


}

void VoxCraftGame::Run()
{
    Application::Run();

    LOG_INFO("Application", "Initializing game world.");
    Init();
    //uim_logSystem->add_output("*", std::cout);
    LOG_INFO("Application", "Creating window.");
    window = std::make_unique<SDL3Window>();
    if (!window->Create(GET_CVAR(int, "w_size_width"),
                        GET_CVAR(int, "w_size_height"),
                        GET_CVAR(std::string, "w_title"))) {
        LOG_FATAL("Application", "Window creation failed. Parameters: width({}) height({}) title({})",
                  GET_CVAR(int, "w_size_width"),
                  GET_CVAR(int, "w_size_height"),
                  GET_CVAR(std::string, "w_title"));
        return;
                        }

    LOG_INFO("Application", "Creating Vulkan renderer.");
    renderer = std::make_unique<VulkanRenderer>();
    if (!renderer->Init(window.get(), m_world.get())) {
        LOG_FATAL("Application", "Failed to initialize Vulkan renderer.");
        return;
    }

    auto* imguiContext = Engine::GetCurrentContext().GetImGui()->GetContext();
    ImGui::SetCurrentContext(imguiContext);



    LOG_INFO("Application", "Initialization successful.");
    LOG_INFO("Application", "Starting main loop.");
    MainLoop();

    LOG_INFO("Application", "Main loop terminated. Shutting down renderer.");
    renderer->Cleanup();
    LOG_INFO("Application", "Renderer cleaned up. Executing shutdown procedures.");
    Shutdown();
    LOG_INFO("Application", "Shutdown complete.");
}

