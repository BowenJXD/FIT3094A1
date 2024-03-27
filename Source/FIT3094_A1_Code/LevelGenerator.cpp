// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelGenerator.h"

#include "FIT3094_A1_CodeGameModeBase.h"
#include "Ship.h"
#include "Containers/BinaryHeap.h"
#include "Engine/World.h"
#include "IO/IoPriorityQueue.h"
#include "Kismet/GameplayStatics.h"
#include "Util/IndexPriorityQueue.h"
#include "Utils/CBS.h"
#include "Utils/CPD.h"
#include "Utils/StatisticsExporter.h"

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

	if(ShipsAtGoal == Ships.Num() || SkipScenarioCount < SkipScenarios)
	{
		NextLevel();
		SkipScenarioCount++;
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

			// ------------ Code Modification (Adding) ----------------
			WorldArray[Y][X]->Direction = EDir::None;
			WorldArray[Y][X]->TimeStep = 0;
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
			
			// Code Modification (Adding Code)
			// ---------------------------
			Ship->PathCost += CurrentNode->GetTravelCost();
			// ---------------------------
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
			// Code Modification (Changing Code)
			// ---------------------------
			// Code before: TotalPathCost += Ships[i]->Path[j]->GetTravelCost();
			// ---------------------------
			int Cost = Ships[i]->Path[j]->GetTravelCost();
			ShipPathCost += Cost;
			TotalPathCost += Cost;
			// ---------------------------
		}
		
		if(IndividualStats)
		{
			UE_LOG(IndividualShips, Warning, TEXT("Ship %s has: Cells Searched: %d, Planned Path Cost: %d, Planned Path Action Amount: %d"), *Ships[i]->GetName(), Ships[i]->CellsSearched, ShipPathCost, Ships[i]->Path.Num());

			// Code Modification (Adding Code)
			ShipLog* Log = new ShipLog();
			Log->Start = FIntPoint(ShipSpawns[i].X, ShipSpawns[i].Y);
			Log->End = FIntPoint(GoldSpawns[i].X, GoldSpawns[i].Y);
			Log->ScenarioShipCount = Scenarios[ScenarioIndex - 1];
			Log->ShipName = Ships[i]->GetName();
			Log->CellsExpanded = Ships[i]->CellsSearched;
			Log->PathCost = ShipPathCost;
			Log->ActionAmount = Ships[i]->Path.Num();
			StatisticsExporter::Get().AddShipLog(*Log);
			// ---------------------------
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

	// Code Modification (Adding Code)
	if (!CollisionAndReplanning)
	{
		ScenarioLog* Log = new ScenarioLog();
		Log->ShipCount = Scenarios[ScenarioIndex - 1];
		Log->TotalCellsExpanded = SearchCount;
		Log->TotalPathLength = ShipPathLength;
		Log->TotalPathCost = TotalPathCost;
		Log->TotalTimeTaken = PlanEndTime - PlanStartTime;
		StatisticsExporter::Get().AddScenarioLog(*Log);
	}
	// ---------------------------
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

		
		// Code Modification (Adding Code)
		ScenarioLog* Log = new ScenarioLog();
		Log->ShipCount = Scenarios[ScenarioIndex - 1];
		Log->TotalCellsExpanded = SearchCount;
		Log->TotalPathLength = (TotalPathCost) * 10000 / PreviousPlannedCost;
		Log->TotalPathCost = TotalPathCost;
		Log->TotalTimeTaken = PlanEndTime - PlanStartTime;
		StatisticsExporter::Get().AddScenarioLog(*Log);
		// ---------------------------
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

			// Code Modification (Adding Code)
			StatisticsExporter::Get().ScenarioLogCSV();
			if (IndividualStats)
			{
				StatisticsExporter::Get().ShipLogCSV();
			}
			// ---------------------------
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
	// UCS();
	for (int i = 0; i < Ships.Num(); i++)
	{
		AStar(Ships[i]);
	}

	if (CollisionAndReplanning) CBS::Execute(Ships);

	for (int i = 0; i < Ships.Num(); i++)
	{
		RenderPath(Ships[i]->Path);
	}
}

