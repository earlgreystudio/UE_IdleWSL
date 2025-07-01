#include "GridMapComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UGridMapComponent::UGridMapComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridMapComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// テスト用グリッド初期化
	InitializeGrid();
}

FVector UGridMapComponent::GridToWorld(const FIntPoint& GridPos) const
{
	return FVector(
		GridPos.X * CellSize,
		GridPos.Y * CellSize,
		0.0f
	);
}

FIntPoint UGridMapComponent::WorldToGrid(const FVector& WorldPos) const
{
	return FIntPoint(
		FMath::FloorToInt(WorldPos.X / CellSize),
		FMath::FloorToInt(WorldPos.Y / CellSize)
	);
}

FGridCellData UGridMapComponent::GetCellData(const FIntPoint& GridPos) const
{
	if (const FGridCellData* CellData = GridData.Find(GridPos))
	{
		return *CellData;
	}
	
	// デフォルトセルデータを返す
	FGridCellData DefaultData;
	DefaultData.LocationType = FGameplayTag::RequestGameplayTag(FName("Location.Plains"));
	return DefaultData;
}

bool UGridMapComponent::SetCellData(const FIntPoint& GridPos, const FGridCellData& CellData)
{
	if (IsValidGridPosition(GridPos))
	{
		GridData.Add(GridPos, CellData);
		return true;
	}
	return false;
}

TArray<FIntPoint> UGridMapComponent::GetAdjacentCells(const FIntPoint& GridPos) const
{
	TArray<FIntPoint> AdjacentCells;
	
	// 4方向の隣接セル
	TArray<FIntPoint> Directions = {
		FIntPoint(0, 1),   // 北
		FIntPoint(1, 0),   // 東
		FIntPoint(0, -1),  // 南
		FIntPoint(-1, 0)   // 西
	};
	
	for (const FIntPoint& Direction : Directions)
	{
		FIntPoint Adjacent = GridPos + Direction;
		if (IsValidGridPosition(Adjacent))
		{
			AdjacentCells.Add(Adjacent);
		}
	}
	
	return AdjacentCells;
}

bool UGridMapComponent::IsValidGridPosition(const FIntPoint& GridPos) const
{
	return GridPos.X >= 0 && GridPos.X < GridWidth &&
	       GridPos.Y >= 0 && GridPos.Y < GridHeight;
}

TArray<FIntPoint> UGridMapComponent::FindPath(const FIntPoint& Start, const FIntPoint& Goal)
{
	TArray<FIntPoint> Path;
	
	if (!IsValidGridPosition(Start) || !IsValidGridPosition(Goal))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridMapComponent: Invalid start or goal position"));
		return Path;
	}
	
	if (Start == Goal)
	{
		Path.Add(Start);
		return Path;
	}
	
	// A*アルゴリズムの簡易実装
	TSet<FIntPoint> OpenSet;
	TSet<FIntPoint> ClosedSet;
	TMap<FIntPoint, FIntPoint> CameFrom;
	TMap<FIntPoint, float> GScore;
	TMap<FIntPoint, float> FScore;
	
	OpenSet.Add(Start);
	GScore.Add(Start, 0.0f);
	FScore.Add(Start, CalculateHeuristic(Start, Goal));
	
	while (OpenSet.Num() > 0)
	{
		// 最小FScoreのノードを取得
		FIntPoint Current = Start;
		float MinFScore = FLT_MAX;
		for (const FIntPoint& Node : OpenSet)
		{
			float Score = FScore.FindRef(Node);
			if (Score < MinFScore)
			{
				MinFScore = Score;
				Current = Node;
			}
		}
		
		if (Current == Goal)
		{
			// パス再構築
			return ReconstructPath(CameFrom, Current);
		}
		
		OpenSet.Remove(Current);
		ClosedSet.Add(Current);
		
		// 隣接セルを調査
		TArray<FIntPoint> Neighbors = GetAdjacentCells(Current);
		for (const FIntPoint& Neighbor : Neighbors)
		{
			if (ClosedSet.Contains(Neighbor))
			{
				continue;
			}
			
			// 移動可能かチェック
			FGridCellData CellData = GetCellData(Neighbor);
			if (!CellData.bIsWalkable)
			{
				continue;
			}
			
			float TentativeGScore = GScore.FindRef(Current) + CellData.MovementCost;
			
			if (!OpenSet.Contains(Neighbor))
			{
				OpenSet.Add(Neighbor);
			}
			else if (TentativeGScore >= GScore.FindRef(Neighbor))
			{
				continue;
			}
			
			CameFrom.Add(Neighbor, Current);
			GScore.Add(Neighbor, TentativeGScore);
			FScore.Add(Neighbor, TentativeGScore + CalculateHeuristic(Neighbor, Goal));
		}
	}
	
	// パスが見つからない
	UE_LOG(LogTemp, Warning, TEXT("GridMapComponent: No path found from (%d,%d) to (%d,%d)"), 
		Start.X, Start.Y, Goal.X, Goal.Y);
	return Path;
}

