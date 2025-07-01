#include "BTDecorator_HasCraftingTask.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/TeamComponent.h"
#include "../../C_PlayerController.h"
#include "../../Types/TaskTypes.h"
#include "../../Types/TeamTypes.h"
#include "Kismet/GameplayStatics.h"

UBTDecorator_HasCraftingTask::UBTDecorator_HasCraftingTask()
{
	NodeName = TEXT("Has Crafting Task");
	
	// Blackboardキーの設定
	TargetItemKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasCraftingTask, TargetItemKey));
}

bool UBTDecorator_HasCraftingTask::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* TaskManager = GetTaskManagerComponent(OwnerComp);
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !TaskManager || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTDecorator_HasCraftingTask: Missing required components"));
		return false;
	}
	
	// キャラクターのチームインデックス取得
	int32 TeamIndex = GetCharacterTeamIndex(OwnerComp);
	if (TeamIndex == -1)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCraftingTask: %s not in any team"), *Character->GetName());
		return false;
	}
	
	// 製作タスクの確認
	FString TargetItem;
	bool bHasTask = HasCraftingTaskForTeam(TaskManager, TeamIndex, TargetItem);
	
	if (bHasTask && !TargetItem.IsEmpty())
	{
		// Blackboardに製作対象アイテムを設定
		Blackboard->SetValueAsString(TargetItemKey.SelectedKeyName, TargetItem);
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCraftingTask: %s has crafting task for %s"), 
			*Character->GetName(), *TargetItem);
		return true;
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCraftingTask: %s has no crafting task"), 
		*Character->GetName());
	return false;
}

FString UBTDecorator_HasCraftingTask::GetStaticDescription() const
{
	return FString::Printf(TEXT("Has Crafting Task → %s"), 
		*TargetItemKey.SelectedKeyName.ToString());
}

UTaskManagerComponent* UBTDecorator_HasCraftingTask::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

int32 UBTDecorator_HasCraftingTask::GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const
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

bool UBTDecorator_HasCraftingTask::HasCraftingTaskForTeam(UTaskManagerComponent* TaskManager, int32 TeamIndex, FString& OutTargetItem) const
{
	if (!TaskManager)
	{
		return false;
	}
	
	// TaskManagerから実際のタスクデータを取得
	FGlobalTask NextTask = TaskManager->GetNextAvailableTask(TeamIndex);
	
	// 製作タスクかどうかをチェック
	if (NextTask.TaskType == ETaskType::Crafting && !NextTask.bIsCompleted)
	{
		OutTargetItem = NextTask.TargetItemId;
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCraftingTask: Team %d has crafting task for %s (Priority: %d, Target: %d)"), 
			TeamIndex, *OutTargetItem, NextTask.Priority, NextTask.TargetQuantity);
		
		return !OutTargetItem.IsEmpty();
	}
	
	// 製作タスクが見つからない場合
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCraftingTask: Team %d has no crafting task (Current task type: %d)"), 
		TeamIndex, (int32)NextTask.TaskType);
	
	OutTargetItem.Empty();
	return false;
}