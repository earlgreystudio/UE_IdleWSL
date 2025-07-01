#include "BTTask_ExecuteCombat.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/GridMapComponent.h"
#include "../../AI/IdleAIController.h"
#include "../../C_PlayerController.h"
#include "../../Services/CombatService.h"
#include "Kismet/GameplayStatics.h"

UBTTask_ExecuteCombat::UBTTask_ExecuteCombat()
{
	NodeName = TEXT("Execute Combat");
	
	// Blackboardキーの設定
	TargetEnemyKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ExecuteCombat, TargetEnemyKey));
	CombatResultKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ExecuteCombat, CombatResultKey));
}

EBTNodeResult::Type UBTTask_ExecuteCombat::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCombat: Missing character or blackboard"));
		return EBTNodeResult::Failed;
	}
	
	// Blackboardから戦闘対象を取得
	FString TargetEnemy = Blackboard->GetValueAsString(TargetEnemyKey.SelectedKeyName);
	if (TargetEnemy.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCombat: No target enemy specified"));
		return EBTNodeResult::Failed;
	}
	
	UE_LOG(LogTemp, Log, TEXT("BTTask_ExecuteCombat: %s executing combat against %s"), 
		*Character->GetName(), *TargetEnemy);
	
	// 戦闘実行
	bool bCombatSuccess = ExecuteCombatAt(OwnerComp, TargetEnemy);
	
	// 結果をBlackboardに設定
	Blackboard->SetValueAsBool(CombatResultKey.SelectedKeyName, bCombatSuccess);
	
	if (bCombatSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_ExecuteCombat: %s successfully completed combat against %s"), 
			*Character->GetName(), *TargetEnemy);
		return EBTNodeResult::Succeeded;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCombat: %s failed combat against %s"), 
			*Character->GetName(), *TargetEnemy);
		return EBTNodeResult::Failed;
	}
}

FString UBTTask_ExecuteCombat::GetStaticDescription() const
{
	return FString::Printf(TEXT("Execute Combat: %s → %s"), 
		*TargetEnemyKey.SelectedKeyName.ToString(),
		*CombatResultKey.SelectedKeyName.ToString());
}

UTaskManagerComponent* UBTTask_ExecuteCombat::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

UGridMapComponent* UBTTask_ExecuteCombat::GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* AIController = Cast<AIdleAIController>(OwnerComp.GetAIOwner()))
	{
		return AIController->GetGridMapComponent();
	}
	return nullptr;
}

int32 UBTTask_ExecuteCombat::GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		return -1;
	}
	
	// チームインデックスをBlackboardから取得（BTService_UpdateCurrentTaskで設定済み）
	if (auto* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		return Blackboard->GetValueAsInt(TEXT("TeamIndex"));
	}
	
	return 0; // デフォルト
}

bool UBTTask_ExecuteCombat::ExecuteCombatAt(UBehaviorTreeComponent& OwnerComp, const FString& TargetEnemy) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		return false;
	}
	
	// CombatServiceを使用して戦闘実行
	// 注意：CombatServiceが実装されていることを前提
	
	try 
	{
		// 現在位置での戦闘実行
		FIntPoint CurrentGrid = Character->GetCurrentGridPosition();
		
		UE_LOG(LogTemp, Log, TEXT("BTTask_ExecuteCombat: Executing combat at grid (%d, %d) against %s"), 
			CurrentGrid.X, CurrentGrid.Y, *TargetEnemy);
		
		// 簡易戦闘実装
		// 実際のCombatServiceとの統合は後で実装
		
		// 暫定実装：戦闘は常に成功とする
		// 実際の実装では、Character stats, enemy stats, equipment などを考慮
		bool bSuccess = true;
		
		if (bSuccess)
		{
			// 戦闘完了の処理
			// - 経験値獲得
			// - アイテムドロップ
			// - タスク進行更新
			
			auto* TaskManager = GetTaskManagerComponent(OwnerComp);
			if (TaskManager)
			{
				// タスク進行を更新（1回の戦闘で1進行）
				TaskManager->UpdateTaskProgress(TargetEnemy, 1);
			}
		}
		
		return bSuccess;
	}
	catch (...)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_ExecuteCombat: Error during combat execution"));
		return false;
	}
}