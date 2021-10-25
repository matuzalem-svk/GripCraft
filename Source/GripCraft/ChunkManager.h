// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GripCraftGameMode.h"
#include "FastNoiseWrapper.h"
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ChunkManager.generated.h"

enum BlockType
{
	Dirt,
	Rock,
	Wood,
	GlowingRock
};

struct BlockData
{
	// block coordinates
	int32 X, Y, Z;

	// type of block
	BlockType Type;

	// ABlock instance
	// TODO use some sort of unique_ptr
	ABlock* BlockPtr;

	BlockData()
	: X(0), Y(0), Z(0), Type(BlockType::Dirt), BlockPtr(nullptr)
	{ }

	BlockData(int32 X, int32 Y, int32 Z, BlockType Type = BlockType::Dirt, ABlock* BlockPtr = nullptr)
	: X(X), Y(Y), Z(Z), Type(Type), BlockPtr(BlockPtr)
	{ }

	virtual ~BlockData()
	{
		if (this && BlockPtr) BlockPtr->Destroy();
	}

	void SetBlockPtr(ABlock* blockPtr)
	{
		if (this && BlockPtr) BlockPtr->Destroy();

		BlockPtr = blockPtr;
	}
};

struct ChunkDelta
{
	FVector2D chunk;
	// this could've been just iterators to the blocks array
	TArray<BlockData> data;

	ChunkDelta(FVector2D chunk, TArray<BlockData> data)
	: chunk(chunk), data(data)
	{

	}
};

class AGripCraftGameMode;

/**
 * ChunkManager generates and stores all the block and chunk data (position, type).
 */
UCLASS()
class GRIPCRAFT_API AChunkManager : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AChunkManager();
	virtual ~AChunkManager() = default;

public:
	void SetGameMode(AGripCraftGameMode* mode);

	void SetChunkRadius(uint8 radius);

	bool InitializeGenerator(bool forceRandomSeed = false);

	TArray<BlockData> GenerateChunkData(const FVector2D& chunk);
	void WriteChunkData(const FVector2D& chunk, const TArray<BlockData>& data);
	void ClearAllBlockData();

	TArray<ChunkDelta> GenerateDeltaChunkData(const FVector2D& delta);
	void WriteDeltaChunkData(const FVector2D& delta, const TArray<ChunkDelta>& data);

	BlockData* GetTopBlockData(const FVector2D& chunk, const FVector& block);

public:
	inline int32 GetChunkArrayIndex(const FVector2D& chunk) const;
	inline int32 GetBlockArrayIndex(const FVector2D& chunk, const FVector2D& block) const;

public:
	// See GripCraftGameMode
	uint8 ChunkRadius;

	// Array of all block data
	TArray<BlockData> BlocksDataArray;

private:
	AGripCraftGameMode* GameMode;

	int32 ChunkIndexDimension;
	int32 CenterChunkIndex;
	int32 MaxChunkIndex;

	//FCriticalSection ChunkDataWriteMutex;

	UFastNoiseWrapper* NoiseGenerator;
};
