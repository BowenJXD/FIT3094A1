#include "ScenarioLog.h"

ScenarioLog::ScenarioLog()
{
	ShipCount = 0;
	TotalCellsExpanded = 0;
	TotalPathLength = 0;
	TotalPathCost = 0;
	TotalTimeTaken = 0;
}

const FString ScenarioLog::CSVHeader()
{
	return "ShipCount,TotalCellsExpanded,TotalPathLength,TotalPathCost,TotalTimeTaken";
}

FString ScenarioLog::ToCSV()
{
	return FString::FromInt(ShipCount) + "," +
		FString::FromInt(TotalCellsExpanded) + "," +
		FString::FromInt(TotalPathLength) + "," +
		FString::FromInt(TotalPathCost) + "," +
		FString::FromInt(TotalTimeTaken);
}