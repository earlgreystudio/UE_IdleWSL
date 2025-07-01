#include "EnvQueryContext_GridCells.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "../../Components/GridMapComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../AI/IdleAIController.h"
#include "Engine/World.h"
#include "AIController.h"

UEnvQueryContext_GridCells::UEnvQueryContext_GridCells()
{
    SearchRadius = 5;
    bWalkableOnly = true;
    MaxResults = 50;
    TargetLocationTag = FGameplayTag::EmptyTag;
}

void UEnvQueryContext_GridCells::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
    // GridMapComponentを取得
    UGridMapComponent* GridMap = GetGridMapComponent(QueryInstance);
    if (!GridMap)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnvQueryContext_GridCells: GridMapComponent not found"));
        return;
    }

    // クエリ元の位置取得
    FIntPoint QuerierGrid = GetQuerierGridPosition(QueryInstance);
    
    // セル位置生成
    TArray<FVector> CellLocations;
    GenerateCellsInRadius(QuerierGrid, SearchRadius, GridMap, CellLocations);

    // 距離でソート
    FVector QuerierWorldPos = GridMap->GridToWorld(QuerierGrid);
    SortLocationsByDistance(CellLocations, QuerierWorldPos);

    // 最大結果数で制限
    if (CellLocations.Num() > MaxResults)
    {
        CellLocations.SetNum(MaxResults);
    }

    // EQSコンテキストデータに設定
    UEnvQueryItemType_Point::SetContextHelper(ContextData, CellLocations);

    UE_LOG(LogTemp, VeryVerbose, TEXT("EnvQueryContext_GridCells: Generated %d cell locations around (%d, %d)"), 
        CellLocations.Num(), QuerierGrid.X, QuerierGrid.Y);
}

// === Private Implementation ===

FIntPoint UEnvQueryContext_GridCells::GetQuerierGridPosition(const FEnvQueryInstance& QueryInstance) const
{
    // クエリ元のActorを取得
    AActor* QuerierActor = Cast<AActor>(QueryInstance.Owner.Get());
    if (!QuerierActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnvQueryContext_GridCells: No querier actor found"));
        return FIntPoint(0, 0);
    }

    // AC_IdleCharacterの場合はグリッド位置を直接取得
    if (AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(QuerierActor))
    {
        return Character->GetCurrentGridPosition();
    }

    // その他の場合はワールド位置からグリッド位置を計算
    UGridMapComponent* GridMap = GetGridMapComponent(QueryInstance);
    if (GridMap)
    {
        return GridMap->WorldToGrid(QuerierActor->GetActorLocation());
    }

    return FIntPoint(0, 0);
}

UGridMapComponent* UEnvQueryContext_GridCells::GetGridMapComponent(const FEnvQueryInstance& QueryInstance) const
{
    // AIControllerからGridMapを取得
    AActor* QuerierActor = Cast<AActor>(QueryInstance.Owner.Get());
    if (!QuerierActor)
    {
        return nullptr;
    }

    // AIController経由で取得
    if (AAIController* AIController = Cast<AAIController>(QuerierActor))
    {
        if (AIdleAIController* IdleAI = Cast<AIdleAIController>(AIController))
        {
            return IdleAI->GetGridMapComponent();
        }
    }

    // PawnのAIController経由で取得
    if (APawn* Pawn = Cast<APawn>(QuerierActor))
    {
        if (AIdleAIController* IdleAI = Cast<AIdleAIController>(Pawn->GetController()))
        {
            return IdleAI->GetGridMapComponent();
        }
    }

    // ワールドからGameModeやPlayerController経由で取得（フォールバック）
    UWorld* World = QuerierActor->GetWorld();
    if (World)
    {
        // 他の方法でGridMapComponentを探す
        // 例：ゲームモードやシングルトンから取得
    }

    return nullptr;
}

void UEnvQueryContext_GridCells::GenerateCellsInRadius(const FIntPoint& CenterGrid, int32 Radius, 
    const UGridMapComponent* GridMap, TArray<FVector>& OutLocations) const
{
    if (!GridMap)
    {
        return;
    }

    OutLocations.Reset();

    // 範囲内のセルを生成
    for (int32 dx = -Radius; dx <= Radius; dx++)
    {
        for (int32 dy = -Radius; dy <= Radius; dy++)
        {
            FIntPoint CellPos = CenterGrid + FIntPoint(dx, dy);
            
            // セルが有効かつフィルター条件に合致するかチェック
            if (GridMap->IsValidCell(CellPos) && ShouldIncludeCell(CellPos, GridMap))
            {
                FVector WorldPos = GridMap->GridToWorld(CellPos);
                OutLocations.Add(WorldPos);
            }
        }
    }
}

bool UEnvQueryContext_GridCells::ShouldIncludeCell(const FIntPoint& CellPos, const UGridMapComponent* GridMap) const
{
    if (!GridMap)
    {
        return false;
    }

    FGridCellData CellData = GridMap->GetCellData(CellPos);

    // 歩行可能性チェック
    if (bWalkableOnly && !CellData.bIsWalkable)
    {
        return false;
    }

    // GameplayTagフィルター
    if (TargetLocationTag.IsValid() && !TargetLocationTag.IsEmpty())
    {
        if (CellData.LocationType != TargetLocationTag)
        {
            return false;
        }
    }

    return true;
}

void UEnvQueryContext_GridCells::SortLocationsByDistance(TArray<FVector>& Locations, const FVector& Origin) const
{
    // 距離でソート（近い順）
    Locations.Sort([Origin](const FVector& A, const FVector& B)
    {
        float DistA = FVector::DistSquared(A, Origin);
        float DistB = FVector::DistSquared(B, Origin);
        return DistA < DistB;
    });
}