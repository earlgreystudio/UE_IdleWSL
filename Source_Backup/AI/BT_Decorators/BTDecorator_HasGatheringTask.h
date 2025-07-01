#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasGatheringTask.generated.h"

class UTaskManagerComponent;

/**
 * 採集タスク判定デコレーター
 * キャラクターのチームが採集タスクを持っているかどうかを判定
 */
UCLASS()
class UE_IDLE_API UBTDecorator_HasGatheringTask : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_HasGatheringTask();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	
	// Blackboardキー：採集対象アイテム（出力）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task Check")
	FBlackboardKeySelector TargetItemKey;
	
private:
	// ヘルパー関数
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	int32 GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const;
	bool HasGatheringTaskForTeam(UTaskManagerComponent* TaskManager, int32 TeamIndex, FString& OutTargetItem) const;
};