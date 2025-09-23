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
#include "ECS/Player/AVoxCraftPlayer.h"
#include "Platform/Window/SDL3/SDL3Window.h"
#include "Platform/Window/Components/WindowInputComponent.h"

class TestCube : public AActor {
    UCLASS(TestCube);
public:
    TestCube() {
        AddComponent(std::make_shared<UTransformComponent>());
        auto mesh = std::make_shared<UMeshComponent>();
        mesh->SetIcosahedronMesh();
        AddComponent(mesh);
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
    m_localPlayer->SetController(playerContoller);
    playerContoller->SetPlayer(m_localPlayer);
    auto playerPawn = m_world->SpawnActor<AVoxCraftPlayer>();
    m_localPlayer->GetController()->Possess(playerPawn);
    glm::vec3 offset = glm::vec3(1.0f, 3.0f, 2.0f);
   // testPlayer = m_world->SpawnActor<LocalPlayer>();
    auto model = m_world->SpawnActor<TestCube>();
    glm::vec3 pos = glm::vec3(0, 0, 15) * 2.0f;
    model->GetComponent<UTransformComponent>()->SetPosition(pos);
    model->GetComponent<UTransformComponent>()->scale = glm::vec3(2);
    model->GetComponent<UTransformComponent>()->rotation = glm::quat(glm::vec3(67.5f, 0.0f, 0.0f));
    bool isLoaded = model->GetComponent<UMeshComponent>()->LoadFromOBJ("mesh_voxelized.obj");
    if (!isLoaded)
    {
        LOG_ERROR("Application", "Failed to load model");
    }
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                auto cube = m_world->SpawnActor<TestCube>();
                glm::vec3 pos = glm::vec3(x, y, z) * 2.0f - offset;
                cube->GetComponent<UTransformComponent>()->SetPosition(pos);
            }
        }
    }
}

void VoxCraftGame::Update(float deltaTime)
{
    Application::Update(deltaTime);
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
