#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "UE_Idle/Types/LocationTypes.h"
#include "LocationDataTableManager.generated.h"

UCLASS(BlueprintType)
class UE_IDLE_API ULocationDataTableManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // DataTable設定
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    void SetLocationDataTable(UDataTable* InDataTable);

    // Core location data access
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    bool GetLocationData(const FString& LocationId, FLocationDataRow& OutLocationData) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    FLocationDataRow GetLocationDataByRowName(const FName& RowName) const;

    // Location queries
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    bool IsValidLocation(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    bool HasGatherableItems(const FString& LocationId) const;

    // Blueprint-friendly data retrieval
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FLocationDataRow> GetAllLocations() const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FLocationDataRow> GetLocationsByType(ELocationType LocationType) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FName> GetAllLocationRowNames() const;

    // Location specific data access
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    FString GetLocationDisplayName(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    FString GetLocationDescription(const FString& LocationId) const;

    // 移動システム関連（新マップ生成システム用）
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    float GetLocationMovementCost(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    float GetLocationMovementDifficulty(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    int32 GetLocationDifficultyLevel(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    bool IsLocationWalkable(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FGatherableItemInfo> GetGatherableItems(const FString& LocationId) const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FString> GetLocationEnemiesList(const FString& LocationId) const;

    // Utility functions
    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FString> GetAllValidLocationIds() const;

    UFUNCTION(BlueprintCallable, Category = "Location Manager")
    TArray<FString> GetGatherableLocationIds() const;

protected:
    // DataTable reference - set this in Blueprint or assign programmatically
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    UDataTable* LocationDataTable;

private:
    // Helper function to find location by LocationId field (not row name)
    const FLocationDataRow* FindLocationByLocationId(const FString& LocationId) const;
};