// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SkyBoxGameMode.h"
#include "SkyBoxHUD.h"
#include "SkyBoxCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASkyBoxGameMode::ASkyBoxGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ASkyBoxHUD::StaticClass();
}
