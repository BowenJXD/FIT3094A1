#pragma once
#include "CPD.h"
#include "ScenarioLog.h"
#include "ShipLog.h"
#include "Misc/LazySingleton.h"

/**
 * Export Log data to CSV
 */
class StatisticsExporter
{
public:

	static StatisticsExporter& Get()
	{
		return TLazySingleton<StatisticsExporter>::Get();
	}

	friend class FLazySingleton;

	void AddScenarioLog(ScenarioLog Log);
	TArray<ScenarioLog> ScenarioLogs;
	void ScenarioLogCSV();

	void AddShipLog(ShipLog Log);
	TArray<ShipLog> ShipLogs;
	void ShipLogCSV();

	static FString GetPath(FString FileName);

	// void FirstMoveMapLogCSV();
};
