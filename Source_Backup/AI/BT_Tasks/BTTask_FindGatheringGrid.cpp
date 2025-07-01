#include "BTTask_FindGatheringGrid.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/GridMapComponent.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../AI/IdleAIController.h"
#include "../../C_PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBTTask_FindGatheringGrid::UBTTask_FindGatheringGrid()
{
	NodeName = TEXT("Find Gathering Grid");
	
	// Blackboardキーの設定
	TargetItemKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindGatheringGrid, TargetItemKey));
	TargetGridPositionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindGatheringGrid, TargetGridPositionKey));
}

EBTNodeResult::Type UBTTask_FindGatheringGrid::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !Blackboard)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_FindGatheringGrid: Invalid character or blackboard"));
		return EBTNodeResult::Failed;
	}
	
	// 採集対象アイテム取得
	FString TargetItem = Blackboard->GetValueAsString(TargetItemKey.SelectedKeyName);
	if (TargetItem.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_FindGatheringGrid: No target item specified"));
		return EBTNodeResult::Failed;
	}
	
	// 最適な採集場所を検索
	FIntPoint BestLocation = FindBestGatheringLocation(OwnerComp, TargetItem);
	
	if (BestLocation.X == -1 && BestLocation.Y == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_FindGatheringGrid: No suitable gathering location found for %s"), *TargetItem);
		return EBTNodeResult::Failed;
	}
	
	// Blackboardに結果を設定
	FVector GridPosition = FVector(BestLocation.X, BestLocation.Y, 0);
	Blackboard->SetValueAsVector(TargetGridPositionKey.SelectedKeyName, GridPosition);
	
	UE_LOG(LogTemp, Warning, TEXT("BTTask_FindGatheringGrid: %s found gathering location for %s at (%d,%d)"), 
		*Character->GetName(), *TargetItem, BestLocation.X, BestLocation.Y);
	
	return EBTNodeResult::Succeeded;
}

FString UBTTask_FindGatheringGrid::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find Gathering Grid (%s → %s)"), 
		*TargetItemKey.SelectedKeyName.ToString(),
		*TargetGridPositionKey.SelectedKeyName.ToString());
}

UGridMapComponent* UBTTask_FindGatheringGrid::GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* AIController = Cast<AIdleAIController>(OwnerComp.GetAIOwner()))
	{
		return AIController->GetGridMapComponent();
	}
	return nullptr;
}

UTaskManagerComponent* UBTTask_FindGatheringGrid::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

FIntPoint UBTTask_FindGatheringGrid::FindBestGatheringLocation(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* GridMap = GetGridMapComponent(OwnerComp);
	
	if (!Character || !GridMap)
	{
		return FIntPoint(-1, -1);
	}
	
	FIntPoint CharacterPos = Character->GetCurrentGridPosition();
	FGameplayTag ResourceTag = ItemToResourceTag(TargetItem);
	
	// 最適な場所を検索
	FIntPoint BestLocation(-1, -1);
	float BestScore = -1.0f;
	
	// グリッド全体を検索（最適化可能）
	for (int32 x = FMath::Max(0, CharacterPos.X - MaxSearchDistance); 
		 x <= FMath::Min(GridMap->GridWidth - 1, CharacterPos.X + MaxSearchDistance); x++)
	{
		for (int32 y = FMath::Max(0, CharacterPos.Y - MaxSearchDistance); 
			 y <= FMath::Min(GridMap->GridHeight - 1, CharacterPos.Y + MaxSearchDistance); y++)
		{
			FIntPoint TestLocation(x, y);
			FGridCellData CellData = GridMap->GetCellData(TestLocation);
			
			// このセルで目標リソースが採集可能かチェック
			if (CellData.bIsWalkable && CellData.AvailableResources.Contains(ResourceTag))
			{
				float Score = CalculateLocationScore(TestLocation, CharacterPos);
				if (Score > BestScore)
				{
					BestScore = Score;
					BestLocation = TestLocation;
				}
			}
		}
	}
	
	// 効率的な検索（グリッドマップの機能を使用）
	if (BestLocation.X == -1 && BestLocation.Y == -1)
	{
		// フォールバック：全グリッドから最も近い場所を検索
		BestLocation = GridMap->FindNearestCellWithTag(CharacterPos, ResourceTag);
	}
	
	return BestLocation;
}

FGameplayTag UBTTask_FindGatheringGrid::ItemToResourceTag(const FString& ItemName) const
{
	// アイテム名からリソースタグへの変換
	// 例：\"Wood\" → \"Resource.Wood\"
	FString ResourceTagName = FString::Printf(TEXT("Resource.%s"), *ItemName);
	return FGameplayTag::RequestGameplayTag(*ResourceTagName);
}

float UBTTask_FindGatheringGrid::CalculateLocationScore(const FIntPoint& Location, const FIntPoint& CharacterPos) const
{
	// 距離ベースのスコア計算（近いほど高スコア）
	float Distance = FVector::Dist2D(
		FVector(Location.X, Location.Y, 0),
		FVector(CharacterPos.X, CharacterPos.Y, 0)
	);
	
	// 距離が近いほど高スコア、最大スコアは100
	float Score = FMath::Max(0.0f, 100.0f - Distance);
	
	return Score;
}