#include "BTTask_MoveToGridCell.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/FloatingPawnMovementComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/GridMapComponent.h"
#include "../../AI/IdleAIController.h"

UBTTask_MoveToGridCell::UBTTask_MoveToGridCell()
{
	NodeName = TEXT("Move To Grid Cell");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	
	// Blackboardキーの設定
	TargetGridPositionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToGridCell, TargetGridPositionKey));
}

EBTNodeResult::Type UBTTask_MoveToGridCell::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGridCell: No valid character found"));
		return EBTNodeResult::Failed;
	}
	
	FMoveTaskMemory* TaskMemory = reinterpret_cast<FMoveTaskMemory*>(NodeMemory);
	new(TaskMemory) FMoveTaskMemory();
	
	// パス計算
	if (!CalculatePath(OwnerComp, TaskMemory))
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGridCell: Failed to calculate path for %s"), *Character->GetName());
		return EBTNodeResult::Failed;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGridCell: %s starting movement with %d waypoints"), 
		*Character->GetName(), TaskMemory->Path.Num());
	
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_MoveToGridCell::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (Character && Character->GetMovementComponent())
	{
		Character->GetMovementComponent()->StopMovementImmediately();
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGridCell: Movement aborted for %s"), *Character->GetName());
	}
	
	return EBTNodeResult::Aborted;
}

void UBTTask_MoveToGridCell::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMoveTaskMemory* TaskMemory = reinterpret_cast<FMoveTaskMemory*>(NodeMemory);
	
	// 目標に到達したかチェック
	if (HasReachedTarget(OwnerComp, TaskMemory))
	{
		auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
		if (Character)
		{
			UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGridCell: %s reached target"), *Character->GetName());
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	// 移動更新
	if (!UpdateMovement(OwnerComp, TaskMemory, DeltaSeconds))
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGridCell: Movement update failed"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

FString UBTTask_MoveToGridCell::GetStaticDescription() const
{
	return FString::Printf(TEXT("Move to Grid Cell (%s)"), 
		*TargetGridPositionKey.SelectedKeyName.ToString());
}

UGridMapComponent* UBTTask_MoveToGridCell::GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* AIController = Cast<AIdleAIController>(OwnerComp.GetAIOwner()))
	{
		return AIController->GetGridMapComponent();
	}
	return nullptr;
}

bool UBTTask_MoveToGridCell::CalculatePath(UBehaviorTreeComponent& OwnerComp, FMoveTaskMemory* TaskMemory)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* GridMap = GetGridMapComponent(OwnerComp);
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !GridMap || !Blackboard)
	{
		return false;
	}
	
	// 目標グリッド位置取得
	FVector TargetVector = Blackboard->GetValueAsVector(TargetGridPositionKey.SelectedKeyName);
	FIntPoint TargetGrid = FIntPoint(FMath::RoundToInt(TargetVector.X), FMath::RoundToInt(TargetVector.Y));
	
	// 現在位置
	FIntPoint CurrentGrid = Character->GetCurrentGridPosition();
	
	UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGridCell: Calculating path from (%d,%d) to (%d,%d)"), 
		CurrentGrid.X, CurrentGrid.Y, TargetGrid.X, TargetGrid.Y);
	
	// 既に目標位置にいる場合
	if (CurrentGrid == TargetGrid)
	{
		TaskMemory->Path.Add(CurrentGrid);
		TaskMemory->TargetWorldPosition = GridMap->GridToWorld(TargetGrid);
		return true;
	}
	
	// A*パス探索
	TaskMemory->Path = GridMap->FindPath(CurrentGrid, TargetGrid);
	
	if (TaskMemory->Path.Num() > 0)
	{
		TaskMemory->TargetWorldPosition = GridMap->GridToWorld(TargetGrid);
		TaskMemory->CurrentPathIndex = 0;
		return true;
	}
	
	return false;
}

bool UBTTask_MoveToGridCell::UpdateMovement(UBehaviorTreeComponent& OwnerComp, FMoveTaskMemory* TaskMemory, float DeltaSeconds)
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* GridMap = GetGridMapComponent(OwnerComp);
	
	if (!Character || !GridMap || TaskMemory->Path.Num() == 0)
	{
		return false;
	}
	
	// 現在の目標ウェイポイント
	if (TaskMemory->CurrentPathIndex >= TaskMemory->Path.Num())
	{
		return true; // パス完了
	}
	
	FIntPoint CurrentWaypoint = TaskMemory->Path[TaskMemory->CurrentPathIndex];
	FVector WaypointWorldPos = GridMap->GridToWorld(CurrentWaypoint);
	
	// 現在位置からウェイポイントまでの距離
	FVector CurrentPos = Character->GetActorLocation();
	float DistanceToWaypoint = FVector::Dist2D(CurrentPos, WaypointWorldPos);
	
	// ウェイポイントに到達した場合、次のウェイポイントへ
	if (DistanceToWaypoint < 100.0f) // 1m以内
	{
		TaskMemory->CurrentPathIndex++;
		
		// グリッド位置更新
		Character->SetCurrentGridPosition(CurrentWaypoint);
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("BTTask_MoveToGridCell: %s reached waypoint (%d,%d), %d/%d"), 
			*Character->GetName(), CurrentWaypoint.X, CurrentWaypoint.Y, 
			TaskMemory->CurrentPathIndex, TaskMemory->Path.Num());
		
		if (TaskMemory->CurrentPathIndex >= TaskMemory->Path.Num())
		{
			return true; // 最終目標到達
		}
		
		// 次のウェイポイント取得
		CurrentWaypoint = TaskMemory->Path[TaskMemory->CurrentPathIndex];
		WaypointWorldPos = GridMap->GridToWorld(CurrentWaypoint);
	}
	
	// 移動方向計算
	FVector Direction = (WaypointWorldPos - CurrentPos).GetSafeNormal2D();
	
	// 移動実行
	if (auto* MovementComp = Character->GetMovementComponent())
	{
		MovementComp->AddInputVector(Direction);
	}
	
	return true;
}

bool UBTTask_MoveToGridCell::HasReachedTarget(UBehaviorTreeComponent& OwnerComp, FMoveTaskMemory* TaskMemory) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character || TaskMemory->Path.Num() == 0)
	{
		return false;
	}
	
	// 最終目標位置
	FVector CurrentPos = Character->GetActorLocation();
	float DistanceToTarget = FVector::Dist2D(CurrentPos, TaskMemory->TargetWorldPosition);
	
	return DistanceToTarget < (AcceptanceDistance * 100.0f); // cm単位に変換
}