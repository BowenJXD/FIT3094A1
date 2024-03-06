// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelGenerator.h"

#include "FIT3094_A1_CodeGameModeBase.h"
#include "Ship.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(IndividualShips);
DEFINE_LOG_CATEGORY(Heuristics);
DEFINE_LOG_CATEGORY(Collisions)

ALevelGenerator::ALevelGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	AFIT3094_A1_CodeGameModeBase* GameModeBase = Cast<AFIT3094_A1_CodeGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	GenerateWorldFromFile(GameModeBase->GetMapArray(GameModeBase->GetAssessedMapFile()));
	GenerateScenarioFromFile(GameModeBase->GetMapArray(GameModeBase->GetScenarioFile()));
	NextLevel();
	
}

void ALevelGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	int ShipsAtGoal = 0;

	for(int i = 0; i < Ships.Num(); i++)
	{
		if(Ships[i]->bAtGoal)
		{
			ShipsAtGoal++;
		}
	}

	if(ShipsAtGoal == Ships.Num())
	{
		NextLevel();
	}
}

void ALevelGenerator::SpawnWorldActors(TArray<TArray<char>> Grid)
{
	if(DeepBlueprint && ShallowBlueprint && LandBlueprint)
	{
		for(int Y = 0; Y < MapSizeY; Y++)
		{
			for(int X = 0; X < MapSizeX; X++)
			{
				float XPos = X * GRID_SIZE_WORLD;
				float YPos = Y * GRID_SIZE_WORLD;

				FVector Position(XPos, YPos, 0);

				switch(Grid[Y][X])
				{
				case '.':
					Terrain.Add(GetWorld()->SpawnActor(DeepBlueprint, &Position));
					break;
				case 'T':
					Terrain.Add(GetWorld()->SpawnActor(ShallowBlueprint, &Position));
					break;
				case '@':
					Terrain.Add(GetWorld()->SpawnActor(LandBlueprint, &Position));
					break;
				default:
					break;
				}
			}
		}
	}

	if(Camera)
	{
		FVector CameraPosition = Camera->GetActorLocation();

		CameraPosition.X = MapSizeX * 0.5 * GRID_SIZE_WORLD;
		CameraPosition.Y = MapSizeY * 0.5 * GRID_SIZE_WORLD;

		if(!CameraRotated)
		{
			CameraRotated = true;
			FRotator CameraRotation = Camera->GetActorRotation();

			CameraRotation.Pitch = 270;
			CameraRotation.Roll = 180;

			Camera->SetActorRotation(CameraRotation);
			Camera->AddActorLocalRotation(FRotator(0,0,90));
		}
		
		Camera->SetActorLocation(CameraPosition);
		
	}
}

void ALevelGenerator::GenerateNodeGrid(TArray<TArray<char>> Grid)
{
	for(int Y = 0; Y < MapSizeY; Y++)
	{
		for(int X = 0; X < MapSizeX; X++)
		{
			WorldArray[Y][X] = new GridNode();
			WorldArray[Y][X]->Y = Y;
			WorldArray[Y][X]->X = X;

			switch(Grid[Y][X])
			{
			case '.':
				WorldArray[Y][X]->GridType = GridNode::DeepWater;
				break;
			case '@':
				WorldArray[Y][X]->GridType = GridNode::Land;
				break;
			case 'T':
				WorldArray[Y][X]->GridType = GridNode::ShallowWater;
				break;
			default:
				break;
			}
			
		}
	}
}

void ALevelGenerator::ResetAllNodes()
{
	for( int Y = 0; Y < MapSizeY; Y++)
	{
		for(int X = 0; X < MapSizeX; X++)
		{
			WorldArray[Y][X]->F = 0;
			WorldArray[Y][X]->G = 0;
			WorldArray[Y][X]->H = 0;
			WorldArray[Y][X]->Parent = nullptr;
		}
	}
}

float ALevelGenerator::CalculateDistanceBetween(GridNode* First, GridNode* Second)
{
	FVector DistToTarget = FVector(Second->X - First->X,Second->Y - First->Y, 0);
	return DistToTarget.Size();
}

