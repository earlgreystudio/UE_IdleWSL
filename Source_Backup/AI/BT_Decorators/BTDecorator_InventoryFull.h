#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_InventoryFull.generated.h"

/**
 * インベントリ満杯判定デコレーター
 * キャラクターのインベントリが満杯に近いかチェック
 */
UCLASS()
class UE_IDLE_API UBTDecorator_InventoryFull : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_InventoryFull();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	
	// インベントリ満杯の閾値（0.0-1.0）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullThreshold = 0.8f;
	
private:
	// ヘルパー関数
	bool IsInventoryFull(UBehaviorTreeComponent& OwnerComp) const;
};