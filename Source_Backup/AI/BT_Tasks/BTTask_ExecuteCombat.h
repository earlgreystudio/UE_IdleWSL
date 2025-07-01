#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExecuteCombat.generated.h"

class UTaskManagerComponent;
class UGridMapComponent;

/**
 * 戦闘実行タスク
 * 現在地で戦闘を実行
 */
UCLASS()
class UE_IDLE_API UBTTask_ExecuteCombat : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ExecuteCombat();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
	// 戦闘対象を指定するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FBlackboardKeySelector TargetEnemyKey;
	
	// 戦闘結果を出力するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FBlackboardKeySelector CombatResultKey;
	
private:
	// ヘルパー関数
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	UGridMapComponent* GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const;
	int32 GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const;
	bool ExecuteCombatAt(UBehaviorTreeComponent& OwnerComp, const FString& TargetEnemy) const;
};