void ALevelGenerator::GenerateWorldFromFile(TArray<FString> WorldArrayStrings)
{
	if(WorldArrayStrings.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Map file not found!"))
		return;
	}

	FString Height = WorldArrayStrings[1];
	Height.RemoveFromStart("height ");
	MapSizeY = FCString::Atoi(*Height);

	FString Width = WorldArrayStrings[2];
	Width.RemoveFromStart("width ");
	MapSizeX = FCString::Atoi(*Width);

	TArray<TArray<char>> CharMapArray;
	CharMapArray.Init( TArray<char>(), MAX_MAP_SIZE);
	
	for(int i = 0; i < CharMapArray.Num(); i++)
	{
		CharMapArray[i].Init('x', MAX_MAP_SIZE);
	}
	
	for(int LineNum = 4; LineNum < MapSizeY + 4; LineNum++)
	{
		for(int CharNum = 0; CharNum < MapSizeX; CharNum++)
		{
			CharMapArray[LineNum-4][CharNum] = WorldArrayStrings[LineNum][CharNum];
		}
	}

	GenerateNodeGrid(CharMapArray);
	SpawnWorldActors(CharMapArray);
	
}

void ALevelGenerator::GenerateScenarioFromFile(TArray<FString> ScenarioArrayStrings)
{
	if(ScenarioArrayStrings.Num() == 0)
	{
		return;
	}
	
	for(int i = 1; i < ScenarioArrayStrings.Num(); i++)
	{
		TArray<FString> SplitLine;
		FString CurrentLine = ScenarioArrayStrings[i];
		
		CurrentLine.ParseIntoArray(SplitLine,TEXT("\t"));

		int ShipX = FCString::Atoi(*SplitLine[4]);
		int ShipY = FCString::Atoi(*SplitLine[5]);
		int GoldX = FCString::Atoi(*SplitLine[6]);
		int GoldY = FCString::Atoi(*SplitLine[7]);

		ShipSpawns.Add(FVector2d(ShipX, ShipY));
		GoldSpawns.Add(FVector2d(GoldX, GoldY));
	}
}

void ALevelGenerator::InitialisePaths()
{
	ResetPath();
	PlanStartTime = FPlatformTime::Seconds();
	CalculatePath();
	PlanEndTime = FPlatformTime::Seconds();
	DetailPlan();
}

void ALevelGenerator::RenderPath(AShip* Ship)
{
	GridNode* CurrentNode = Ship->GoalNode;

	if(CurrentNode)
	{
		while(CurrentNode->Parent != nullptr)
		{
			FVector Position(CurrentNode->X * GRID_SIZE_WORLD, CurrentNode->Y * GRID_SIZE_WORLD, 10);
			AActor* PathActor = GetWorld()->SpawnActor(PathDisplayBlueprint, &Position);
			PathDisplayActors.Add(PathActor);

			Ship->Path.EmplaceAt(0, WorldArray[CurrentNode->Y][CurrentNode->X]);
			CurrentNode = CurrentNode->Parent;
		}
	}
	
	
}

void ALevelGenerator::ResetPath()
{
	SearchCount = 0;
	ResetAllNodes();

	for(int i = 0; i < PathDisplayActors.Num(); i++)
	{
		PathDisplayActors[i]->Destroy();
	}
	PathDisplayActors.Empty();

	for(int i = 0; i < Ships.Num(); i++)
	{
		Ships[i]->Path.Empty();
	}
}

void ALevelGenerator::DetailPlan()
{
	int ShipPathLength = 0;
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		ShipPathLength += Ships[i]->Path.Num();
	}
	
	int TotalPathCost = 0;
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		int ShipPathCost = 0;
		
		for(int j = 1; j < Ships[i]->Path.Num(); j++)
		{
			TotalPathCost += Ships[i]->Path[j]->GetTravelCost();
		}
		
		if(IndividualStats)
		{
			UE_LOG(IndividualShips, Warning, TEXT("Ship %s has: Cells Searched: %d, Planned Path Cost: %d, Planned Path Action Amount: %d"), *Ships[i]->GetName(), Ships[i]->CellsSearched, ShipPathCost, Ships[i]->Path.Num());
		}
	}

	PreviousPlannedCost = TotalPathCost;

	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("Total Planning Time Taken: %d seconds"), PlanEndTime - PlanStartTime));
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("Total Estimated Path Cost: %d"), TotalPathCost));
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("Total Cells Expanded: %d with a Total Path Action Amount of: %d"), SearchCount, ShipPathLength));
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("CURRENT SCENARIO TOTAL")));
	
	UE_LOG(Heuristics, Warning, TEXT("CURRENT SCENARIO TOTAL"));
	UE_LOG(Heuristics, Warning, TEXT("Total Cells Expanded: %d with a total path length of: %d"), SearchCount, ShipPathLength);
	UE_LOG(Heuristics, Warning, TEXT("Total Estimated Path Cost: %d"), TotalPathCost);
	UE_LOG(Heuristics, Warning, TEXT("Total Planning Time Taken: %d seconds"), PlanEndTime - PlanStartTime);

}

