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
	return ScenarioShipCount + "," +
		ShipName + "," +
		FString::FromInt(CellsExpanded) + "," +
		FString::FromInt(PathCost) + "," +
		FString::FromInt(ActionAmount);
}