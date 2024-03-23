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
	return "Scenario,Start,End,ShipName,CellsExpanded,PathCost,ActionAmount";
}

FString ShipLog::ToCSV()
{
	FString result = "";
	result += FString::FromInt(ScenarioShipCount) + ",";
	result += FString::FromInt(Start.X) + "." + FString::FromInt(Start.Y) + ",";
	result += FString::FromInt(End.X) + "." + FString::FromInt(End.Y) + ",";
	result += ShipName + ",";
	result += FString::FromInt(CellsExpanded) + ",";
	result += FString::FromInt(PathCost) + ",";
	result += FString::FromInt(ActionAmount);
	return result;
}