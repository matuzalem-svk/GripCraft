// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Block.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class GRIPCRAFT_API ABlock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABlock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	void SetPosition(FVector pos);
	void SetPositionRelative(FVector pos, FVector_NetQuantizeNormal normal);

	void HighlightBlock();
	void UnhighlightBlock();

	UFUNCTION()
	void TickBreaking(float DeltaTime);
	void ResetBreaking();

	void SetMaterialScalarParameterValue(FName name, float value);

public:	
	/** Block mesh */
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* SM_BlockMesh;

	/** Block mesh dimension in units */
	UPROPERTY(EditDefaultsOnly)
	float Dimension;

	/** Block name */
	UPROPERTY(EditDefaultsOnly)
	FString BlockName;

	/** How strongly is the block highlighted when it's being looked at */
	UPROPERTY(EditDefaultsOnly)
	float HighlightFactor;

	/** How long it takes (in seconds) to destroy the block */
	UPROPERTY(EditDefaultsOnly)
	float TimeToDestroy;

	/** Is the block breakable? */
	UPROPERTY(EditDefaultsOnly)
	bool bIsBreakable;

private:
	bool bHighlighted;
	float fTimeDestroyed;
	UMaterialInstanceDynamic* MID_BlockMaterial;
};
