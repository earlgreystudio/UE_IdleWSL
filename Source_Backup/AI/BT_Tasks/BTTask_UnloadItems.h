#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UnloadItems.generated.h"

class UInventoryComponent;

/**
 * アイテム荷下ろしタスク
 * キャラクターのインベントリをグローバルストレージに移動
 */
UCLASS()
class UE_IDLE_API UBTTask_UnloadItems : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_UnloadItems();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
	// 荷下ろし結果を出力するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unload")
	FBlackboardKeySelector UnloadResultKey;
	
private:
	// ヘルパー関数
	UInventoryComponent* GetCharacterInventory(UBehaviorTreeComponent& OwnerComp) const;
	UInventoryComponent* GetGlobalInventory(UBehaviorTreeComponent& OwnerComp) const;
	bool TransferAllItemsToGlobal(UBehaviorTreeComponent& OwnerComp) const;
};