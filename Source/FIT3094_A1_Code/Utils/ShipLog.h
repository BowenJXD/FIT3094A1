#pragma once

class ShipLog
{
public:
	int ScenarioShipCount;
	FString ShipName;
	FIntPoint Start;
	FIntPoint End;
	int CellsExpanded;
	int PathCost;
	int ActionAmount;

	ShipLog();
	
	const static FString CSVHeader();
	
	FString ToCSV();
};
