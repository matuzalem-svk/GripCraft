// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <utility>
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Block.h"
#include "GripCraftCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UENUM()
enum InteractAction
{
	Destroying	UMETA(DisplayName = "Destroying"),
	Placing		UMETA(DisplayName = "Placing"),
	// KEEP AS LAST VALUE
	InteractActionEnd
};

UCLASS(config=Game)
class AGripCraftCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

public:
	AGripCraftCharacter();

protected:
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

private:
	std::pair<ABlock*, FVector_NetQuantizeNormal> GetViewedBlock();

public:
	/** The block the player is currently looking at */
	ABlock* ViewedBlock;

public:
	/** How far can the player's look reach in units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Reach;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	UAnimMontage* FireAnimation;

	/** Block type that serves as the placement guide */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ABlock> DummyPlacementBlockType;

	/** Array of blocks the player can place */
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<ABlock>> PlaceableBlocks;

protected:
	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

protected:
	void OnInteractBegin();

	void OnInteractEnd();

	void CycleInteractAction();

	void SelectionUpAction();

	void SelectionDownAction();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

private:
	float blockBreakingTickTime;

	ABlock* DummyPlacementBlock;

	TEnumAsByte<InteractAction> PlayerInteractAction;

	int32 PlaceableBlockIndex;

	FTimerDelegate BlockBreakingDelegate;
	FTimerHandle BlockBreakingHandle;
};

