#include "StatisticsExporter.h"

void StatisticsExporter::AddScenarioLog(ScenarioLog Log)
{
	ScenarioLogs.Add(Log);
}

void StatisticsExporter::ScenarioLogCSV()
{
	FString CSV = ScenarioLog::CSVHeader() + "\n";
	for (ScenarioLog Log : ScenarioLogs)
	{
		CSV += Log.ToCSV() + "\n";
	}
	
	FString Path = GetPath("Scenario");
	if (FFileHelper::SaveStringToFile(CSV, *Path))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("File Successfully Saved At %s "), *Path));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("File Failed to save At %s "), *Path));
	}
}

void StatisticsExporter::AddShipLog(ShipLog Log)
{
	ShipLogs.Add(Log);
}

void StatisticsExporter::ShipLogCSV()
{
	FString CSV = ShipLog::CSVHeader() + "\n";
	for (ShipLog Log : ShipLogs)
	{
		CSV += Log.ToCSV() + "\n";
	}
	
	FString Path = GetPath("Ship");
	if (FFileHelper::SaveStringToFile(CSV, *Path))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("File Successfully Saved At %s "), *Path));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("File Failed to save At %s "), *Path));
	}
}

FString StatisticsExporter::GetPath(FString FileName)
{
	// Get the current timestamp
	FDateTime CurrentTime = FDateTime::Now();
	FString Timestamp = CurrentTime.ToString();

	// Replace any characters in the timestamp that are not valid in filenames
	Timestamp.ReplaceInline(TEXT(":"), TEXT("-"));
	Timestamp.ReplaceInline(TEXT(" "), TEXT("_"));

	// Append the timestamp to the filename
	const FString Result = FPaths::ProjectContentDir() + "Logs/" + FileName + Timestamp + ".csv";
	return *Result;
}