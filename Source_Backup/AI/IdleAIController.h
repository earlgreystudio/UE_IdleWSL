#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "IdleAIController.generated.h"

class UGridMapComponent;

UCLASS()
class UE_IDLE_API AIdleAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AIdleAIController();
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
public:
	// Behavior Tree資産
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* IdleCharacterBT;
	
	// Blackboardデータ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBlackboardData* IdleCharacterBB;
	
	// グリッドマップ参照取得
	UFUNCTION(BlueprintCallable, Category = "AI")
	UGridMapComponent* GetGridMapComponent() const;
	
	// 毎ターンBehavior Tree再開始
	UFUNCTION(BlueprintCallable, Category = "AI")
	void RestartBehaviorTree();
	
protected:
	// グリッドマップ参照
	UPROPERTY()
	UGridMapComponent* GridMapRef;
	
	// ターン管理用タイマー
	FTimerHandle TurnTimer;
	
	// ターンティック処理
	void OnTurnTick();
	
	// Behavior Tree開始
	void StartBehaviorTree();
	
private:
	// 初期化完了フラグ
	bool bIsInitialized = false;
};