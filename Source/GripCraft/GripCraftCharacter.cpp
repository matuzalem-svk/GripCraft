// Copyright Epic Games, Inc. All Rights Reserved.

#include "GripCraftCharacter.h"
#include "GripCraftGameMode.h"
#include "GripCraftProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AGripCraftCharacter

AGripCraftCharacter::AGripCraftCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
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

	Reach = 250.f;
	PlaceableBlockIndex = 0;
}

void AGripCraftCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	Mesh1P->SetHiddenInGame(false, true);

	blockBreakingTickTime = Cast<AGripCraftGameMode>(GetWorld()->GetAuthGameMode())->BlockDestructionTickTime;

	PlayerInteractAction = InteractAction::Destroying;

	if (DummyPlacementBlockType)
	{
		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		spawnParams.bNoFail = true;
		DummyPlacementBlock = GetWorld()->SpawnActor<ABlock>(DummyPlacementBlockType, FVector{}, FRotator{}, spawnParams);
	}
	if (DummyPlacementBlock)
	{
		DummyPlacementBlock->SetActorEnableCollision(false);
		DummyPlacementBlock->SetHidden(true);
	}
}

void AGripCraftCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto NewViewedBlockData = GetViewedBlock();

	if (NewViewedBlockData.first != ViewedBlock)
	{
		GetWorld()->GetTimerManager().ClearTimer(BlockBreakingHandle);

		if (ViewedBlock)
		{
			ViewedBlock->UnhighlightBlock();
			ViewedBlock->ResetBreaking();
		}

		ViewedBlock = NewViewedBlockData.first;

		if (ViewedBlock) ViewedBlock->HighlightBlock();
	}

	if (DummyPlacementBlock && PlayerInteractAction == InteractAction::Placing)
	{
		// make invisible if we're not "looking" at any block
		if (ViewedBlock)
		{
			// move it according to the hit normal
			DummyPlacementBlock->SetPositionRelative(ViewedBlock->GetActorLocation(), NewViewedBlockData.second);

			if (DummyPlacementBlock->IsHidden()) DummyPlacementBlock->SetActorHiddenInGame(false);
		}
		else
		{
			if (!DummyPlacementBlock->IsHidden()) DummyPlacementBlock->SetActorHiddenInGame(true);
		}
	}

	Cast<AGripCraftGameMode>(GetWorld()->GetAuthGameMode())->UpdateChunkCoords(GetActorLocation());
}

std::pair<ABlock*, FVector_NetQuantizeNormal> AGripCraftCharacter::GetViewedBlock()
{
	FHitResult R_TraceHit;

	FVector V_TraceBegin = FirstPersonCameraComponent->GetComponentLocation();
	FVector V_TraceEnd = V_TraceBegin + (FirstPersonCameraComponent->GetForwardVector() * Reach);

	FCollisionQueryParams CQP;
	CQP.AddIgnoredActor(this);
	CQP.AddIgnoredActor(DummyPlacementBlock);

	GetWorld()->LineTraceSingleByChannel(R_TraceHit, V_TraceBegin, V_TraceEnd, ECollisionChannel::ECC_WorldDynamic, CQP);

	return std::make_pair(Cast<ABlock>(R_TraceHit.GetActor()), R_TraceHit.ImpactNormal);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGripCraftCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind interact/destroy event
	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AGripCraftCharacter::OnInteractBegin);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AGripCraftCharacter::OnInteractEnd);

	// Bind placement event
	PlayerInputComponent->BindAction("RMB", IE_Pressed, this, &AGripCraftCharacter::CycleInteractAction);

	// Bind sub-action selection events
	PlayerInputComponent->BindAction("Mouse Wheel Up", IE_Pressed, this, &AGripCraftCharacter::SelectionUpAction);
	PlayerInputComponent->BindAction("Mouse Wheel Down", IE_Pressed, this, &AGripCraftCharacter::SelectionDownAction);

	// Bind world refresh action
	PlayerInputComponent->BindAction("Refresh", IE_Pressed, Cast<AGripCraftGameMode>(GetWorld()->GetAuthGameMode()), &AGripCraftGameMode::OnRefreshWorld);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AGripCraftCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGripCraftCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}