void ALevelGenerator::AStar(AShip* Ship, TArray<Constraint*> Constraints)
{
    GridNode* StartNode = GetLocation(Ship);
    GridNode* GoalNode = Ship->GoalNode;

    TMap<GridNode*, int> OpenList;
    TMap<GridNode*, int> ClosedList;
    OpenList.Add(StartNode);
    StartNode->TimeStep = 0;

    while (OpenList.Num() > 0)
    {
        GridNode* CurrentNode = OpenList.begin().Key();
    	for (auto& Pair : OpenList)
		{
			if (Pair.Key->F < CurrentNode->F)
			{
				CurrentNode = Pair.Key;
			}
		}
    	OpenList.Remove(CurrentNode);

        ClosedList.Add(CurrentNode);

        if (CurrentNode == GoalNode)
        {
            int CellsSearched = ClosedList.Num();
            SearchCount += CellsSearched;
            Ship->CellsSearched = CellsSearched;

        	Ship->PathCost = 0;
        	Ship->Path.Empty();
        	while (CurrentNode->Parent != nullptr)
        	{
        		Ship->Path.EmplaceAt(0, WorldArray[CurrentNode->Y][CurrentNode->X]);
        		Ship->PathCost += CurrentNode->GetTravelCost();
        		GridNode* NextNode = CurrentNode->Parent;
        		CurrentNode = NextNode;
        	}
        	ResetAllNodes();
        	//
        	
            break;
        }

        TArray<GridNode*> Neighbours = GetNeighbours(CurrentNode);
        for (int j = 0; j < Neighbours.Num(); j++)
        {
            GridNode* Neighbour = Neighbours[j];

        	if (ClosedList.Contains(Neighbour)) continue;

            int NewG = CurrentNode->G + Neighbour->GetTravelCost();
            int NewTimeStep = CurrentNode->TimeStep + 1;

            if (IsNodeValid(CurrentNode, Neighbour, NewTimeStep, Ship, Constraints))
            {
                if (!OpenList.Contains(Neighbour)
                	|| NewG < Neighbour->G)
                {
                    Neighbour->Parent = CurrentNode;
                    Neighbour->G = NewG;
                    Neighbour->H = GetManhattanDistance(Neighbour, GoalNode);
                    Neighbour->F = Neighbour->G + Neighbour->H;
                    Neighbour->TimeStep = NewTimeStep;

					if (!OpenList.Contains(Neighbour))
					{
						OpenList.Add(Neighbour);
					}
                }
            }
        }
    }
}

bool ALevelGenerator::IsNodeValid(GridNode* Current, GridNode* Next, int NextTimeStep, AShip* Ship, TArray<Constraint*> Constraints)
{
	bool bValid = true;
	Constraints.Append(Ship->Constraints);
	for (Constraint* Constraint : Constraints)
	{
		if (Constraint->TimeStep == NextTimeStep
			&& Constraint->End == Next
			&& (Constraint->Start == Next || Constraint->Start == Current))
		{
			bValid = false;
			break;
		}
	}
	return bValid;
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
	if (Node->X > 0)
	{
		Neighbours.Add(WorldArray[Node->Y][Node->X - 1]);
	}
	if (Node->X < MapSizeX - 1)
	{
		Neighbours.Add(WorldArray[Node->Y][Node->X + 1]);
	}
	if (Node->Y > 0)
	{
		Neighbours.Add(WorldArray[Node->Y - 1][Node->X]);
	}
	if (Node->Y < MapSizeY - 1)
	{
		Neighbours.Add(WorldArray[Node->Y + 1][Node->X]);
	}
	return Neighbours;
}

int ALevelGenerator::GetManhattanDistance(const GridNode* Start, const GridNode* End) const
{
	return FMath::Abs(Start->X - End->X) + FMath::Abs(Start->Y - End->Y);
}

int ALevelGenerator::GetIndex(GridNode* Node) const
{
	return Node->Y * MapSizeX + Node->X;
}

GridNode* ALevelGenerator::GetNode(int Index) const
{
	int X = Index % MapSizeX;
	int Y = Index / MapSizeX;
	return WorldArray[Y][X];
}

void ALevelGenerator::GenerateFirstMoveMapFiles(GridNode* Node)
{
	BackwardUniformCostSearch(Node);
	FString Filename = FString::FromInt(Node->X) + "_" + FString::FromInt(Node->Y);
	FirstMoveMapLogTXT(Filename);
}

GridNode* ALevelGenerator::GetNodeFromDirection(GridNode* Node, EDir Direction)
{
	switch (Direction)
	{
	case EDir::Up:
		if (Node->Y > 0)
		{
			return WorldArray[Node->Y - 1][Node->X];
		}
	case EDir::Down:
		if (Node->Y < MapSizeY - 1)
		{
			return WorldArray[Node->Y + 1][Node->X];
		}
	case EDir::Left:
		if (Node->X > 0)
		{
			return WorldArray[Node->Y][Node->X - 1];
		}
	case EDir::Right:
		if (Node->X < MapSizeX - 1)
		{
			return WorldArray[Node->Y][Node->X + 1];
		}
	default:
		return nullptr;
	}
}

