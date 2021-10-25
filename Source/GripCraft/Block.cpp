// Fill out your copyright notice in the Description page of Project Settings.

#include "Block.h"
#include "GripCraftGameMode.h"

// Sets default values
ABlock::ABlock()
{
	bHighlighted = false;
	SM_BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));
	bIsBreakable = true;
	HighlightFactor = 2.f;
	TimeToDestroy = 3.f;
	Dimension = 100.f;
}

// Called when the game starts or when spawned
void ABlock::BeginPlay()
{
	Super::BeginPlay();

	Dimension = Cast<AGripCraftGameMode>(GetWorld()->GetAuthGameMode())->BlockDimension;
	MID_BlockMaterial = SM_BlockMesh->CreateDynamicMaterialInstance(0, SM_BlockMesh->GetMaterial(0));
}

void ABlock::SetPosition(FVector pos)
{
	SetActorLocation(pos);
}

void ABlock::SetPositionRelative(FVector pos, FVector_NetQuantizeNormal normal)
{
	SetActorLocation(pos + normal*Dimension);
}

void ABlock::HighlightBlock()
{
	if (bHighlighted) return;

	MID_BlockMaterial->SetScalarParameterValue(FName(TEXT("HighlightFactor")), HighlightFactor);

	bHighlighted = true;
}

void ABlock::UnhighlightBlock()
{
	if (!bHighlighted) return;

	MID_BlockMaterial->SetScalarParameterValue(FName(TEXT("HighlightFactor")), 1.f);

	bHighlighted = false;
}

void ABlock::TickBreaking(float DeltaTime)
{
	fTimeDestroyed += DeltaTime; 
	float destroyedPercentage = fTimeDestroyed / TimeToDestroy;
	MID_BlockMaterial->SetScalarParameterValue(FName(TEXT("CrackingFactor")), destroyedPercentage);

	if (destroyedPercentage >= 1.f)
	{
		Destroy();
	}
}

void ABlock::ResetBreaking()
{
	fTimeDestroyed = 0.f;
	MID_BlockMaterial->SetScalarParameterValue(FName(TEXT("CrackingFactor")), 0.f);
}

void ABlock::SetMaterialScalarParameterValue(FName name, float value)
{
	if (!MID_BlockMaterial)
	{
		MID_BlockMaterial = SM_BlockMesh->CreateDynamicMaterialInstance(0, SM_BlockMesh->GetMaterial(0));
	}

	if (MID_BlockMaterial)
	{
		MID_BlockMaterial->SetScalarParameterValue(name, value);
	}
}
