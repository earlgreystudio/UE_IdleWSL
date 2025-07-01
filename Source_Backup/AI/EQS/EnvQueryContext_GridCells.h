#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameplayTagContainer.h"
#include "EnvQueryContext_GridCells.generated.h"

class UGridMapComponent;

/**
 * グリッド用軽量EQSコンテキスト
 * グリッドベースの高速検索を提供
 */
UCLASS()
class UE_IDLE_API UEnvQueryContext_GridCells : public UEnvQueryContext
{
    GENERATED_BODY()

public:
    UEnvQueryContext_GridCells();

    // EQSコンテキスト提供
    virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

protected:
    // === 検索設定 ===

    // 検索半径（グリッドセル数）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Search Settings")
    int32 SearchRadius = 5;

    // 検索対象のGameplayTag（空の場合は全セル）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Search Settings")
    FGameplayTag TargetLocationTag;

    // 歩行可能セルのみ検索
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Search Settings")
    bool bWalkableOnly = true;

    // 最大結果数
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Search Settings")
    int32 MaxResults = 50;

private:
    // === ヘルパー関数 ===

    // クエリ元のグリッド位置取得
    FIntPoint GetQuerierGridPosition(const FEnvQueryInstance& QueryInstance) const;

    // GridMapComponentの取得
    UGridMapComponent* GetGridMapComponent(const FEnvQueryInstance& QueryInstance) const;

    // 検索範囲内のセル生成
    void GenerateCellsInRadius(const FIntPoint& CenterGrid, int32 Radius, const UGridMapComponent* GridMap, TArray<FVector>& OutLocations) const;

    // セルのフィルタリング
    bool ShouldIncludeCell(const FIntPoint& CellPos, const UGridMapComponent* GridMap) const;

    // 距離による並び替え
    void SortLocationsByDistance(TArray<FVector>& Locations, const FVector& Origin) const;
};