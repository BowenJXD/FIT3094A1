// Fill out your copyright notice in the Description page of Project Settings.
#include "Ship.h"

#include "LevelGenerator.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AShip::AShip()
{
 	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	MoveSpeed = 500;
	Tolerance = MoveSpeed / 20;
	GoalNode = nullptr;
}

// Called when the game starts or when spawned
void AShip::BeginPlay()
{
	Super::BeginPlay();
	LevelGenerator = Cast<ALevelGenerator>(UGameplayStatics::GetActorOfClass(GetWorld(), ALevelGenerator::StaticClass()));
	GetComponents(UStaticMeshComponent::StaticClass(), Meshes);
}

// Called every frame
void AShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if(Path.Num() > 0 && !bAtNextNode)
	{
		FVector CurrentPosition = GetActorLocation();

		float TargetXPos = Path[0]->X * ALevelGenerator::GRID_SIZE_WORLD;
		float TargetYPos = Path[0]->Y * ALevelGenerator::GRID_SIZE_WORLD;

		FVector TargetPosition(TargetXPos, TargetYPos, CurrentPosition.Z);

		Direction = TargetPosition - CurrentPosition;
		Direction.Normalize();

		CurrentPosition += Direction * MoveSpeed * DeltaTime;

		if(FVector::Dist(CurrentPosition, TargetPosition) <= Tolerance)
		{
			CurrentPosition = TargetPosition;
			
			if(Path[0] == GoalNode)
			{
				bAtGoal = true;
				for(int i = 0; i < Meshes.Num(); i++)
				{
					Cast<UStaticMeshComponent>(Meshes[i])->SetMaterial(0, FinishedMaterial);
				}
			}
			
			if(LevelGenerator)
			{
				if(bFirstMove)
				{
					bFirstMove = false;
				}
				else
				{
					LevelGenerator->PathCostTaken.Add(Path[0]->GetTravelCost());
				}
			}

			LastNode = CurrentNode;
			CurrentNode = Path[0];
			Path.RemoveAt(0);
			bAtNextNode = true;
			LevelGenerator->CheckForCollisions();
		}
		
		SetActorLocation(CurrentPosition);
		SetActorRotation(Direction.Rotation());
		
	}
}

