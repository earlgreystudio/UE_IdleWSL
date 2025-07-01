#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCurrentTask.generated.h"

class UTaskManagerComponent;
class UGridMapComponent;

/**
 * 現在タスク更新サービス
 * Blackboardの状況情報を定期的に更新
 */
UCLASS()
class UE_IDLE_API UBTService_UpdateCurrentTask : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_UpdateCurrentTask();
	
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
	
	// 更新するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task Update")
	FBlackboardKeySelector CurrentLocationKey;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task Update")
	FBlackboardKeySelector TeamIndexKey;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task Update")
	FBlackboardKeySelector InventoryFullKey;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task Update")
	FBlackboardKeySelector CurrentGridPositionKey;
	
private:
	// ヘルパー関数
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	UGridMapComponent* GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const;
	int32 GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const;
	bool IsInventoryFull(UBehaviorTreeComponent& OwnerComp) const;
	void UpdateLocationInfo(UBehaviorTreeComponent& OwnerComp) const;
	void UpdateTaskInfo(UBehaviorTreeComponent& OwnerComp) const;
	void UpdateInventoryInfo(UBehaviorTreeComponent& OwnerComp) const;
};