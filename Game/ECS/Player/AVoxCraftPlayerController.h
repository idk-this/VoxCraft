//
// Created by IDKTHIS on 23.09.2025.
//

#pragma once
#include "Core/ECS/Player/APlayerController.h"


class AVoxCraftPlayerController : public APlayerController {
    GENERATED_BODY();
public:
    AVoxCraftPlayerController() = default;
    ~AVoxCraftPlayerController() override = default;

    void Possess(std::shared_ptr<APawn> pawn) override { APlayerController::Possess(pawn);};
    void UnPossess() override { APlayerController::UnPossess();};

    void Update(float deltaTime) override;
    int Speed = 3;
};
