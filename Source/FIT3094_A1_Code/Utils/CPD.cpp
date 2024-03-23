#include "CPD.h"

#include "FIT3094_A1_Code/LevelGenerator.h"

void CPD::BackwardUniformCostSearch(GridNode* Target, WorldArray World) {
	NodeQueue Frontier;

	// Initialize the search
	Target->H = 0;
	Frontier.push(Target);

	// Search loop
	while (!Frontier.empty()) {
		const GridNode* Current = Frontier.top();
		Frontier.pop();

		// Iterate over the neighbors
		auto Neighbors = GetNeighbors(Current, World);
		for (auto& [dir, next] : Neighbors) {
			const float NewCost = Current->H + next->GetTravelCost();
			if (NewCost < next->H) {
				next->H = NewCost;
				Frontier.push(next);

				// Update the first move direction
				next->Direction = dir;
			}
		}
	}
}


TMap<EDir, GridNode*> CPD::GetNeighbors(const GridNode* Node, WorldArray World) {
	TMap<EDir, GridNode*> Neighbors;
	const int x = Node->X;
	const int y = Node->Y;

	if (x > 0) Neighbors[EDir::Up] = World[x - 1][y];
	if (x < ALevelGenerator::MAX_MAP_SIZE - 1) Neighbors[EDir::Down] = World[x + 1][y];
	if (y > 0) Neighbors[EDir::Left] = World[x][y - 1];
	if (y < ALevelGenerator::MAX_MAP_SIZE - 1) Neighbors[EDir::Right] = World[x][y + 1];

	return Neighbors;
}

