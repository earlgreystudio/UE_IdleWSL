#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToGridCell.generated.h"

class UGridMapComponent;
class AC_IdleCharacter;

/**
 * グリッドベースの移動タスク
 * キャラクターを指定されたグリッド座標に移動させる
 */
UCLASS()
class UE_IDLE_API UBTTask_MoveToGridCell : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToGridCell();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

public:
	// 目標グリッド座標を指定するBlackboardキー
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Movement")
	FBlackboardKeySelector TargetGridPositionKey;
	
	// 移動速度（グリッド単位/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Movement")
	float MovementSpeed = 2.0f;
	
	// パス探索に失敗した場合の動作
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Movement")
	bool bFailOnNoPath = true;

private:
	struct FBTMoveToGridCellMemory
	{
		TArray<FIntPoint> Path;
		int32 CurrentPathIndex;
		FVector StartWorldPosition;
		FVector TargetWorldPosition;
		float MoveStartTime;
		bool bIsMoving;
		
		FBTMoveToGridCellMemory()
		{
			CurrentPathIndex = 0;
			StartWorldPosition = FVector::ZeroVector;
			TargetWorldPosition = FVector::ZeroVector;
			MoveStartTime = 0.0f;
			bIsMoving = false;
		}
	};
	
	// ヘルパー関数
	UGridMapComponent* GetGridMapComponent(UBehaviorTreeComponent& OwnerComp);
	AC_IdleCharacter* GetIdleCharacter(UBehaviorTreeComponent& OwnerComp);
	bool StartMovementToNextCell(UBehaviorTreeComponent& OwnerComp, FBTMoveToGridCellMemory* Memory);
};