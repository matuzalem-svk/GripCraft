// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkManager.h"

AChunkManager::AChunkManager()
: NoiseGenerator(nullptr)
{
	NoiseGenerator = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("FastNoiseWrapper"));
}

void AChunkManager::SetGameMode(AGripCraftGameMode* mode)
{
	GameMode = mode;
}

void AChunkManager::SetChunkRadius(uint8 radius)
{
	ChunkRadius = radius;
	ChunkIndexDimension = 2*ChunkRadius + 1;
	MaxChunkIndex = ChunkIndexDimension*ChunkIndexDimension;
	CenterChunkIndex = (MaxChunkIndex - 1)/2;

	BlocksDataArray.Init(BlockData{}, GameMode->ChunkSize2*MaxChunkIndex);
}

bool AChunkManager::InitializeGenerator(bool forceRandomSeed)
{
	// set up noise generator for terrain creation
	bool randomSeed = GameMode->bRandomizeSeed || forceRandomSeed;
	if (NoiseGenerator)
	{
		NoiseGenerator->SetupFastNoise(
			EFastNoise_NoiseType::Perlin,										// noise type
			randomSeed ? FDateTime::Now().ToUnixTimestamp() : GameMode->Seed,	// seed
			GameMode->Frequency,												// frequency
			EFastNoise_Interp::Quintic,											// interpolation
			EFastNoise_FractalType::FBM,										// fractal type
			GameMode->Octaves,													// octaves
			GameMode->Lacunarity,												// lacunarity
			GameMode->Gain														// gain
		);

		if (NoiseGenerator->IsInitialized()) UE_LOG(LogTemp, Warning, TEXT("FastNoise initialized!"));

		return NoiseGenerator->IsInitialized();
	}

	return false;
}

TArray<BlockData> AChunkManager::GenerateChunkData(const FVector2D& chunk)
{
	TArray<BlockData> ret;
	ret.Init(BlockData(), GameMode->ChunkSize2);

	for (uint8 i = 0; i < GameMode->ChunkSize; ++i)
	{
		for (uint8 j = 0; j < GameMode->ChunkSize; ++j)
		{
			const int32 idx = j + i*GameMode->ChunkSize;
			const int32 worldX = j + chunk.X*GameMode->ChunkSize;
			const int32 worldY = i + chunk.Y*GameMode->ChunkSize;

			float noiseValue = NoiseGenerator->GetNoise2D((float)worldX, (float)worldY);
			UE_LOG(LogTemp, Warning, TEXT("Noise value %f"), noiseValue)

			ret[idx] = BlockData(worldX, worldY, (int32)(noiseValue*GameMode->NoiseAmplitude + GameMode->NoiseOffset));
			auto& block = ret[idx];

			if (block.Z > GameMode->MaxDirtHeight)
			{
				block.Type = BlockType::Rock;
			}
		}
	}

	return ret;
}

void AChunkManager::WriteChunkData(const FVector2D& chunk, const TArray<BlockData>& data)
{
	//FScopeLock(*ChunkDataWriteMutex);

	int32 beginIdx = GetBlockArrayIndex(chunk, FVector2D{ 0.f, 0.f });

	// this could be faster with some memory swap, idk how to do it though
	for (int32 i = 0; i < data.Num(); ++i)
	{
		BlocksDataArray[beginIdx + i] = data[i];
	}
}

void AChunkManager::ClearAllBlockData()
{
	BlocksDataArray.Empty();
	BlocksDataArray.Init(BlockData{}, GameMode->ChunkSize2*MaxChunkIndex);
}

TArray<ChunkDelta> AChunkManager::GenerateDeltaChunkData(const FVector2D& delta)
{
	TArray<ChunkDelta> ret;

	if (delta.X != 0)
	{
		for (int32 i = -(ChunkIndexDimension - 1)/2; i < (ChunkIndexDimension - 1)/2; ++i)
		{
			FVector2D chunk(GameMode->currentChunkCoords.X + delta.X, GameMode->currentChunkCoords.Y + i);
			ret.Emplace(chunk, GenerateChunkData(chunk));
		}
	}
	else if (delta.Y != 0)
	{
		for (int32 i = -(ChunkIndexDimension - 1)/2; i < (ChunkIndexDimension - 1)/2; ++i)
		{
			FVector2D chunk(GameMode->currentChunkCoords.X + i, GameMode->currentChunkCoords.Y + delta.Y);
			ret.Emplace(chunk, GenerateChunkData(chunk));
		}
	}

	return ret;
}

void AChunkManager::WriteDeltaChunkData(const FVector2D& delta, const TArray<ChunkDelta>& data)
{
	for (int32 i = 0; i < ChunkIndexDimension; ++i)
	{
		for (int32 j = 0; j < ChunkIndexDimension; ++j)
		{
			FVector2D chunkVec;
			int32 d;
			int32 kBegin, kEnd, kInc;
			if (delta.X != 0)
			{
				d = delta.X;
				chunkVec = FVector2D(j, i);
				kBegin = FMath::Min(0, d*GameMode->ChunkSize2); 
				kEnd = FMath::Max(d*GameMode->ChunkSize2, 0); 
				kInc = d*1;
			}
			else if (delta.Y != 0)
			{
				d = delta.Y;
				chunkVec = FVector2D(i, j);
				kBegin = FMath::Max(0, d*GameMode->ChunkSize2); 
				kEnd = FMath::Min(d*GameMode->ChunkSize2, 0); 
				kInc = d*1;
			}

			const int32 curChunkIdx = GetChunkArrayIndex(chunkVec)*GameMode->ChunkSize2;

			for (int32 k = kBegin; k < kEnd; k += kInc)
			{
				const int32 curIdx = curChunkIdx + k;
				const int32 newIdx = curIdx + d*GameMode->ChunkSize2;

				if (!BlocksDataArray.IsValidIndex(curIdx) || !BlocksDataArray.IsValidIndex(newIdx)) continue;

				BlocksDataArray.Swap(curIdx, newIdx);
			}
		}
	}

	for (auto& d : data)
	{
		WriteChunkData(d.chunk, d.data);
	}
}

BlockData* AChunkManager::GetTopBlockData(const FVector2D& chunk, const FVector& block)
{
	const int32 idx = GetBlockArrayIndex(chunk, FVector2D(block.X, block.Y));

	UE_LOG(LogTemp, Warning, TEXT("index for chunk [%f, %f], block [%f, %f, %f]: %d"), chunk.X, chunk.Y, block.X, block.Y, block.Z, idx);
	if (!BlocksDataArray.IsValidIndex(idx)) return nullptr;

	return &BlocksDataArray[idx];
}

inline int32 AChunkManager::GetChunkArrayIndex(const FVector2D& chunk) const
{
	FVector2D relative = chunk - GameMode->currentChunkCoords;
	return CenterChunkIndex + relative.X + relative.Y*ChunkIndexDimension;
}

inline int32 AChunkManager::GetBlockArrayIndex(const FVector2D& chunk, const FVector2D& block) const
{
	FVector2D relative = block - GameMode->currentChunkCoords*GameMode->ChunkSize;
	return GetChunkArrayIndex(chunk)*GameMode->ChunkSize2 + relative.X + relative.Y*GameMode->ChunkSize;
}
