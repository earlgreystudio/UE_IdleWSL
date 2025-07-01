#include "BTDecorator_InventoryFull.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/InventoryComponent.h"

UBTDecorator_InventoryFull::UBTDecorator_InventoryFull()
{
	NodeName = TEXT("Inventory Full");
}

bool UBTDecorator_InventoryFull::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTDecorator_InventoryFull: Missing character"));
		return false;
	}
	
	bool bIsFull = IsInventoryFull(OwnerComp);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTDecorator_InventoryFull: %s inventory full check = %s"), 
		*Character->GetName(), bIsFull ? TEXT("true") : TEXT("false"));
	
	return bIsFull;
}

FString UBTDecorator_InventoryFull::GetStaticDescription() const
{
	return FString::Printf(TEXT("Inventory Full (>= %.1f%%)"), FullThreshold * 100.0f);
}

bool UBTDecorator_InventoryFull::IsInventoryFull(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		return false;
	}
	
	auto* Inventory = Character->GetInventoryComponent();
	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTDecorator_InventoryFull: Character %s has no inventory component"), 
			*Character->GetName());
		return false;
	}
	
	// インベントリの使用率をチェック
	// 注意：InventoryComponentの実装に依存
	// 暫定実装：基本的なロジック
	
	try 
	{
		// InventoryComponentの実際のAPIを使用
		// 例：float UsageRatio = Inventory->GetCapacityUsageRatio();
		// return UsageRatio >= FullThreshold;
		
		// 暫定実装：Blackboardから取得（BTService_UpdateCurrentTaskで設定済み）
		if (auto* Blackboard = OwnerComp.GetBlackboardComponent())
		{
			return Blackboard->GetValueAsBool(TEXT("InventoryFull"));
		}
		
		// フォールバック：常にfalse
		return false;
	}
	catch (...)
	{
		UE_LOG(LogTemp, Error, TEXT("BTDecorator_InventoryFull: Error checking inventory"));
		return false;
	}
}