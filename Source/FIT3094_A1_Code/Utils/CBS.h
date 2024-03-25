#pragma once
#include "FIT3094_A1_Code/Ship.h"

struct Conflict
{
 AShip* Agent1;
 AShip* Agent2;
 GridNode* Node1;
 GridNode* Node2;
 unsigned int TimeStep;
};

struct Constraint
{
 GridNode* Start;
 GridNode* End;
 unsigned int TimeStep;
};

struct CTNode
{
public:
 CTNode* Parent;
 AShip* Agent1;
 AShip* Agent2;
 TMap<AShip*, TArray<GridNode*>> Solution;
 unsigned int Cost;

 CTNode(CTNode* Parent, AShip* Agent1, AShip* Agent2)
 {
  this->Parent = Parent;
  this->Agent1 = Agent1;
  this->Agent2 = Agent2;
 }

 Conflict* FindConflict();
 void FindSolution(AShip* ConstrainedAgent);
};

/**
 * @brief smaller cost is higher priority
 */
struct CTNodeCompare
{
 bool operator()(const CTNode* a, const CTNode* b) const
 {
  return a->Cost > b->Cost;
 }
};

/**
 * @brief Conflict-based search
 */
class CBS
{
public: 
 static void Execute(TArray<AShip*> Agents);
 static Conflict* FindConflict(AShip* Agent1, AShip* Agent2);
 static void ResolveConflict(Conflict* Con);
};
