#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_FindGatheringGrid.generated.h"

class UGridMapComponent;
class UTaskManagerComponent;

/**
 * 採集場所検索タスク
 * グリッドマップから指定されたリソースを採集できる最適な場所を検索
 */
UCLASS()
class UE_IDLE_API UBTTask_FindGatheringGrid : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FindGatheringGrid();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	// Blackboardキー：採集対象アイテム
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
	FBlackboardKeySelector TargetItemKey;
	
	// Blackboardキー：目標グリッド位置（出力）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
	FBlackboardKeySelector TargetGridPositionKey;
	
	// 最大検索距離（グリッド単位）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
	int32 MaxSearchDistance = 10;
	
private:
	// ヘルパー関数
	UGridMapComponent* GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const;
	UTaskManagerComponent* GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const;
	FIntPoint FindBestGatheringLocation(UBehaviorTreeComponent& OwnerComp, const FString& TargetItem) const;
	FGameplayTag ItemToResourceTag(const FString& ItemName) const;
	float CalculateLocationScore(const FIntPoint& Location, const FIntPoint& CharacterPos) const;
};