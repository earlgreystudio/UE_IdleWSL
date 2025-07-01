#include "BTTask_IdleWait.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "../../Actor/C_IdleCharacter.h"

UBTTask_IdleWait::UBTTask_IdleWait()
{
	NodeName = TEXT("Idle Wait");
	// bInstantCompleteはデフォルトでtrueに設定されているため、bNotifyTickはfalse
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_IdleWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	if (!AIOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_IdleWait: No valid AI Owner found"));
		return EBTNodeResult::Failed;
	}
	
	AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(AIOwner->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_IdleWait: No valid character found"));
		return EBTNodeResult::Failed;
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTTask_IdleWait: Character is waiting"));
	
	// 即座に完了する場合
	if (bInstantComplete)
	{
		return EBTNodeResult::Succeeded;
	}
	
	// 時間待機の場合（将来実装）
	return EBTNodeResult::InProgress;
}

FString UBTTask_IdleWait::GetStaticDescription() const
{
	if (bInstantComplete)
	{
		return TEXT("Wait (Instant)");
	}
	else
	{
		return TEXT("Wait (Timed)");
	}
}