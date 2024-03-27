#include "CBS.h"

#include "FIT3094_A1_Code/LevelGenerator.h"

void CBS::Execute(TArray<AShip*> Agents)
{
	for (int r = 0; r < MaxIteration; r++)
	{
		TArray<Conflict*> Conflicts;
		for (int i = 0; i < Agents.Num(); i++)
		{
			for (int j = i + 1; j < Agents.Num(); j++)
			{
				Conflict* Con = FindConflict(Agents[i], Agents[j]);
				if (Con != nullptr)	Conflicts.Add(Con);
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Number of conflicts: %d"), Conflicts.Num());
		
		if (Conflicts.Num() == 0) break;
		
		for (int i = 0; i < Conflicts.Num(); i++)
		{
			ResolveConflict(Conflicts[i]);
		}
	}
}

Conflict* CBS::FindConflict(AShip* Agent1, AShip* Agent2)
{	
	int Max = FMath::Max(Agent1->Path.Num(), Agent2->Path.Num());
	Conflict* Result = new Conflict();
	int NodeIndex1 = 0;
	int NodeIndex2 = 0;
	for(int i = 1; i < Max; i++)
	{
		if (i < Agent1->Path.Num()) NodeIndex1 = i;
		if (i < Agent2->Path.Num()) NodeIndex2 = i;
		if (Agent1->Path[NodeIndex1] == Agent2->Path[NodeIndex2] // Vertex collision
			|| (Agent1->Path[NodeIndex1] == Agent2->Path[NodeIndex2-1]
				&& Agent1->Path[NodeIndex1-1] == Agent2->Path[NodeIndex2])) // Edge collision
					{
			Result->Agent1 = Agent1;
			Result->Agent2 = Agent2;
			Result->Node1 = Agent1->Path[NodeIndex1];
			Result->Node2 = Agent2->Path[NodeIndex2];
			Result->TimeStep = i + 1; // 1 for 0-based index
			return Result;
					}
	}
	return nullptr;
}

void CBS::ResolveConflict(Conflict* Con)
{
	AShip* Agent1 = Con->Agent1;
	AShip* Agent2 = Con->Agent2;
	CTNode* Root = new CTNode(Agent1, Agent2);
	std::priority_queue<CTNode*, std::vector<CTNode*>, CTNodeCompare> OpenQueue;
	OpenQueue.push(Root);
	Conflict* CurrentConflict = Con;
	CTNode* CurrentNode = Root;
	
	int counter = 0;
	
	while(!OpenQueue.empty())
	{
		CurrentNode = OpenQueue.top();  // Loop Invariant: CurrentNode is the node with the lowest cost
		OpenQueue.pop();

		// Root won't trigger
		if (CurrentNode != Root) CurrentConflict = FindConflict(CurrentNode->ConstrainedAgent, CurrentNode->OtherAgent);
		if (CurrentConflict == nullptr) break; 
		
		Constraint* Constraint1 = new Constraint{ CurrentConflict->Node2, CurrentConflict->Node1, CurrentConflict->TimeStep };
		CTNode* Node1 = new CTNode(Agent1, Agent2, CurrentNode->Constraints1, CurrentNode->Constraints2);
		Node1->Constraints1.Add(Constraint1);
		Node1->FindSolution();
		OpenQueue.push(Node1);

		Constraint* Constraint2 = new Constraint{ CurrentConflict->Node1, CurrentConflict->Node2, CurrentConflict->TimeStep };
		CTNode* Node2 = new CTNode(Agent2, Agent1, CurrentNode->Constraints2, CurrentNode->Constraints1);
		Node2->Constraints2.Add(Constraint2);
		Node2->FindSolution();
		OpenQueue.push(Node2);
		
		// depth of a tree
		counter++;
		if (FMath::IsPowerOfTwo(counter + 1))
		{
			continue;
		}
	}

	CurrentNode->AcceptSolution();
}

void CTNode::FindSolution()
{
	ConstrainedAgent->LevelGenerator->AStar(ConstrainedAgent, Constraints1);
	Solution1 = ConstrainedAgent->Path;
	Solution2 = OtherAgent->Path;
	Cost = ConstrainedAgent->PathCost + OtherAgent->PathCost;
}

void CTNode::AcceptSolution()
{
	ConstrainedAgent->Path = Solution1;
	ConstrainedAgent->Constraints.Append(Constraints1);
	OtherAgent->Path = Solution2;
	OtherAgent->Constraints.Append(Constraints2);
}
