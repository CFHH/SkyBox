// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SkyBoxGameMode.generated.h"

UCLASS(minimalapi)
class ASkyBoxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASkyBoxGameMode();
    virtual ~ASkyBoxGameMode();
    virtual void StartPlay() override;
};



