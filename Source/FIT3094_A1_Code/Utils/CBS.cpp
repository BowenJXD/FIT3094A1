#include "CBS.h"

#include "FIT3094_A1_Code/LevelGenerator.h"

void CBS::Execute(TArray<AShip*> Agents)
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

	for (int i = 0; i < Conflicts.Num(); i++)
	{
		ResolveConflict(Conflicts[i]);
	}
}

Conflict* CBS::FindConflict(AShip* Agent1, AShip* Agent2)
{	
	int Min = FMath::Min(Agent1->Path.Num(), Agent2->Path.Num());
	Conflict* Result = new Conflict();
	for(int i = 1; i < Min; i++)
	{
		if (Agent1->Path[i] == Agent2->Path[i] // Vertex collision
			|| (Agent1->Path[i] == Agent2->Path[i-1]
				&& Agent2->Path[i] == Agent1->Path[i-1])) // Edge collision
					{
			Result->Agent1 = Agent1;
			Result->Agent2 = Agent2;
			Result->Node1 = Agent1->Path[i];
			Result->Node2 = Agent2->Path[i];
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
	CTNode* Root = new CTNode(nullptr, Agent1, Agent2);
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
		if (CurrentNode != Root) CurrentConflict = CurrentNode->FindConflict(); 
		if (CurrentConflict == nullptr) break; 
		
		Constraint* Constraint1 = new Constraint{ CurrentConflict->Node2, CurrentConflict->Node1, CurrentConflict->TimeStep };
		Agent1->Constraints.Add(Constraint1);
		CTNode* Node1 = new CTNode(CurrentNode, Agent1, Agent2);
		Node1->FindSolution(Agent1);
		OpenQueue.push(Node1);

		Constraint* Constraint2 = new Constraint{ CurrentConflict->Node1, CurrentConflict->Node2, CurrentConflict->TimeStep };
		Agent2->Constraints.Add(Constraint2);
		CTNode* Node2 = new CTNode(CurrentNode, Agent2, Agent1);
		Node2->FindSolution(Agent2);
		OpenQueue.push(Node2);
		
		// depth of a tree
		counter++;
		if (FMath::IsPowerOfTwo(counter + 1))
		{
			continue;
		}
	}

	// Loop through Node's Solution
	for (auto& Solution : CurrentNode->Solution)
	{
		Solution.Key->Path = Solution.Value;
		Solution.Key->LevelGenerator->RenderPath(Solution.Value);
	}
}

Conflict* CTNode::FindConflict()
{
	// TODO: path validation
	
	int Min = FMath::Min(Agent1->Path.Num(), Agent2->Path.Num());
	Conflict* Result = new Conflict();
	for(int i = 1; i < Min; i++)
	{
		if (Agent1->Path[i] == Agent2->Path[i] // Vertex collision
			|| (Agent1->Path[i] == Agent2->Path[i-1]
				&& Agent2->Path[i] == Agent1->Path[i-1])) // Edge collision
		{
			Result->Agent1 = Agent1;
			Result->Agent2 = Agent2;
			Result->Node1 = Agent1->Path[i];
			Result->Node2 = Agent2->Path[i];
			Result->TimeStep = i;
			return Result;
		}
	}
	return nullptr;
}

void CTNode::FindSolution(AShip* ConstrainedAgent)
{
	if (ConstrainedAgent == Agent1)
		Agent1->LevelGenerator->AStar(Agent1);
	if (ConstrainedAgent == Agent2)
		Agent2->LevelGenerator->AStar(Agent2);
	Solution.Add(Agent1, Agent1->Path);
	Solution.Add(Agent2, Agent2->Path);
	Cost = Agent1->PathCost + Agent2->PathCost;
}