//////////////////////////////////////////////////////////////////////////
// MOVEMENT

void AGripCraftCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AGripCraftCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

//////////////////////////////////////////////////////////////////////////
// INTERACTION

void AGripCraftCharacter::OnInteractBegin()
{
	switch (PlayerInteractAction)
	{
		case InteractAction::Destroying:
		{
			// try to destroy a block
			if (ViewedBlock && ViewedBlock->bIsBreakable)
			{
				BlockBreakingDelegate.BindUFunction(ViewedBlock, FName("TickBreaking"), blockBreakingTickTime);
				GetWorld()->GetTimerManager().SetTimer(BlockBreakingHandle, BlockBreakingDelegate, blockBreakingTickTime, true);
			}
			
			// try and play the sound if specified
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
			}

			// try and play a firing animation if specified
			if (FireAnimation != nullptr)
			{
				// Get the animation object for the arms mesh
				UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
				if (AnimInstance != nullptr)
				{
					AnimInstance->Montage_Play(FireAnimation, 1.f);
				}
			}
		}
		break;
		case InteractAction::Placing:
		{
			if (DummyPlacementBlock && PlaceableBlocks.Num())
			{
				FActorSpawnParameters spawnParams;
				spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				spawnParams.bNoFail = true;
				GetWorld()->SpawnActor<ABlock>(
					PlaceableBlocks[PlaceableBlockIndex], 
					DummyPlacementBlock->GetActorLocation(), 
					DummyPlacementBlock->GetActorRotation(), 
					spawnParams
				);
			}
		}
		break;
	}
}

void AGripCraftCharacter::OnInteractEnd()
{
	GetWorld()->GetTimerManager().ClearTimer(BlockBreakingHandle);
	if (ViewedBlock)
	{
		ViewedBlock->ResetBreaking();
	}
}

void AGripCraftCharacter::CycleInteractAction()
{
	// cycle through the InteractAction values
	// perhaps unnecessarily complex for 2 values, but made to 
	BYTE actionId = (BYTE)PlayerInteractAction.GetValue();

	if (++actionId == (BYTE)InteractAction::InteractActionEnd) PlayerInteractAction = InteractAction::Destroying;
	else PlayerInteractAction = (InteractAction)actionId;

	// force end of any interaction
	OnInteractEnd();

	// hide placement guide if we're not in placement mode
	if (PlayerInteractAction.GetValue() != InteractAction::Placing)
	{
		DummyPlacementBlock->SetActorHiddenInGame(true);
	}

	// TODO put this in a GUI or something
	UE_LOG(LogTemp, Warning, TEXT("Selected action: %s"), *UEnum::GetValueAsString(PlayerInteractAction.GetValue()));
}

void AGripCraftCharacter::SelectionUpAction()
{
	switch (PlayerInteractAction)
	{
		case InteractAction::Placing:
		{
			if (++PlaceableBlockIndex >= PlaceableBlocks.Num()) PlaceableBlockIndex = 0;

			// TODO put this in a GUI or something
			UE_LOG(LogTemp, Warning, TEXT("Selected placeable block: %d"), PlaceableBlockIndex);
		}
		break;
	}
}

void AGripCraftCharacter::SelectionDownAction()
{
	switch (PlayerInteractAction)
	{
		case InteractAction::Placing:
		{
			if (--PlaceableBlockIndex < 0) PlaceableBlockIndex = PlaceableBlocks.Num() - 1;

			// TODO put this in a GUI or something
			UE_LOG(LogTemp, Warning, TEXT("Selected placeable block: %d"), PlaceableBlockIndex);
		}
		break;
	}
}
