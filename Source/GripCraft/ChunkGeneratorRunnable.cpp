// Fill out your copyright notice in the Description page of Project Settings.

#include "ChunkGeneratorRunnable.h"

#pragma region Main Thread Code

FChunkGeneratorRunnable::FChunkGeneratorRunnable(AGripCraftGameMode& gameMode, const int idx = 0)
: GameMode(gameMode)
{
	Thread = FRunnableThread::Create(this, *(FString("ChunkGeneratorThread")+FString::FromInt(idx)));
}

FChunkGeneratorRunnable::~FChunkGeneratorRunnable()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
	}
}

#pragma endregion

bool FChunkGeneratorRunnable::Init()
{
	return true;
}

uint32 FChunkGeneratorRunnable::Run()
{
	FVector2D chunk;
	TArray<BlockData> chunkData;
	chunkData.Init(BlockData{}, GameMode.ChunkSize*GameMode.ChunkSize);

	while (bRunThread)
	{
		// apparently Enqueue() and Dequeue() are thread-safe!
		// also - eww, busy-looping, but it will have to suffice for now
		if (!GameMode.chunkCreationQueue.Dequeue(chunk))
		{
			bIsBusy = false;
			continue;
		}

		bIsBusy = true;

		// generate chunk data
		if (GameMode.chunkManager)
		{
			chunkData = GameMode.chunkManager->GenerateChunkData(chunk);

			// once the data is generated, write it to the chunk manager
			GameMode.chunkManager->WriteChunkData(chunk, chunkData);
		}
	}

	return 0;
}

void FChunkGeneratorRunnable::Stop()
{
	bRunThread = false;
}

