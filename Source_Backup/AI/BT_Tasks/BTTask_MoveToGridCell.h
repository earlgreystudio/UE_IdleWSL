#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToGridCell.generated.h"

class UGridMapComponent;

/**
 * グリッドセルへの移動タスク
 * A*パス探索を使用してグリッド上を移動
 */
UCLASS()
class UE_IDLE_API UBTTask_MoveToGridCell : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_MoveToGridCell();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	// Blackboardキー：目標グリッド位置
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Movement")
	FBlackboardKeySelector TargetGridPositionKey;
	
	// 受け入れ可能距離（グリッド単位）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Movement", meta = (ClampMin = "0.1"))
	float AcceptanceDistance = 0.5f;
	
private:
	// 移動状態管理
	struct FMoveTaskMemory
	{
		TArray<FIntPoint> Path;
		int32 CurrentPathIndex;
		FVector TargetWorldPosition;
		bool bMovementStarted;
		
		FMoveTaskMemory()
		{
			CurrentPathIndex = 0;
			TargetWorldPosition = FVector::ZeroVector;
			bMovementStarted = false;
		}
	};
	
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMoveTaskMemory); }
	
	// ヘルパー関数
	UGridMapComponent* GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const;
	bool CalculatePath(UBehaviorTreeComponent& OwnerComp, FMoveTaskMemory* TaskMemory);
	bool UpdateMovement(UBehaviorTreeComponent& OwnerComp, FMoveTaskMemory* TaskMemory, float DeltaSeconds);
	bool HasReachedTarget(UBehaviorTreeComponent& OwnerComp, FMoveTaskMemory* TaskMemory) const;
};