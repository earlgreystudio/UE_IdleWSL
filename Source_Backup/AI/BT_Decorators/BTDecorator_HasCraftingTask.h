#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasCraftingTask.generated.h"

class UTaskManagerComponent;

/**
 * 製作タスク有無判定デコレーター
 * チームに製作タスクがあるかチェック
 */
UCLASS()
class UE_IDLE_API UBTDecorator_HasCraftingTask : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_HasCraftingTask();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	
	// 製作対象アイテムを設定するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting Task")
	FBlackboardKeySelector TargetItemKey;
	
private:
	// ヘルパー関数
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	int32 GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const;
	bool HasCraftingTaskForTeam(UTaskManagerComponent* TaskManager, int32 TeamIndex, FString& OutTargetItem) const;
};