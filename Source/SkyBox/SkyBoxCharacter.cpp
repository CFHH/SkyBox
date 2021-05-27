// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SkyBoxCharacter.h"
#include "SkyBoxProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
// ZZW
#include <Private/PostProcess/SceneRenderTargets.h>
#include <SlateApplication.h>
#include <ImageUtils.h>
#include <FileHelper.h>
#include "SkyBoxRPC.h"
#include "SkyBoxWorker.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ASkyBoxCharacter

ASkyBoxCharacter::ASkyBoxCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;

    // ZZW
    PrimaryActorTick.bCanEverTick = true; //为了在编辑器里执行Tick
    m_SixDirection.Push(FRotator(0.0f, 0.0f, 0.0f));  //前
    m_SixDirection.Push(FRotator(0.0f, 90.0f, 0.0f));  //右
    m_SixDirection.Push(FRotator(0.0f, 180.0f, 0.0f));  //后
    m_SixDirection.Push(FRotator(0.0f, -90.0f, 0.0f));  //左
    m_SixDirection.Push(FRotator(90.0f, 0.0f, 0.0f));  //上
    m_SixDirection.Push(FRotator(-90.0f, 0.0f, 0.0f));  //下
    //m_SixDirection.Push(FRotator(0.0f, 0.0f, 0.0f));  //前
    m_current_job = NULL;
    m_CurrentDirection = -1;
    m_CurrentState = CaptureState::Invalid;
}

void ASkyBoxCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}

    // ZZW
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！ASkyBoxCharacter::BeginPlay()"));
    SkyBoxWorker::StartUp();
    APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(this, 0);
    OurPlayerController->SetViewTarget(SBCamera);
    FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddUObject(this, &ASkyBoxCharacter::OnBackBufferReady_RenderThread);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASkyBoxCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASkyBoxCharacter::OnFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ASkyBoxCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ASkyBoxCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASkyBoxCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASkyBoxCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASkyBoxCharacter::LookUpAtRate);
}

void ASkyBoxCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<ASkyBoxProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				const FRotator SpawnRotation = GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<ASkyBoxProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void ASkyBoxCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ASkyBoxCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void ASkyBoxCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void ASkyBoxCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void ASkyBoxCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ASkyBoxCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ASkyBoxCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASkyBoxCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool ASkyBoxCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ASkyBoxCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &ASkyBoxCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &ASkyBoxCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASkyBoxCharacter::~ASkyBoxCharacter()
{
    // ZZW
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！ASkyBoxCharacter::~ASkyBoxCharacter"));
    SkyBoxWorker::Shutdown();
    //FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().RemoveAll(this);
}

void ASkyBoxCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FScopeLock lock(&m_lock);
    
    if (m_current_job == NULL)
    {
        m_current_job = SkyBoxServiceImpl::Instance()->GetJob();
        if (m_current_job == NULL)
            return;
        UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！Get New Job, job_id = %d, scene_id = %d, position = (%.1f, %.1f, %.1f)"),
            m_current_job->JobID(), m_current_job->m_position.scene_id, m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z);
        SetActorLocation(FVector(m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z));
        SBCamera->SetActorLocation(FVector(m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z));
        m_CurrentDirection = 0;
        m_CurrentState = CaptureState::Waiting1;
        SBCamera->SetActorRotation(m_SixDirection[m_CurrentDirection]);
        UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！Change Direction, job_id = %d, m_CurrentDirection = %d"), m_current_job->JobID(), m_CurrentDirection);
        return;
    }
    if (m_CurrentState == CaptureState::Waiting1)
    {
        m_CurrentState = CaptureState::Prepared;
        return;
    }
    if (m_CurrentState == CaptureState::Captured)
    {
        bool ok = SavePNGToFile();
        if (!ok)
        {
            UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！Job Failed, job_id = %d, scene_id = %d, position = (%.1f, %.1f, %.1f)"),
                m_current_job->JobID(), m_current_job->m_position.scene_id, m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z);
            m_current_job->SetStatus(skybox::JobStatus::Failed);
            SkyBoxServiceImpl::Instance()->OnJobCompleted(m_current_job);
            m_current_job = NULL;
            m_CurrentDirection = -1;
            m_CurrentState = CaptureState::Invalid;
        }
        else
        {
            m_CurrentState = CaptureState::Saved;
        }
        return;
    }
    if (m_CurrentState == CaptureState::Saved)
    {
        ++m_CurrentDirection;
        if (m_CurrentDirection < m_SixDirection.Num())
        {
            m_CurrentState = CaptureState::Waiting1;
            SBCamera->SetActorRotation(m_SixDirection[m_CurrentDirection]);
            UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！Change Direction, job_id = %d, m_CurrentDirection = %d"), m_current_job->JobID(), m_CurrentDirection);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！Job Succeeded, job_id = %d, scene_id = %d, position = (%.1f, %.1f, %.1f)"),
                m_current_job->JobID(), m_current_job->m_position.scene_id, m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z);
            m_current_job->SetStatus(skybox::JobStatus::Succeeded);
            SkyBoxServiceImpl::Instance()->OnJobCompleted(m_current_job);
            m_current_job = NULL;
            m_CurrentDirection = -1;
            m_CurrentState = CaptureState::Invalid;
        }
        return;
    }
}

