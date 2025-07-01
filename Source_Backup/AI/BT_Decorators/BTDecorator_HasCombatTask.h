#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasCombatTask.generated.h"

class UTaskManagerComponent;

/**
 * 戦闘タスク有無判定デコレーター
 * チームに戦闘タスクがあるかチェック
 */
UCLASS()
class UE_IDLE_API UBTDecorator_HasCombatTask : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_HasCombatTask();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	
	// 対象敵情報を設定するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Task")
	FBlackboardKeySelector TargetEnemyKey;
	
private:
	// ヘルパー関数
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	int32 GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const;
	bool HasCombatTaskForTeam(UTaskManagerComponent* TaskManager, int32 TeamIndex, FString& OutTargetEnemy) const;
};