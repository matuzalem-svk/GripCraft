// Copyright Epic Games, Inc. All Rights Reserved.

#include "GripCraftGameMode.h"
#include "GripCraftHUD.h"
#include "GripCraftCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGripCraftGameMode::AGripCraftGameMode()
: Super(), ChunkRadius(1), ChunkSize(32), BlockDimension(100.f), BlockDestructionTickTime(0.1f),
  NoiseAmplitude(50), NoiseOffset(32), SnowHeight(100.f), MaxDirtHeight(70.f)
{
	// set up the Chunk Manager game state class
	GameStateClass = AChunkManager::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AGripCraftHUD::StaticClass();
}

void AGripCraftGameMode::StartPlay()
{
	ChunkSize2 = ChunkSize*ChunkSize;

	// setup world generation

	chunkManager = GetGameState<AChunkManager>();

	if (chunkManager)
	{
		chunkManager->SetGameMode(this);
		chunkManager->SetChunkRadius(ChunkRadius);
		chunkManager->InitializeGenerator();
	}

	// generate world (initial)
	RefreshWorld(true);

	Super::StartPlay();
}

// event couldn't handle a default param method, so this wrapper makes it work
void AGripCraftGameMode::OnRefreshWorld()
{
	// run-time refresh
	RefreshWorld(false);
}

void AGripCraftGameMode::RefreshWorld(bool initial)
{
	// clear existing spawned blocks
	
	chunkManager->ClearAllBlockData();
	
	// randomize generator seed

	chunkManager->InitializeGenerator(!initial);

	// initialize generator runnables

	/*for (int32 i = 0; i < numGenerators; ++i)
	{
		chunkGeneratorRunnables.Emplace(*this, i);
	}*/

	// generate initial world chunks

	for (int32 i = -ChunkRadius; i < ChunkRadius+1; ++i)
	{
		for (int32 j = -ChunkRadius; j < ChunkRadius+1; ++j)
		{
			FVector2D chunk = { 0.f + j, 0.f + i};
			auto data = chunkManager->GenerateChunkData(chunk);
			chunkManager->WriteChunkData(chunk, data);
		}
	}

	//while (!AreGeneratorsFinished()) FPlatformProcess::Sleep(0.5f);;

	GenerateBlocksFromChunkData(chunkManager->BlocksDataArray);

	// move player to be above ground

	FVector playerLoc(0.f, 0.f, 0.f);
	FVector2D chunkLoc = GetChunkCoordsFromLocation(playerLoc);
	playerLoc = GetChunkMiddleCoords(chunkLoc);
	playerLoc.Z = (chunkManager->GetTopBlockData(chunkLoc, GetBlockCoordsFromLocation(chunkLoc, playerLoc))->Z + 2)*BlockDimension;
	GetWorld()->GetFirstPlayerController()->GetPawn()->SetActorLocation(playerLoc);
}

void AGripCraftGameMode::GenerateBlocksFromChunkData(TArray<BlockData>& chunkData, TArray<ChunkDelta> deltaData)
{
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnParams.bNoFail = true;

	for (auto& d : chunkData)
	{
		if (!d.BlockPtr)
		{
			ABlock* block = GetWorld()->SpawnActor<ABlock>(
				PlaceableBlocks[d.Type],
				FVector{ d.X * BlockDimension, d.Y * BlockDimension, d.Z * BlockDimension },
				FRotator{ 0.f, 0.f, 0.f },
				spawnParams
				);

			d.SetBlockPtr(block);

			if (block)
			{
				block->SetMaterialScalarParameterValue(TEXT("GrassFactor"), 0.5f);

				if (d.Z >= SnowHeight)
				{
					block->SetMaterialScalarParameterValue(TEXT("SnowFactor"), 0.2f);
				}
			}
		}
	}

	/*
	for (auto& dd : deltaData)
	{
		const int32 didx = chunkManager->GetChunkArrayIndex(dd.chunk);
		for (int32 idx = didx; idx < didx+ChunkSize; ++idx)
		{ 
			BlockData& d = chunkData[idx];

			ABlock* block = GetWorld()->SpawnActor<ABlock>(
				PlaceableBlocks[d.Type],
				FVector{ d.X * BlockDimension, d.Y * BlockDimension, d.Z * BlockDimension },
				FRotator{ 0.f, 0.f, 0.f },
				spawnParams
				);

			d.SetBlockPtr(block);

			if (block)
			{
				block->SetMaterialScalarParameterValue(TEXT("GrassFactor"), 0.5f);

				if (d.Z >= SnowHeight)
				{
					block->SetMaterialScalarParameterValue(TEXT("SnowFactor"), 0.2f);
				}
			}
		}
	}
	*/
}

void AGripCraftGameMode::UpdateChunkCoords(const FVector& loc)
{
	FVector2D newChunkCoords = GetChunkCoordsFromLocation(loc);
	FVector2D updateDelta = currentChunkCoords - newChunkCoords;
	currentChunkCoords = newChunkCoords;

	// broken
	/*if (updateDelta.X != 0 || updateDelta.Y != 0)
	{
		auto deltaData = chunkManager->GenerateDeltaChunkData(updateDelta);
		chunkManager->WriteDeltaChunkData(updateDelta, deltaData);
		GenerateBlocksFromChunkData(chunkManager->BlocksDataArray, deltaData);
	}*/

}

inline FVector2D AGripCraftGameMode::GetChunkCoordsFromLocation(const FVector& loc) const
{
	return FVector2D{ FMath::Floor((loc.X/BlockDimension)/ChunkSize), FMath::Floor((loc.Y/BlockDimension)/ChunkSize) };
}

inline FVector AGripCraftGameMode::GetBlockCoordsFromLocation(const FVector2D& chunk, const FVector& loc) const
{
	return FVector{	FMath::Floor((loc.X/BlockDimension) - chunk.X*ChunkSize), FMath::Floor((loc.X/BlockDimension) - chunk.Y*ChunkSize), loc.Z};
}

inline FVector AGripCraftGameMode::GetChunkMiddleCoords(const FVector2D& chunk) const
{
	return FVector{ FMath::Floor(chunk.X*ChunkSize/2), FMath::Floor(chunk.Y*ChunkSize/2), 0.f};
}

bool AGripCraftGameMode::AreGeneratorsFinished()
{
	for (int32 i = 0; i < numGenerators; ++i)
	{
		if (chunkGeneratorRunnables[i].bIsBusy) return false;
	}

	return true;
}
