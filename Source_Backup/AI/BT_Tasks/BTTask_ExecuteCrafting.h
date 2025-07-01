#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExecuteCrafting.generated.h"

class UTaskManagerComponent;
class UInventoryComponent;

/**
 * 製作実行タスク
 * 指定されたアイテムの製作を実行
 */
UCLASS()
class UE_IDLE_API UBTTask_ExecuteCrafting : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ExecuteCrafting();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
	// 製作対象アイテムを指定するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	FBlackboardKeySelector TargetItemKey;
	
	// 製作結果を出力するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	FBlackboardKeySelector CraftingResultKey;
	
private:
	// ヘルパー関数
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	UInventoryComponent* GetCharacterInventory(UBehaviorTreeComponent& OwnerComp) const;
	int32 GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const;
	bool ExecuteCraftingFor(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const;
	bool CheckCraftingRequirements(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const;
};