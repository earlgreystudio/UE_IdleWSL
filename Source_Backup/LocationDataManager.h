#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "../Types/LocationTypes.h"
#include "LocationDataManager.generated.h"

// 処理済み場所データ
USTRUCT(BlueprintType)
struct UE_IDLE_API FProcessedLocationData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString LocationId;

    UPROPERTY(BlueprintReadWrite)
    FString Name;

    UPROPERTY(BlueprintReadWrite)
    FString Description;

    UPROPERTY(BlueprintReadWrite)
    FGameplayTag LocationTypeTag; // Location.Plains, Location.Cave等

    UPROPERTY(BlueprintReadWrite)
    TArray<FEnemySpawnInfo> EnemySpawns;

    UPROPERTY(BlueprintReadWrite)
    TArray<FGatherableItemInfo> GatherableItems;

    UPROPERTY(BlueprintReadWrite)
    float MovementCost;

    UPROPERTY(BlueprintReadWrite)
    float MovementDifficulty;

    UPROPERTY(BlueprintReadWrite)
    int32 DifficultyLevel;

    FProcessedLocationData()
    {
        LocationId = TEXT("");
        Name = TEXT("");
        Description = TEXT("");
        LocationTypeTag = FGameplayTag();
        MovementCost = 1.0f;
        MovementDifficulty = 1.0f;
        DifficultyLevel = 1;
    }
};

/**
 * LocationData.csvを読み込み、グリッドシステムと統合するマネージャー
 */
UCLASS()
class UE_IDLE_API ULocationDataManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

protected:
    // DataTable参照
    UPROPERTY()
    TObjectPtr<UDataTable> LocationDataTable;

    // 処理済みデータ
    UPROPERTY()
    TMap<FString, FProcessedLocationData> ProcessedLocationData;

public:
    // === データ読み込み ===

    // DataTable設定
    UFUNCTION(BlueprintCallable, Category = "Location Data")
    void SetLocationDataTable(UDataTable* NewDataTable);

    // CSVデータを処理済みデータに変換
    UFUNCTION(BlueprintCallable, Category = "Location Data")
    void ProcessLocationData();

    // === データ取得 ===

    // 場所データ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Data")
    FProcessedLocationData GetLocationData(const FString& LocationId) const;

    // すべての場所データ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Data")
    TArray<FProcessedLocationData> GetAllLocationData() const;

    // 場所タイプから場所リスト取得
    UFUNCTION(BlueprintCallable, Category = "Location Data")
    TArray<FString> GetLocationsByType(const FString& LocationType) const;

    // === グリッドシステム統合 ===

    // 場所データからFGridCellData作成
    UFUNCTION(BlueprintCallable, Category = "Grid Integration")
    FGridCellData CreateGridCellFromLocation(const FString& LocationId) const;

    // グリッドマップに場所を配置
    UFUNCTION(BlueprintCallable, Category = "Grid Integration")
    void PopulateGridWithLocations(class UGridMapComponent* GridMap, const TMap<FIntPoint, FString>& LocationLayout) const;

    // === 検索・フィルタリング ===

    // リソースを採集できる場所検索
    UFUNCTION(BlueprintCallable, Category = "Location Search")
    TArray<FString> FindLocationsWithResource(const FGameplayTag& ResourceTag) const;

    // 特定敵が出現する場所検索
    UFUNCTION(BlueprintCallable, Category = "Location Search")
    TArray<FString> FindLocationsWithEnemy(const FString& EnemyId) const;

private:
    // === 内部処理 ===

    // 敵スポーンリスト文字列をパース
    TArray<FEnemySpawnInfo> ParseEnemySpawnString(const FString& SpawnString) const;

    // 採集アイテム文字列をパース
    TArray<FGatherableItemInfo> ParseGatherableItemsString(const FString& ItemString) const;

    // LocationTypeをGameplayTagに変換
    FGameplayTag ConvertLocationTypeToTag(const FString& LocationType) const;

    // ResourceIdをGameplayTagに変換
    FGameplayTag ConvertResourceIdToTag(const FString& ResourceId) const;
};