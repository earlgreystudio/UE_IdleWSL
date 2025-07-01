#include "BTTask_IdleWait.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"

UBTTask_IdleWait::UBTTask_IdleWait()
{
	NodeName = TEXT("Idle Wait");
	bNotifyTick = !bInstantComplete;
}

EBTNodeResult::Type UBTTask_IdleWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_IdleWait: No valid character found"));
		return EBTNodeResult::Failed;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("BTTask_IdleWait: %s is waiting"), *Character->GetName());
	
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
		return FString::Printf(TEXT("Wait (Instant)"));
	}
	else
	{
		return FString::Printf(TEXT("Wait %.1fs"), WaitTime);
	}
}