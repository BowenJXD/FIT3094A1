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
 AShip* ConstrainedAgent;
 AShip* OtherAgent;
 TArray<Constraint*> Constraints1;
 TArray<Constraint*> Constraints2;
 TArray<GridNode*> Solution1;
 TArray<GridNode*> Solution2;
 unsigned int Cost;

 CTNode(AShip* Agent1, AShip* Agent2, TArray<Constraint*> Constraints1 = TArray<Constraint*>(), TArray<Constraint*> Constraints2 = TArray<Constraint*>())
 {
  this->ConstrainedAgent = Agent1;
  this->OtherAgent = Agent2;
  this->Constraints1 = Constraints1;
  this->Constraints2 = Constraints2;
 }

 void FindSolution();

 void AcceptSolution();
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
 constexpr static int MaxIteration = 20;
 
 static void Execute(TArray<AShip*> Agents);
 static Conflict* FindConflict(AShip* Agent1, AShip* Agent2);
 static void ResolveConflict(Conflict* Con);
};
