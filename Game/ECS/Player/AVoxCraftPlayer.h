//
// Created by IDKTHIS on 23.09.2025.
//

#pragma once
#include <memory>

#include "Core/ECS/Player/APawn.h"
#include "Core/ECS/Components/UCameraComponent.h"
#include "Core/ECS/Player/APlayerController.h"
#include "Core/ECS/Player/UPlayer.h"


class AVoxCraftPlayer : public APawn {
    GENERATED_BODY();
public:
    AVoxCraftPlayer() {
        AddComponent(std::make_shared<UCameraComponent>());
    }

};
