#include "BTDecorator_HasGatheringTask.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/TeamComponent.h"
#include "../../C_PlayerController.h"
#include "../../Types/TaskTypes.h"
#include "../../Types/TeamTypes.h"
#include "Kismet/GameplayStatics.h"

UBTDecorator_HasGatheringTask::UBTDecorator_HasGatheringTask()
{
	NodeName = TEXT("Has Gathering Task");
	
	// Blackboardキーの設定
	TargetItemKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasGatheringTask, TargetItemKey));
}

bool UBTDecorator_HasGatheringTask::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* TaskManager = GetTaskManagerComponent(OwnerComp);
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !TaskManager || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTDecorator_HasGatheringTask: Missing required components"));
		return false;
	}
	
	// キャラクターのチームインデックス取得
	int32 TeamIndex = GetCharacterTeamIndex(OwnerComp);
	if (TeamIndex == -1)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasGatheringTask: %s not in any team"), *Character->GetName());
		return false;
	}
	
	// 採集タスクの確認
	FString TargetItem;
	bool bHasTask = HasGatheringTaskForTeam(TaskManager, TeamIndex, TargetItem);
	
	if (bHasTask && !TargetItem.IsEmpty())
	{
		// Blackboardに採集対象アイテムを設定
		Blackboard->SetValueAsString(TargetItemKey.SelectedKeyName, TargetItem);
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasGatheringTask: %s has gathering task for %s"), 
			*Character->GetName(), *TargetItem);
		return true;
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasGatheringTask: %s has no gathering task"), 
		*Character->GetName());
	return false;
}

FString UBTDecorator_HasGatheringTask::GetStaticDescription() const
{
	return FString::Printf(TEXT("Has Gathering Task → %s"), 
		*TargetItemKey.SelectedKeyName.ToString());
}

UTaskManagerComponent* UBTDecorator_HasGatheringTask::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

int32 UBTDecorator_HasGatheringTask::GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		return -1;
	}
	
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		if (auto* TeamComp = PC->FindComponentByClass<UTeamComponent>())
		{
			// チーム検索
			for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
			{
				TArray<AC_IdleCharacter*> TeamMembers = TeamComp->GetTeamMembers(i);
				if (TeamMembers.Contains(Character))
				{
					return i;
				}
			}
		}
	}
	
	// チームが見つからない場合はデフォルトでチーム0
	return 0;
}

bool UBTDecorator_HasGatheringTask::HasGatheringTaskForTeam(UTaskManagerComponent* TaskManager, int32 TeamIndex, FString& OutTargetItem) const
{
	if (!TaskManager)
	{
		return false;
	}
	
	// TaskManagerから実際のタスクデータを取得
	FGlobalTask NextTask = TaskManager->GetNextAvailableTask(TeamIndex);
	
	// 採集タスクかどうかをチェック
	if (NextTask.TaskType == ETaskType::Gathering && !NextTask.bIsCompleted)
	{
		OutTargetItem = NextTask.TargetItemId;
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasGatheringTask: Team %d has gathering task for %s (Priority: %d, Target: %d)"), 
			TeamIndex, *OutTargetItem, NextTask.Priority, NextTask.TargetQuantity);
		
		return !OutTargetItem.IsEmpty();
	}
	
	// 採集タスクが見つからない場合
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasGatheringTask: Team %d has no gathering task (Current task type: %d)"), 
		TeamIndex, (int32)NextTask.TaskType);
	
	OutTargetItem.Empty();
	return false;
}