void ALevelGenerator::DetailActual()
{
	if(CollisionAndReplanning)
	{
		int TotalPathCost = 0;
	
		for(int i = 0; i < PathCostTaken.Num(); i++)
		{
			TotalPathCost += PathCostTaken[i];
		}

		TotalPathCost += CrashPenalty;

		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("Ratio of Actual vs Planned: %fx"), (float)(TotalPathCost)/PreviousPlannedCost));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("Actual Total Path Cost including Crashes & Replanning: %d"), TotalPathCost));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("Actual Total Cells Expanded: %d with actual Total Path Action Amount of: %d"), SearchCount, PathCostTaken.Num()));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("PREVIOUS SCENARIO")));
		
		UE_LOG(Heuristics, Warning, TEXT("PREVIOUS SCENARIO"));
		UE_LOG(Heuristics, Warning, TEXT("Actual Total Cells Expanded: %d with actual Total Path Action Amount of: %d"), SearchCount, PathCostTaken.Num());
		UE_LOG(Heuristics, Warning, TEXT("Actual Total Path Cost including Crashes & Replanning: %d"), TotalPathCost);
		UE_LOG(Heuristics, Warning, TEXT("Ratio of Actual vs Planned: %fx"), (float)(TotalPathCost)/PreviousPlannedCost);
	}

	CrashPenalty = 0;
	PathCostTaken.Empty();
}

void ALevelGenerator::NextLevel()
{
	DestroyAllActors();
	
	if(ScenarioIndex >= 7)
	{
		if(!FinishedScenarios)
		{
			DetailActual();
			FinishedScenarios = true;
			UE_LOG(LogTemp, Warning, TEXT("Completed!"));
		}
	}
	
	else
	{
		if(ScenarioIndex != 0)
		{
			DetailActual();
		}
		
		for(int i = 0; i < Scenarios[ScenarioIndex]; i++)
		{
			if(GoldBlueprint && ShipBlueprint)
			{
				int GoldXPos = GoldSpawns[i + TotalIndex].X;
				int GoldYPos = GoldSpawns[i + TotalIndex].Y;

				FVector GoldPosition(GoldXPos* GRID_SIZE_WORLD, GoldYPos* GRID_SIZE_WORLD, 20);
				AActor* Gold = GetWorld()->SpawnActor(GoldBlueprint, &GoldPosition);
				
				Goals.Add(Gold);

				int ShipXPos = ShipSpawns[i + TotalIndex].X;
				int ShipYPos = ShipSpawns[i + TotalIndex].Y;

				FVector ShipPosition(ShipXPos* GRID_SIZE_WORLD, ShipYPos* GRID_SIZE_WORLD, 20);
				AShip* Ship = Cast<AShip>(GetWorld()->SpawnActor(ShipBlueprint, &ShipPosition));

				Ship->GoalNode = WorldArray[GoldYPos][GoldXPos];
				Ships.Add(Ship);
			}
		}
		
		TotalIndex += Scenarios[ScenarioIndex];
		ScenarioIndex++;
		InitialisePaths();
	}
}

void ALevelGenerator::DestroyAllActors()
{
	for(int i = 0; i < Goals.Num(); i++)
	{
		Goals[i]->Destroy();
	}
	Goals.Empty();
	
	for(int i = 0; i < PathDisplayActors.Num(); i++)
	{
		PathDisplayActors[i]->Destroy();
	}
	PathDisplayActors.Empty();
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		Ships[i]->Destroy();
	}
	Ships.Empty();
	
}

