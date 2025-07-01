#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GridMapComponent.generated.h"

// グリッドセルデータ
USTRUCT(BlueprintType)
struct FGridCellData
{
	GENERATED_BODY()
	
	// 場所タイプ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
	FGameplayTag LocationType; // Location.Base, Location.Forest, Location.Plains
	
	// 移動可能性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
	bool bIsWalkable = true;
	
	// 移動コスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
	float MovementCost = 1.0f;
	
	// 採集可能リソース
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
	TArray<FGameplayTag> AvailableResources; // Resource.Wood, Resource.Stone等
	
	// 視覚表現アクター
	UPROPERTY(BlueprintReadWrite, Category = "Grid Cell")
	class AGridCellActor* CellActor = nullptr;
	
	FGridCellData()
	{
		LocationType = FGameplayTag();
		bIsWalkable = true;
		MovementCost = 1.0f;
		CellActor = nullptr;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UGridMapComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UGridMapComponent();
	
protected:
	virtual void BeginPlay() override;
	
public:
	// グリッド設定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	int32 GridWidth = 20;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	int32 GridHeight = 20;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	float CellSize = 1000.0f; // 10m per cell
	
	// グリッドデータ
	UPROPERTY()
	TMap<FIntPoint, FGridCellData> GridData;
	
	// === 座標変換 ===
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GridToWorld(const FIntPoint& GridPos) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FIntPoint WorldToGrid(const FVector& WorldPos) const;
	
	// === セル情報 ===
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FGridCellData GetCellData(const FIntPoint& GridPos) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool SetCellData(const FIntPoint& GridPos, const FGridCellData& CellData);
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FIntPoint> GetAdjacentCells(const FIntPoint& GridPos) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool IsValidGridPosition(const FIntPoint& GridPos) const;
	
	// === パス探索（A*） ===
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FIntPoint> FindPath(const FIntPoint& Start, const FIntPoint& Goal);
	
	// === 高速検索 ===
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FIntPoint FindNearestCellWithTag(const FIntPoint& Origin, FGameplayTag RequiredTag);
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<FIntPoint> GetCellsWithTag(FGameplayTag LocationTag);
	
	// === 初期化 ===
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeGrid();
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void CreateTestGrid(); // テスト用グリッド作成
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void CreateSimpleTestGrid(); // GameplayTags無しのシンプルグリッド作成
	
private:
	// A*アルゴリズム用ヘルパー
	struct FPathNode
	{
		FIntPoint Position;
		float GCost; // 開始点からのコスト
		float HCost; // ゴールまでの推定コスト
		float FCost; // GCost + HCost
		FIntPoint Parent;
		
		FPathNode()
		{
			Position = FIntPoint::ZeroValue;
			GCost = 0.0f;
			HCost = 0.0f;
			FCost = 0.0f;
			Parent = FIntPoint::ZeroValue;
		}
	};
	
	float CalculateHeuristic(const FIntPoint& Start, const FIntPoint& Goal) const;
	TArray<FIntPoint> ReconstructPath(const TMap<FIntPoint, FIntPoint>& CameFrom, FIntPoint Current) const;
};