bool ASkyBoxCharacter::ShouldTickIfViewportsOnly() const
{
    return true;
}

void ASkyBoxCharacter::OnBackBufferReady_RenderThread(SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer)
{
    CaptureBackBufferToPNG(BackBuffer);
}

void ASkyBoxCharacter::CaptureBackBufferToPNG(const FTexture2DRHIRef& BackBuffer)
{
    FScopeLock lock(&m_lock);

    if (m_CurrentState != CaptureState::Prepared)
        return;
    if (m_BackBufferData.Num() != 0)
        return;
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！CAPTURE, job_id = %d, m_CurrentDirection = %d"), m_current_job->JobID(), m_CurrentDirection);
    FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
    FIntRect Rect(0, 0, BackBuffer->GetSizeX(), BackBuffer->GetSizeY());
    RHICmdList.ReadSurfaceData(BackBuffer, Rect, m_BackBufferData, FReadSurfaceDataFlags(RCM_UNorm));
    for (FColor& Color : m_BackBufferData)
    {
        Color.A = 255;
    }
    m_BackBufferSizeX = BackBuffer->GetSizeX();
    m_BackBufferSizeY = BackBuffer->GetSizeY();

    FDateTime Time = FDateTime::Now();
    /*m_BackBufferFilePath = FString::Printf(TEXT("G:\\UE4Workspace\\png\\BACK(%dX%d)_%d__%04d-%02d-%02d_%02d-%02d-%02d_%d.png"),
        m_BackBufferSizeX, m_BackBufferSizeY, m_CurrentDirection, Time.GetYear(), Time.GetMonth(), Time.GetDay(), Time.GetHour(), Time.GetMinute(), Time.GetSecond(), Time.GetMillisecond());*/
    m_BackBufferFilePath = FString::Printf(TEXT("G:\\UE4Workspace\\png\\BACK(%dX%d)_%d_(%.1f，%.1f，%.1f)_%d.png"),
        m_BackBufferSizeX, m_BackBufferSizeY, m_current_job->JobID(), m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z, m_CurrentDirection);
    m_CurrentState = CaptureState::Captured;
}

bool ASkyBoxCharacter::SavePNGToFile()
{
    if (m_BackBufferData.Num() == 0)
        return false;
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！SAVE, job_id = %d, position = (%.1f, %.1f, %.1f), m_CurrentDirection = %d"),
        m_current_job->JobID(), m_current_job->m_position.x, m_current_job->m_position.y, m_current_job->m_position.z, m_CurrentDirection);
    TArray<uint8> CompressedBitmap;
    FImageUtils::CompressImageArray(m_BackBufferSizeX, m_BackBufferSizeY, m_BackBufferData, CompressedBitmap);
    bool Success = FFileHelper::SaveArrayToFile(CompressedBitmap, *m_BackBufferFilePath);
    if (!Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！SavePNGToFile() FAIL %s"), *m_BackBufferFilePath);
        return false;
    }
    m_BackBufferData.Reset();
    return true;
}
