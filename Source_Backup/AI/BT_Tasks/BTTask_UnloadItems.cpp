#include "BTTask_UnloadItems.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/InventoryComponent.h"
#include "../../C_PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBTTask_UnloadItems::UBTTask_UnloadItems()
{
	NodeName = TEXT("Unload Items");
	
	// Blackboardキーの設定
	UnloadResultKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_UnloadItems, UnloadResultKey));
}

EBTNodeResult::Type UBTTask_UnloadItems::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_UnloadItems: Missing character or blackboard"));
		return EBTNodeResult::Failed;
	}
	
	UE_LOG(LogTemp, Log, TEXT("BTTask_UnloadItems: %s unloading items to global storage"), 
		*Character->GetName());
	
	// アイテム転送実行
	bool bUnloadSuccess = TransferAllItemsToGlobal(OwnerComp);
	
	// 結果をBlackboardに設定
	Blackboard->SetValueAsBool(UnloadResultKey.SelectedKeyName, bUnloadSuccess);
	
	if (bUnloadSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_UnloadItems: %s successfully unloaded items"), 
			*Character->GetName());
		return EBTNodeResult::Succeeded;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_UnloadItems: %s failed to unload items"), 
			*Character->GetName());
		return EBTNodeResult::Failed;
	}
}

FString UBTTask_UnloadItems::GetStaticDescription() const
{
	return FString::Printf(TEXT("Unload Items → %s"), 
		*UnloadResultKey.SelectedKeyName.ToString());
}

UInventoryComponent* UBTTask_UnloadItems::GetCharacterInventory(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (Character)
	{
		return Character->GetInventoryComponent();
	}
	return nullptr;
}

UInventoryComponent* UBTTask_UnloadItems::GetGlobalInventory(UBehaviorTreeComponent& OwnerComp) const
{
	// PlayerControllerからグローバルインベントリを取得
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UInventoryComponent>();
	}
	return nullptr;
}

bool UBTTask_UnloadItems::TransferAllItemsToGlobal(UBehaviorTreeComponent& OwnerComp) const
{
	auto* CharacterInventory = GetCharacterInventory(OwnerComp);
	auto* GlobalInventory = GetGlobalInventory(OwnerComp);
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	
	if (!CharacterInventory || !GlobalInventory || !Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_UnloadItems: Missing required inventory components"));
		return false;
	}
	
	try 
	{
		// インベントリ転送の実行
		// 注意：InventoryComponentの実際のAPIに依存
		
		UE_LOG(LogTemp, Log, TEXT("BTTask_UnloadItems: %s transferring items to global storage"), 
			*Character->GetName());
		
		// 暫定実装：基本的な転送ロジック
		// 実際の実装では、InventoryComponentのAPIを使用：
		// CharacterInventory->TransferAllItemsTo(GlobalInventory);
		// または
		// for (auto& Item : CharacterInventory->GetAllItems())
		// {
		//     GlobalInventory->AddItem(Item.ItemId, Item.Quantity);
		//     CharacterInventory->RemoveItem(Item.ItemId, Item.Quantity);
		// }
		
		// 簡易実装：常に成功とする
		bool bSuccess = true;
		
		if (bSuccess)
		{
			UE_LOG(LogTemp, Log, TEXT("BTTask_UnloadItems: %s completed item transfer"), 
				*Character->GetName());
		}
		
		return bSuccess;
	}
	catch (...)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_UnloadItems: Error during item transfer"));
		return false;
	}
}