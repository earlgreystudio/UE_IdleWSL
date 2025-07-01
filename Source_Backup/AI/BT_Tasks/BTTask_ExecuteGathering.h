#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_ExecuteGathering.generated.h"

class UGridMapComponent;
class UTaskManagerComponent;
class UInventoryComponent;

/**
 * 採集実行タスク
 * 現在位置で採集を実行し、アイテムをインベントリに追加
 */
UCLASS()
class UE_IDLE_API UBTTask_ExecuteGathering : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ExecuteGathering();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	// Blackboardキー：採集対象アイテム
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
	FBlackboardKeySelector TargetItemKey;
	
	// 採集時間（秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
	float GatheringTime = 2.0f;
	
	// 即座に完了するかどうか
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
	bool bInstantComplete = true;
	
private:
	// ヘルパー関数
	UGridMapComponent* GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const;
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	bool CanGatherAtCurrentLocation(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const;
	bool ExecuteGatheringAction(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const;
};