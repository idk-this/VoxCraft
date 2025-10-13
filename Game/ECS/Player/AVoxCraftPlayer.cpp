//
// Created by IDKTHIS on 23.09.2025.
//

#include "AVoxCraftPlayer.h"

#include "Core/CVar/CVar.h"
#include "Core/CVar/Console.h"
#include "Core/Log/Logger.h"

REGISTER_COMMANDF("setpos", "Set player position", CMD_CHEAT);
REGISTER_COMMANDF("setang", "Set player view angle", CMD_CHEAT);
DECLARE_CONVAR("viewmodel_fov", 90, "Player camera fov", CVAR_RUNTIME_ONLY | CVAR_CONSOLE_EDIT);


AVoxCraftPlayer::AVoxCraftPlayer()
{
    AddComponent(std::make_shared<UCameraComponent>());
    SUBSCRIBE_COMMAND("setpos", SetPosCMD);
    SUBSCRIBE_COMMAND("setang", SetAngCMD);
    DECLARE_CVAR_CALLBACK("viewmodel_fov", [this](const CVarValue& oldValue, const CVarValue& newValue) {
        this->SetFovCVar(newValue);
     });
}

void AVoxCraftPlayer::SetPosCMD(const CommandArgs& args)
{
    float x, y, z;

    if (Parse3Floats(args, x, y, z))
    {
        GetComponent<UTransformComponent>()->SetPosition({x, y, z});
    }
}

void AVoxCraftPlayer::SetAngCMD(const CommandArgs& args)
{
    float pitch, yaw, roll;
    if (Parse3Floats(args, pitch, yaw, roll))
    {
        GetComponent<UCameraComponent>()->RelativeRotation = {pitch, yaw, roll};
    }
}

void AVoxCraftPlayer::SetFovCVar(const CVarValue& newValue)
{
    if (const float* newFov = std::get_if<float>(&newValue))
    {
        LOG_INFO("CVar update", "Updated fov new: {}", (*newFov));

        GetComponent<UCameraComponent>()->fov = *newFov;
    }
    if (const int* newFov = std::get_if<int>(&newValue))
    {
        LOG_INFO("CVar update", "Updated fov new: {}", (*newFov));

        GetComponent<UCameraComponent>()->fov = *newFov;
    }
}