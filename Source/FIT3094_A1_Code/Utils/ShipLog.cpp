#include "ShipLog.h"

ShipLog::ShipLog()
{
	ScenarioShipCount = 0;
	ShipName = "";
	CellsExpanded = 0;
	PathCost = 0;
	ActionAmount = 0;
}

const FString ShipLog::CSVHeader()
{
	return "Scenario,ShipName,CellsExpanded,PathCost,ActionAmount";
}

FString ShipLog::ToCSV()
{
	FString result = "";
	result += FString::FromInt(ScenarioShipCount) + ",";
	result += ShipName + ",";
	result += FString::FromInt(CellsExpanded) + ",";
	result += FString::FromInt(PathCost) + ",";
	result += FString::FromInt(ActionAmount);
	return result;
}