void ALevelGenerator::CheckForCollisions()
{
	for(int i = 0; i < Ships.Num(); i++)
	{
		if(!Ships[i]->bAtNextNode)
		{
			return;
		}
	}

	if(!CollisionAndReplanning)
	{
		for(int i = 0; i < Ships.Num(); i++)
		{
			Ships[i]->bAtNextNode = Ships[i]->bAtGoal;
		}
		return;
	}
	TArray<GridNode*> LastNodes;
	TArray<GridNode*> CurrentNodes;
	for(int i = 0; i < Ships.Num(); i++)
	{
		Ships[i]->bAtNextNode = Ships[i]->bAtGoal;
		LastNodes.Add(Ships[i]->LastNode);
		CurrentNodes.Add(Ships[i]->CurrentNode);
	}

	for(int i = 0; i < CurrentNodes.Num(); i++)
	{
		for(int j = i + 1; j < CurrentNodes.Num(); j++)
		{
			if(CurrentNodes[i] == CurrentNodes[j])
			{
				UE_LOG(Collisions, Warning, TEXT("CRASH OCCURED AT %d %d"), CurrentNodes[i]->X, CurrentNodes[i]->Y);
				CrashPenalty += 200;
			}
		}
		for(int j = i + 1; j < LastNodes.Num(); j++)
		{
			if(CurrentNodes[i] == LastNodes[j])
			{
				if(CurrentNodes[j] == LastNodes[i])
				{
					UE_LOG(Collisions, Warning, TEXT("CRASH OCCURED AT %d %d"), CurrentNodes[i]->X, CurrentNodes[i]->Y);
					CrashPenalty += 200;
				}
			}
		}
	}
	
}

//----------------------------------------------------------YOUR CODE-----------------------------------------------------------------------//

void ALevelGenerator::CalculatePath()
{
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		//INSERT YOUR PATHFINDING ALGORITHM HERE
		//Make sure to call RenderPath(Ship) when you have found a goal for a ship
		
		// A* algorithm
		AShip* Ship = Ships[i];
		TArray<GridNode*> OpenList;
		TArray<GridNode*> ClosedList;
		TArray<GridNode*> Path;
		GridNode* StartNode = GetLocation(Ship);
		GridNode* GoalNode = Ship->GoalNode;
		OpenList.Add(StartNode);
		SearchCount++;
		
		while (OpenList.Num() > 0)
		{
			// Find the node with the lowest F value
			GridNode* CurrentNode = OpenList[0];
			for (int j = 1; j < OpenList.Num(); j++)
			{
				if (OpenList[j]->F < CurrentNode->F)
				{
					CurrentNode = OpenList[j];
				}
			}
			
			OpenList.Remove(CurrentNode);
			ClosedList.Add(CurrentNode);

			// Check if we have reached the goal
			if (CurrentNode == GoalNode)
			{
				Ship->GoalNode = GoalNode;
				Ship->Path = Path;
				RenderPath(Ship);
				break;
			}

			// Check the neighbours of the current node
			TArray<GridNode*> Neighbours = GetNeighbours(CurrentNode);
			for (int j = 0; j < Neighbours.Num(); j++)
			{
				GridNode* Neighbour = Neighbours[j];
				// Skip if the neighbour is a land node
				if (Neighbour->GridType == GridNode::Land)
				{
					continue;
				}
				// Skip if the neighbour is in the closed list
				if (ClosedList.Contains(Neighbour))
				{
					continue;
				}
				// Calculate the new G, H and F values
				int NewG = CurrentNode->G + Neighbour->GetTravelCost();
				if (!OpenList.Contains(Neighbour))
				{
					OpenList.Add(Neighbour);
				}
				// Skip If the node is in the open list and isn't being relaxed
				else if (NewG >= Neighbour->G)
				{
					continue;
				}
				
				Neighbour->Parent = CurrentNode;
				Neighbour->G = NewG;
				Neighbour->H = GetManhattanDistance(Neighbour, GoalNode);
				Neighbour->F = Neighbour->G + Neighbour->H;

				Path.Add(Neighbour);
			}
		}
	}
	
}

void ALevelGenerator::Replan(AShip* Ship)
{
	if(CollisionAndReplanning)
	{
		//INSERT REPLANNING HERE
	}
}

GridNode* ALevelGenerator::GetLocation(const AShip* Ship) const
{
	return WorldArray[static_cast<int>(Ship->GetActorLocation().Y) / GRID_SIZE_WORLD]
					 [static_cast<int>(Ship->GetActorLocation().X) / GRID_SIZE_WORLD];
}

TArray<GridNode*> ALevelGenerator::GetNeighbours(GridNode* Node)
{
	TArray<GridNode*> Neighbours;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if (i == 0 && j == 0)
			{
				continue;
			}
			int X = Node->X + i;
			int Y = Node->Y + j;
			if (X >= 0 && X < MapSizeX && Y >= 0 && Y < MapSizeY)
			{
				Neighbours.Add(WorldArray[Y][X]);
			}
		}
	}
	return Neighbours;
}

int ALevelGenerator::GetManhattanDistance(const GridNode* Start, const GridNode* End) const
{
	return FMath::Abs(Start->X - End->X) + FMath::Abs(Start->Y - End->Y);
}