FIntPoint UGridMapComponent::FindNearestCellWithTag(const FIntPoint& Origin, FGameplayTag RequiredTag)
{
	float MinDistance = FLT_MAX;
	FIntPoint NearestCell = FIntPoint::ZeroValue;
	bool bFound = false;
	
	for (const auto& Cell : GridData)
	{
		if (Cell.Value.LocationType == RequiredTag)
		{
			float Distance = FVector::Dist2D(
				FVector(Origin.X, Origin.Y, 0),
				FVector(Cell.Key.X, Cell.Key.Y, 0)
			);
			
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				NearestCell = Cell.Key;
				bFound = true;
			}
		}
	}
	
	if (!bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridMapComponent: No cell found with tag %s"), 
			*RequiredTag.ToString());
		return FIntPoint(-1, -1); // 無効な位置を示す
	}
	
	return NearestCell;
}

TArray<FIntPoint> UGridMapComponent::GetCellsWithTag(FGameplayTag LocationTag)
{
	TArray<FIntPoint> Cells;
	
	for (const auto& Cell : GridData)
	{
		if (Cell.Value.LocationType == LocationTag)
		{
			Cells.Add(Cell.Key);
		}
	}
	
	return Cells;
}

void UGridMapComponent::InitializeGrid()
{
	UE_LOG(LogTemp, Warning, TEXT("GridMapComponent: Initializing %dx%d grid"), GridWidth, GridHeight);
	
	GridData.Empty();
	
	// テスト用のシンプルなグリッド作成
	CreateTestGrid();
	
	UE_LOG(LogTemp, Warning, TEXT("GridMapComponent: Grid initialized with %d cells"), GridData.Num());
}

void UGridMapComponent::CreateTestGrid()
{
	// GameplayTagsを作成（エラー回避のため文字列で作成）
	FGameplayTag BaseTag = FGameplayTag::RequestGameplayTag(FName("Location.Base"));
	FGameplayTag ForestTag = FGameplayTag::RequestGameplayTag(FName("Location.Forest"));
	FGameplayTag PlainsTag = FGameplayTag::RequestGameplayTag(FName("Location.Plains"));
	FGameplayTag MountainTag = FGameplayTag::RequestGameplayTag(FName("Location.Mountain"));
	
	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		for (int32 X = 0; X < GridWidth; X++)
		{
			FIntPoint GridPos(X, Y);
			FGridCellData CellData;
			
			// 中央を拠点に
			if (X == GridWidth / 2 && Y == GridHeight / 2)
			{
				CellData.LocationType = BaseTag;
			}
			// 端を山に
			else if (X == 0 || X == GridWidth - 1 || Y == 0 || Y == GridHeight - 1)
			{
				CellData.LocationType = MountainTag;
				CellData.MovementCost = 2.0f;
			}
			// ランダムで森と平原
			else
			{
				if (FMath::RandRange(0, 100) < 30)
				{
					CellData.LocationType = ForestTag;
					// 森には木材リソース
					CellData.AvailableResources.Add(FGameplayTag::RequestGameplayTag(FName("Resource.Wood")));
				}
				else
				{
					CellData.LocationType = PlainsTag;
					// 平原には食料リソース
					CellData.AvailableResources.Add(FGameplayTag::RequestGameplayTag(FName("Resource.Food")));
				}
			}
			
			GridData.Add(GridPos, CellData);
		}
	}
}

float UGridMapComponent::CalculateHeuristic(const FIntPoint& Start, const FIntPoint& Goal) const
{
	// マンハッタン距離
	return FMath::Abs(Start.X - Goal.X) + FMath::Abs(Start.Y - Goal.Y);
}

TArray<FIntPoint> UGridMapComponent::ReconstructPath(const TMap<FIntPoint, FIntPoint>& CameFrom, FIntPoint Current) const
{
	TArray<FIntPoint> Path;
	Path.Add(Current);
	
	while (CameFrom.Contains(Current))
	{
		Current = CameFrom[Current];
		Path.Insert(Current, 0);
	}
	
	return Path;
}