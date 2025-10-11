//
// Created by IDKTHIS on 23.09.2025.
//

#pragma once
#include <memory>

#include "Core/CVar/Console.h"
#include "Core/CVar/CVar.h"
#include "Core/ECS/Player/APawn.h"
#include "Core/ECS/Components/UCameraComponent.h"
#include "Core/ECS/Player/APlayerController.h"
#include "Core/ECS/Player/UPlayer.h"


class AVoxCraftPlayer : public APawn {
    UCLASS(AVoxCraftPlayer);
public:
    AVoxCraftPlayer();

private:
    void SetPosCMD(const CommandArgs& args);
    void SetAngCMD(const CommandArgs& args);
    void SetFovCVar(const CVarValue& newValue);
};
