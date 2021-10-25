// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FastNoiseWrapper.h"
#include "Block.h"
#include "ChunkManager.h"
#include "ChunkGeneratorRunnable.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GripCraftGameMode.generated.h"

class FChunkGeneratorRunnable;
class AChunkManager;
struct BlockData;
struct ChunkDelta;

UCLASS(minimalapi)
class AGripCraftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGripCraftGameMode();

public:
	virtual void StartPlay() override;

public:
	/** Generates a new world from position [0, 0] */
	void RefreshWorld(bool initial = false);
	/** RefreshWorld() wrapper for player input binding */
	void OnRefreshWorld();

public:
	/** Sets the current chunk coords based on passed location vector */
	void UpdateChunkCoords(const FVector& loc);

private:
	/** Creates actors 
	* @param chunkData the entirety of block data
	* @param deltaData (unused) data of the newly created chunks, used for endless world generation
	*/
	void GenerateBlocksFromChunkData(TArray<BlockData>& chunkData, TArray<ChunkDelta> deltaData = TArray<ChunkDelta>());
	// for some reason, the compiler had issues with deltaData being a reference, 
	// idk why, didn't have time to investigate and fix anymore

private:
	inline FVector2D GetChunkCoordsFromLocation(const FVector& loc) const;
	inline FVector GetBlockCoordsFromLocation(const FVector2D& chunk, const FVector& loc) const;
	inline FVector GetChunkMiddleCoords(const FVector2D& chunk) const;

private:
	/** Checks whether all generator threads finished working (unused) */
	bool AreGeneratorsFinished();

public:
	/** How long (in seconds) it takes to update block destruction progress */
	UPROPERTY(EditDefaultsOnly)
	float BlockDestructionTickTime;

	/** Noise generator seed */
	UPROPERTY(EditDefaultsOnly)
	int32 Seed;

	/** Should the seed be selected randomly (Unix timestamp) on each refresh? */
	UPROPERTY(EditDefaultsOnly)
	bool bRandomizeSeed;

	/** Noise generator frequency */
	UPROPERTY(EditDefaultsOnly)
	float Frequency;

	/** Number of noise octaves to use in generation */
	UPROPERTY(EditDefaultsOnly)
	int32 Octaves;

	/** Noise generator lacunarity */
	UPROPERTY(EditDefaultsOnly)
	float Lacunarity;

	/** Noise generator gain */
	UPROPERTY(EditDefaultsOnly)
	float Gain;

	/** Generated noise value gets multiplied by this */
	UPROPERTY(EditDefaultsOnly)
	float NoiseAmplitude;

	/** Multiplied noise value gets offset by this */
	UPROPERTY(EditDefaultsOnly)
	float NoiseOffset;

	/** Dimension of the chunk in blocks */
	UPROPERTY(EditDefaultsOnly)
	uint8 ChunkSize;

	/** How many chunks (horizontally, vertically and diagonally) from the center are generated and stored at the same time */
	UPROPERTY(EditDefaultsOnly)
	uint8 ChunkRadius;

	/** Dimension of the block mesh in units */
	UPROPERTY(EditDefaultsOnly)
	float BlockDimension;

	/** Array of available blocks to be placed */
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<ABlock>> PlaceableBlocks;

	/** Block height above which generated blocks get snow texture */
	UPROPERTY(EditDefaultsOnly)
	float SnowHeight;

	/** Block height above which dirt no longer spawns */
	UPROPERTY(EditDefaultsOnly)
	float MaxDirtHeight;

public:
	// chunk size squared
	int32 ChunkSize2;
	AChunkManager* chunkManager;
	FVector2D currentChunkCoords;

public:
	// number of generator threads (unused)
	const int32 numGenerators = 3;
	// generator task queue (unused)
	TQueue<FVector2D> chunkCreationQueue;
	// array of generator threads (unused)
	TArray<FChunkGeneratorRunnable> chunkGeneratorRunnables;
};



