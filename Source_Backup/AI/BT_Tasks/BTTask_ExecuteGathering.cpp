#include "BTTask_ExecuteGathering.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/GridMapComponent.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/InventoryComponent.h"
#include "../../AI/IdleAIController.h"
#include "../../C_PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBTTask_ExecuteGathering::UBTTask_ExecuteGathering()
{
	NodeName = TEXT("Execute Gathering");
	bNotifyTick = !bInstantComplete;
	
	// Blackboardキーの設定
	TargetItemKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ExecuteGathering, TargetItemKey));
}

EBTNodeResult::Type UBTTask_ExecuteGathering::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !Blackboard)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_ExecuteGathering: Invalid character or blackboard"));
		return EBTNodeResult::Failed;
	}
	
	// 採集対象アイテム取得
	FString TargetItem = Blackboard->GetValueAsString(TargetItemKey.SelectedKeyName);
	if (TargetItem.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteGathering: No target item specified"));
		return EBTNodeResult::Failed;
	}
	
	// 現在位置で採集可能かチェック
	if (!CanGatherAtCurrentLocation(OwnerComp, TargetItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteGathering: %s cannot gather %s at current location"), 
			*Character->GetName(), *TargetItem);
		return EBTNodeResult::Failed;
	}
	
	// 採集実行
	if (ExecuteGatheringAction(OwnerComp, TargetItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteGathering: %s successfully gathered %s"), 
			*Character->GetName(), *TargetItem);
		return EBTNodeResult::Succeeded;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteGathering: %s failed to gather %s"), 
			*Character->GetName(), *TargetItem);
		return EBTNodeResult::Failed;
	}
}

FString UBTTask_ExecuteGathering::GetStaticDescription() const
{
	return FString::Printf(TEXT("Execute Gathering (%s)"), 
		*TargetItemKey.SelectedKeyName.ToString());
}

UGridMapComponent* UBTTask_ExecuteGathering::GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* AIController = Cast<AIdleAIController>(OwnerComp.GetAIOwner()))
	{
		return AIController->GetGridMapComponent();
	}
	return nullptr;
}

UTaskManagerComponent* UBTTask_ExecuteGathering::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

bool UBTTask_ExecuteGathering::CanGatherAtCurrentLocation(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* GridMap = GetGridMapComponent(OwnerComp);
	
	if (!Character || !GridMap)
	{
		return false;
	}
	
	// 現在のグリッド位置
	FIntPoint CurrentGrid = Character->GetCurrentGridPosition();
	
	// グリッドセルのデータを取得
	FGridCellData CellData = GridMap->GetCellData(CurrentGrid);
	
	// 採集可能リソースをチェック
	FGameplayTag TargetResourceTag = FGameplayTag::RequestGameplayTag(*FString::Printf(TEXT("Resource.%s"), *TargetItem));
	
	return CellData.AvailableResources.Contains(TargetResourceTag);
}

bool UBTTask_ExecuteGathering::ExecuteGatheringAction(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* TaskManager = GetTaskManagerComponent(OwnerComp);
	
	if (!Character || !TaskManager)
	{
		return false;
	}
	
	// キャラクターのチームインデックスを取得
	int32 TeamIndex = -1;
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		if (auto* TeamComp = PC->FindComponentByClass<UTeamComponent>())
		{
			// チーム検索（簡易実装）
			for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
			{
				TArray<AC_IdleCharacter*> TeamMembers = TeamComp->GetTeamMembers(i);
				if (TeamMembers.Contains(Character))
				{
					TeamIndex = i;
					break;
				}
			}
		}
	}
	
	if (TeamIndex == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteGathering: Character %s not found in any team"), *Character->GetName());
		// デフォルトでチーム0を使用
		TeamIndex = 0;
	}
	
	// 現在位置情報
	FIntPoint CurrentGrid = Character->GetCurrentGridPosition();
	FString LocationId = FString::Printf(TEXT("Grid_%d_%d"), CurrentGrid.X, CurrentGrid.Y);
	
	// TaskManagerを通じて採集実行
	bool bSuccess = TaskManager->ExecuteGathering(TeamIndex, TargetItem, LocationId);
	
	if (bSuccess)
	{
		// 採集成功後の処理
		// アイテムは既にTaskManager::ExecuteGatheringで追加済み
		
		// Blackboardの状態更新
		if (auto* Blackboard = OwnerComp.GetBlackboardComponent())
		{
			Blackboard->SetValueAsString("LastGatheredItem", TargetItem);
			Blackboard->SetValueAsString("LastAction", "Gathering");
		}
	}
	
	return bSuccess;
}