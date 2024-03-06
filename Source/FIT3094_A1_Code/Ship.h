// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GridNode.h"
#include "GameFramework/Actor.h"
#include "Ship.generated.h"

class ALevelGenerator;

UCLASS()
class FIT3094_A1_CODE_API AShip : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShip();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
		float MoveSpeed;
	UPROPERTY(EditAnywhere)
		float Tolerance;
	UPROPERTY(EditAnywhere)
		UMaterial* FinishedMaterial;
	TArray<UActorComponent*> Meshes;

	TArray<GridNode*> Path;
	GridNode* GoalNode;
	GridNode* CurrentNode;
	GridNode* LastNode;
	bool bFirstMove = true;
	int CellsSearched = 0;
	ALevelGenerator* LevelGenerator;
	FVector Direction;
	AShip* PotentialCrash = nullptr;
	bool bAtNextNode = false;
	bool bAtGoal = false;
};
