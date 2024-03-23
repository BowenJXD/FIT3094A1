// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

enum class EDir {
	None,
	Up,
	Down,
	Left,
	Right
};

/**
 * 
 */
class FIT3094_A1_CODE_API GridNode
{

public:

	GridNode();

	enum GRID_TYPE
	{
		DeepWater,
		Land,
		ShallowWater
	};

	int X;
	int Y;
	int G;
	float H;
	float F;

	GRID_TYPE GridType;
	GridNode* Parent;
	
	float GetTravelCost() const;

	// ----------------- New -----------------
	EDir Direction = EDir::None;
};
