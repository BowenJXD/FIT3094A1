#pragma once

class ScenarioLog
{
public:
	int ShipCount;
	int TotalCellsExpanded;
	int TotalPathLength;
	int TotalPathCost;
	int TotalTimeTaken;

	ScenarioLog();

	const static FString CSVHeader();
	
	FString ToCSV();
};
