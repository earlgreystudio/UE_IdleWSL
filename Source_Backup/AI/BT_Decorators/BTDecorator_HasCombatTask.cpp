#include "BTDecorator_HasCombatTask.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/TeamComponent.h"
#include "../../C_PlayerController.h"
#include "../../Types/TaskTypes.h"
#include "../../Types/TeamTypes.h"
#include "Kismet/GameplayStatics.h"

UBTDecorator_HasCombatTask::UBTDecorator_HasCombatTask()
{
	NodeName = TEXT("Has Combat Task");
	
	// Blackboardキーの設定
	TargetEnemyKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasCombatTask, TargetEnemyKey));
}

bool UBTDecorator_HasCombatTask::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* TaskManager = GetTaskManagerComponent(OwnerComp);
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !TaskManager || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTDecorator_HasCombatTask: Missing required components"));
		return false;
	}
	
	// キャラクターのチームインデックス取得
	int32 TeamIndex = GetCharacterTeamIndex(OwnerComp);
	if (TeamIndex == -1)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCombatTask: %s not in any team"), *Character->GetName());
		return false;
	}
	
	// 戦闘タスクの確認
	FString TargetEnemy;
	bool bHasTask = HasCombatTaskForTeam(TaskManager, TeamIndex, TargetEnemy);
	
	if (bHasTask && !TargetEnemy.IsEmpty())
	{
		// Blackboardに戦闘対象敵を設定
		Blackboard->SetValueAsString(TargetEnemyKey.SelectedKeyName, TargetEnemy);
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCombatTask: %s has combat task for %s"), 
			*Character->GetName(), *TargetEnemy);
		return true;
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCombatTask: %s has no combat task"), 
		*Character->GetName());
	return false;
}

FString UBTDecorator_HasCombatTask::GetStaticDescription() const
{
	return FString::Printf(TEXT("Has Combat Task → %s"), 
		*TargetEnemyKey.SelectedKeyName.ToString());
}

UTaskManagerComponent* UBTDecorator_HasCombatTask::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

int32 UBTDecorator_HasCombatTask::GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const
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

bool UBTDecorator_HasCombatTask::HasCombatTaskForTeam(UTaskManagerComponent* TaskManager, int32 TeamIndex, FString& OutTargetEnemy) const
{
	if (!TaskManager)
	{
		return false;
	}
	
	// TaskManagerから実際のタスクデータを取得
	FGlobalTask NextTask = TaskManager->GetNextAvailableTask(TeamIndex);
	
	// 戦闘タスクかどうかをチェック
	if (NextTask.TaskType == ETaskType::Combat && !NextTask.bIsCompleted)
	{
		OutTargetEnemy = NextTask.TargetItemId; // 戦闘では敵の名前がTargetItemIdに入る
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCombatTask: Team %d has combat task for %s (Priority: %d)"), 
			TeamIndex, *OutTargetEnemy, NextTask.Priority);
		
		return !OutTargetEnemy.IsEmpty();
	}
	
	// 戦闘タスクが見つからない場合
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_HasCombatTask: Team %d has no combat task (Current task type: %d)"), 
		TeamIndex, (int32)NextTask.TaskType);
	
	OutTargetEnemy.Empty();
	return false;
}