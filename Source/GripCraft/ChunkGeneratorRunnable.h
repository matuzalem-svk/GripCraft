// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "GripCraftGameMode.h"

class AGripCraftGameMode;

/**
 * 
 */
class GRIPCRAFT_API FChunkGeneratorRunnable : public FRunnable
{
public:
	FChunkGeneratorRunnable(AGripCraftGameMode& gameMode, const int idx);
	virtual ~FChunkGeneratorRunnable() override;

public:
	bool Init() override;
	uint32 Run() override;
	void Stop() override;

public:
	bool bIsBusy;

private:
	AGripCraftGameMode& GameMode;

	FRunnableThread* Thread;
	bool bRunThread;
};
