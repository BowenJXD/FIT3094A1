#pragma once

class ShipLog
{
public:
	int ScenarioShipCount;
	FString ShipName;
	int CellsExpanded;
	int PathCost;
	int ActionAmount;

	ShipLog();
	
	const static FString CSVHeader();
	
	FString ToCSV();
};
