#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_IdleWait.generated.h"

/**
 * シンプルな待機タスク
 * 指定された時間待機するか、即座に完了する
 */
UCLASS()
class UE_IDLE_API UBTTask_IdleWait : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_IdleWait();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	// 待機時間（秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wait")
	float WaitTime = 1.0f;
	
	// 即座に完了するかどうか
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wait")
	bool bInstantComplete = true;
};