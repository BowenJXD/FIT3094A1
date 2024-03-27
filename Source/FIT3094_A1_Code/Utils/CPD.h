#pragma once

#include <queue>
#include <unordered_map>
#include <vector>

#include "FIT3094_A1_Code/GridNode.h"
#include "FIT3094_A1_Code/LevelGenerator.h"
#include "Misc/LazySingleton.h"

struct CompareNodeH {
	bool operator()(const GridNode* a, const GridNode* b) const {
		return a->H > b->H;
	}
};

using NodeQueue = std::priority_queue<GridNode*, std::vector<GridNode*>, CompareNodeH>;
using FirstMoveMap = std::unordered_map<GridNode*, EDir>;
using WorldArray = GridNode*[ALevelGenerator::MAX_MAP_SIZE][ALevelGenerator::MAX_MAP_SIZE];

class CPD
{
public:

	static CPD& Get()
	{
		return TLazySingleton<CPD>::Get();
	}

	static void BackwardUniformCostSearch(GridNode* Target, WorldArray World);

	static TMap<EDir, GridNode*> GetNeighbors(const GridNode* Node, WorldArray World);

};
