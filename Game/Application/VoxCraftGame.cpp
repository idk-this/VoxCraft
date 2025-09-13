//
// Created by IDKTHIS on 02.09.2025.
//

#include "VoxCraftGame.h"

#include "Core/CVar/CVar.h"
#include "Core/ECS/BaseClasses/UWorld.h"
#include "Core/Log/Logger.h"
#include "Platform/Window/SDL3/SDL3Window.h"

VoxCraftGame::VoxCraftGame()
: Engine::Application()
{
    SET_CVAR("w_title", "VoxCraft Beta");
    SET_CVAR("sv_allow_modding", true);
}

VoxCraftGame::~VoxCraftGame()
{
}

void VoxCraftGame::Run()
{
    Application::Run();
    Logger::instance().add_output("*", std::cout);
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
    m_world = std::make_shared<UWorld>();
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
