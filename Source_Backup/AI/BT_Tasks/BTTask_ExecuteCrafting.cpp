#include "BTTask_ExecuteCrafting.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/InventoryComponent.h"
#include "../../C_PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBTTask_ExecuteCrafting::UBTTask_ExecuteCrafting()
{
	NodeName = TEXT("Execute Crafting");
	
	// Blackboardキーの設定
	TargetItemKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ExecuteCrafting, TargetItemKey));
	CraftingResultKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ExecuteCrafting, CraftingResultKey));
}

EBTNodeResult::Type UBTTask_ExecuteCrafting::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCrafting: Missing character or blackboard"));
		return EBTNodeResult::Failed;
	}
	
	// Blackboardから製作対象を取得
	FString TargetItem = Blackboard->GetValueAsString(TargetItemKey.SelectedKeyName);
	if (TargetItem.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCrafting: No target item specified"));
		return EBTNodeResult::Failed;
	}
	
	UE_LOG(LogTemp, Log, TEXT("BTTask_ExecuteCrafting: %s executing crafting for %s"), 
		*Character->GetName(), *TargetItem);
	
	// 製作要件チェック
	if (!CheckCraftingRequirements(OwnerComp, TargetItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCrafting: %s lacks requirements for crafting %s"), 
			*Character->GetName(), *TargetItem);
		Blackboard->SetValueAsBool(CraftingResultKey.SelectedKeyName, false);
		return EBTNodeResult::Failed;
	}
	
	// 製作実行
	bool bCraftingSuccess = ExecuteCraftingFor(OwnerComp, TargetItem);
	
	// 結果をBlackboardに設定
	Blackboard->SetValueAsBool(CraftingResultKey.SelectedKeyName, bCraftingSuccess);
	
	if (bCraftingSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_ExecuteCrafting: %s successfully crafted %s"), 
			*Character->GetName(), *TargetItem);
		return EBTNodeResult::Succeeded;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteCrafting: %s failed to craft %s"), 
			*Character->GetName(), *TargetItem);
		return EBTNodeResult::Failed;
	}
}

FString UBTTask_ExecuteCrafting::GetStaticDescription() const
{
	return FString::Printf(TEXT("Execute Crafting: %s → %s"), 
		*TargetItemKey.SelectedKeyName.ToString(),
		*CraftingResultKey.SelectedKeyName.ToString());
}

UTaskManagerComponent* UBTTask_ExecuteCrafting::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

UInventoryComponent* UBTTask_ExecuteCrafting::GetCharacterInventory(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (Character)
	{
		return Character->GetInventoryComponent();
	}
	return nullptr;
}

int32 UBTTask_ExecuteCrafting::GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const
{
	// チームインデックスをBlackboardから取得（BTService_UpdateCurrentTaskで設定済み）
	if (auto* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		return Blackboard->GetValueAsInt(TEXT("TeamIndex"));
	}
	
	return 0; // デフォルト
}

bool UBTTask_ExecuteCrafting::CheckCraftingRequirements(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const
{
	auto* Inventory = GetCharacterInventory(OwnerComp);
	if (!Inventory)
	{
		return false;
	}
	
	// 製作要件のチェック（簡易実装）
	// 実際の実装では、レシピデータから必要素材をチェック
	
	// 暫定実装：基本的な製作は常に可能とする
	// 実際の実装では、ItemDataTableManagerからレシピ情報を取得し、
	// 必要素材がインベントリにあるかチェック
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTTask_ExecuteCrafting: Checking requirements for %s"), *TargetItem);
	
	// 例：木の剣を作るには木材が必要、など
	// if (TargetItem == "WoodenSword")
	// {
	//     return Inventory->HasItem("Wood", 5);
	// }
	
	return true; // 暫定的に常に成功
}

bool UBTTask_ExecuteCrafting::ExecuteCraftingFor(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Inventory = GetCharacterInventory(OwnerComp);
	auto* TaskManager = GetTaskManagerComponent(OwnerComp);
	
	if (!Character || !Inventory || !TaskManager)
	{
		return false;
	}
	
	try 
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_ExecuteCrafting: %s crafting %s"), 
			*Character->GetName(), *TargetItem);
		
		// 製作実行（簡易実装）
		// 実際の実装では：
		// 1. 必要素材をインベントリから消費
		// 2. 製作アイテムをインベントリに追加
		// 3. 経験値獲得
		// 4. タスク進行更新
		
		// 暫定実装：常に成功とする
		bool bSuccess = true;
		
		if (bSuccess)
		{
			// 製作完了の処理
			// - 製作品をインベントリに追加
			// Inventory->AddItem(TargetItem, 1);
			
			// - タスク進行を更新
			TaskManager->UpdateTaskProgress(TargetItem, 1);
			
			// - 経験値獲得（Character->AddExperience など）
		}
		
		return bSuccess;
	}
	catch (...)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_ExecuteCrafting: Error during crafting execution"));
		return false;
	}
}