void ALevelGenerator::RenderPath(TArray<GridNode*> Path)
{
	for (int i = 0; i < Path.Num(); i++)
	{
		FVector Position(Path[i]->X * GRID_SIZE_WORLD, Path[i]->Y * GRID_SIZE_WORLD, 10);
		AActor* PathActor = GetWorld()->SpawnActor(PathDisplayBlueprint, &Position);
		PathDisplayActors.Add(PathActor);
		Path[i]->Reset();
	}
}

void ALevelGenerator::UCS()
{     	
    for(int i = 0; i < Ships.Num(); i++)
    {
     	//INSERT YOUR PATHFINDING ALGORITHM HERE
     	//Make sure to call RenderPath(Ship) when you have found a goal for a ship
     	BackwardUniformCostSearch(Ships[i]->GoalNode);
     	
     	GridNode* Current = GetLocation(Ships[i]);
     	while (Current != Ships[i]->GoalNode)
     	{
     		GridNode* Parent = Current;
     		Current = GetNodeFromDirection(Current, Current->Direction);
     		Current->Parent = Parent;
     		SearchCount ++;
     		Ships[i]->CellsSearched++;
     	}
     	RenderPath(Ships[i]);
     	
     	ResetAllNodes();
    }	
}

void ALevelGenerator::BackwardUniformCostSearch(GridNode* Target)
{
	NodeQueue Frontier;

	// Initialize the search
	Target->H = 0;
	Frontier.push(Target);

	// Search loop
	while (!Frontier.empty()) {
		GridNode* Current = Frontier.top();
		Frontier.pop();

		// Iterate over the neighbors
		auto Neighbors = GetNeighbours(Current);
		for (GridNode* Next : Neighbors) {
			float NewCost = Current->H + Next->GetTravelCost();
			if (NewCost < Next->H
				|| (Next->H == 0 && Next != Target)) {
				Next->H = NewCost;
				Frontier.push(Next);

				// Update the first move direction
				Next->Direction = GetDirection(Current, Next);
			}
		}
	}
}

// Opposite direction
EDir ALevelGenerator::GetDirection(const GridNode* Current, const GridNode* Next) const
{
	if (Next->X > Current->X)
	{
		return EDir::Left;
	}
	else if (Next->X < Current->X)
	{
		return EDir::Right;
	}
	else if (Next->Y > Current->Y)
	{
		return EDir::Up;
	}
	else if (Next->Y < Current->Y)
	{
		return EDir::Down;
	}
	return EDir::None;
}

void ALevelGenerator::FirstMoveMapLogCSV()
{
	FString CSV = "";
	EDir Direction = EDir::None;
	for (int i = 0; i < MAX_MAP_SIZE; i++)
	{
		FString Row = "";
		for (int j = 0; j < MAX_MAP_SIZE; j++)
		{
			if (WorldArray[i][j] != nullptr)
			{
				Direction = WorldArray[i][j]->Direction;
			}
			else
			{
				Direction = EDir::None;
			}
			
			FString DirectionChar = " ";
			switch (Direction)
			{
			case EDir::Up:
				DirectionChar = "^";
				break;
			case EDir::Down:
				DirectionChar = "v";
				break;
			case EDir::Left:
				DirectionChar = "<";
				break;
			case EDir::Right:
				DirectionChar = ">";
				break;
			default: ;
			}
			Row += DirectionChar + ",";
		}
		CSV += Row + "\n";
	}

	FString Path = StatisticsExporter::GetPath("FirstMoveMap");
	if (FFileHelper::SaveStringToFile(CSV, *Path))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("File Successfully Saved At %s "), *Path));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("File Failed to save At %s "), *Path));
	}
}

void ALevelGenerator::FirstMoveMapLogTXT(FString Filename)
{
	FString CSV = "";
	EDir Direction = EDir::None;
	for (int i = 0; i < MapSizeY; i++)
	{
		FString Row = "";
		for (int j = 0; j < MapSizeX; j++)
		{
			if (WorldArray[i][j] != nullptr)
			{
				Direction = WorldArray[i][j]->Direction;
			}
			else
			{
				Direction = EDir::None;
			}
			
			Row += FString::FromInt(static_cast<int>(Direction));
		}
		CSV += Row + "\n";
	}

	FString Path = FPaths::ProjectContentDir() + "Logs/FirstMoveMap/" + Filename + ".txt";
	if (FFileHelper::SaveStringToFile(CSV, *Path))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("File Successfully Saved At %s "), *Path));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("File Failed to save At %s "), *Path));
	}
}


