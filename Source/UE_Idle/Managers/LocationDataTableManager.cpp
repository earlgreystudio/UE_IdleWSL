#include "LocationDataTableManager.h"
#include "Engine/DataTable.h"

void ULocationDataTableManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // DataTable will be set in Blueprint or loaded programmatically
    UE_LOG(LogTemp, Log, TEXT("LocationDataTableManager initialized"));
}

void ULocationDataTableManager::SetLocationDataTable(UDataTable* InDataTable)
{
    LocationDataTable = InDataTable;
    
    if (LocationDataTable)
    {
        UE_LOG(LogTemp, Log, TEXT("LocationDataTable set successfully. Row count: %d"), 
            LocationDataTable->GetRowMap().Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LocationDataTable set to null"));
    }
}

bool ULocationDataTableManager::GetLocationData(const FString& LocationId, FLocationDataRow& OutLocationData) const
{
    if (!LocationDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("LocationDataTableManager::GetLocationData - LocationDataTable is not set for LocationId: %s"), *LocationId);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("LocationDataTableManager::GetLocationData - Looking for LocationId: %s"), *LocationId);
    const FLocationDataRow* LocationData = FindLocationByLocationId(LocationId);
    if (LocationData)
    {
        OutLocationData = *LocationData;
        UE_LOG(LogTemp, Log, TEXT("LocationDataTableManager::GetLocationData - Found location data for: %s"), *LocationId);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("LocationDataTableManager::GetLocationData - Location not found: %s"), *LocationId);
    return false;
}

FLocationDataRow ULocationDataTableManager::GetLocationDataByRowName(const FName& RowName) const
{
    if (!LocationDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("LocationDataTable is not set"));
        return FLocationDataRow();
    }

    FLocationDataRow* LocationData = LocationDataTable->FindRow<FLocationDataRow>(RowName, TEXT(""));
    if (LocationData)
    {
        return *LocationData;
    }

    UE_LOG(LogTemp, Warning, TEXT("Location row not found: %s"), *RowName.ToString());
    return FLocationDataRow();
}

bool ULocationDataTableManager::IsValidLocation(const FString& LocationId) const
{
    FLocationDataRow TempData;
    return GetLocationData(LocationId, TempData);
}

bool ULocationDataTableManager::HasGatherableItems(const FString& LocationId) const
{
    FLocationDataRow LocationData;
    if (GetLocationData(LocationId, LocationData))
    {
        return LocationData.HasGatherableItems();
    }
    return false;
}

TArray<FLocationDataRow> ULocationDataTableManager::GetAllLocations() const
{
    TArray<FLocationDataRow> AllLocations;
    
    if (!LocationDataTable)
    {
        return AllLocations;
    }

    TArray<FName> RowNames = LocationDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FLocationDataRow* LocationData = LocationDataTable->FindRow<FLocationDataRow>(RowName, TEXT(""));
        if (LocationData)
        {
            AllLocations.Add(*LocationData);
        }
    }

    return AllLocations;
}

TArray<FLocationDataRow> ULocationDataTableManager::GetLocationsByType(ELocationType LocationType) const
{
    TArray<FLocationDataRow> TypedLocations;
    TArray<FLocationDataRow> AllLocations = GetAllLocations();
    
    for (const FLocationDataRow& Location : AllLocations)
    {
        if (Location.LocationType == LocationType)
        {
            TypedLocations.Add(Location);
        }
    }
    
    return TypedLocations;
}

TArray<FName> ULocationDataTableManager::GetAllLocationRowNames() const
{
    if (!LocationDataTable)
    {
        return TArray<FName>();
    }
    
    return LocationDataTable->GetRowNames();
}

FString ULocationDataTableManager::GetLocationDisplayName(const FString& LocationId) const
{
    FLocationDataRow LocationData;
    if (GetLocationData(LocationId, LocationData))
    {
        return LocationData.Name;
    }
    return FString();
}

FString ULocationDataTableManager::GetLocationDescription(const FString& LocationId) const
{
    FLocationDataRow LocationData;
    if (GetLocationData(LocationId, LocationData))
    {
        return LocationData.Description;
    }
    return FString();
}

float ULocationDataTableManager::GetLocationDistance(const FString& LocationId) const
{
    FLocationDataRow LocationData;
    if (GetLocationData(LocationId, LocationData))
    {
        return LocationData.Distance;
    }
    return 0.0f;
}

TArray<FGatherableItemInfo> ULocationDataTableManager::GetGatherableItems(const FString& LocationId) const
{
    FLocationDataRow LocationData;
    if (GetLocationData(LocationId, LocationData))
    {
        return LocationData.ParseGatherableItemsList();
    }
    return TArray<FGatherableItemInfo>();
}

TArray<FString> ULocationDataTableManager::GetLocationEnemiesList(const FString& LocationId) const
{
    FLocationDataRow LocationData;
    if (GetLocationData(LocationId, LocationData))
    {
        // ParseEnemySpawnListはTArray<FEnemySpawnInfo>を返すので、PresetIdのリストに変換
        TArray<FEnemySpawnInfo> SpawnInfoList = LocationData.ParseEnemySpawnList();
        TArray<FString> EnemyPresetIds;
        
        for (const FEnemySpawnInfo& SpawnInfo : SpawnInfoList)
        {
            EnemyPresetIds.Add(SpawnInfo.PresetId);
        }
        
        return EnemyPresetIds;
    }
    return TArray<FString>();
}

TArray<FString> ULocationDataTableManager::GetAllValidLocationIds() const
{
    TArray<FString> LocationIds;
    
    if (!LocationDataTable)
    {
        return LocationIds;
    }

    TArray<FName> RowNames = LocationDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        LocationIds.Add(RowName.ToString());
    }
    
    return LocationIds;
}

TArray<FString> ULocationDataTableManager::GetGatherableLocationIds() const
{
    TArray<FString> GatherableLocationIds;
    
    if (!LocationDataTable)
    {
        return GatherableLocationIds;
    }

    TArray<FName> RowNames = LocationDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FLocationDataRow* LocationData = LocationDataTable->FindRow<FLocationDataRow>(RowName, TEXT(""));
        if (LocationData && LocationData->HasGatherableItems())
        {
            GatherableLocationIds.Add(RowName.ToString());
        }
    }
    
    return GatherableLocationIds;
}

const FLocationDataRow* ULocationDataTableManager::FindLocationByLocationId(const FString& LocationId) const
{
    if (!LocationDataTable)
    {
        return nullptr;
    }

    // LocationIdはRowNameとして使用される
    FName RowName(*LocationId);
    return LocationDataTable->FindRow<FLocationDataRow>(RowName, TEXT(""));
}