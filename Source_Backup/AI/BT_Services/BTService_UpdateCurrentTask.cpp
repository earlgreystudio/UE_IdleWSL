#include "BTService_UpdateCurrentTask.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../../Actor/C_IdleCharacter.h"
#include "../../Components/TaskManagerComponent.h"
#include "../../Components/GridMapComponent.h"
#include "../../Components/TeamComponent.h"
#include "../../Components/InventoryComponent.h"
#include "../../AI/IdleAIController.h"
#include "../../C_PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBTService_UpdateCurrentTask::UBTService_UpdateCurrentTask()
{
	NodeName = TEXT("Update Current Task");
	Interval = 0.5f; // 0.5秒間隔で更新
	RandomDeviation = 0.1f;
	
	// Blackboardキーの設定
	CurrentLocationKey.AddStringFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCurrentTask, CurrentLocationKey));
	TeamIndexKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCurrentTask, TeamIndexKey));
	InventoryFullKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCurrentTask, InventoryFullKey));
	CurrentGridPositionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCurrentTask, CurrentGridPositionKey));
}

void UBTService_UpdateCurrentTask::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTService_UpdateCurrentTask: Missing character or blackboard"));
		return;
	}
	
	// 各種情報を更新
	UpdateLocationInfo(OwnerComp);
	UpdateTaskInfo(OwnerComp);
	UpdateInventoryInfo(OwnerComp);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("BTService_UpdateCurrentTask: Updated info for %s"), *Character->GetName());
}

FString UBTService_UpdateCurrentTask::GetStaticDescription() const
{
	return TEXT("Update Current Task Info");
}

UTaskManagerComponent* UBTService_UpdateCurrentTask::GetTaskManagerComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UTaskManagerComponent>();
	}
	return nullptr;
}

UGridMapComponent* UBTService_UpdateCurrentTask::GetGridMapComponent(UBehaviorTreeComponent& OwnerComp) const
{
	if (auto* AIController = Cast<AIdleAIController>(OwnerComp.GetAIOwner()))
	{
		return AIController->GetGridMapComponent();
	}
	return nullptr;
}

int32 UBTService_UpdateCurrentTask::GetCharacterTeamIndex(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		return -1;
	}
	
	if (auto* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(OwnerComp.GetAIOwner()->GetWorld(), 0)))
	{
		if (auto* TeamComp = PC->FindComponentByClass<UTeamComponent>())
		{
			for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
			{
				TArray<AC_IdleCharacter*> TeamMembers = TeamComp->GetTeamMembers(i);
				if (TeamMembers.Contains(Character))
				{
					return i;
				}
			}
		}
	}
	
	return 0; // デフォルトチーム
}

bool UBTService_UpdateCurrentTask::IsInventoryFull(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Character)
	{
		return false;
	}
	
	if (auto* Inventory = Character->GetInventoryComponent())
	{
		// インベントリの使用率をチェック（例：80%以上で満杯とみなす）
		// 実際の実装はInventoryComponentのAPIに依存
		
		// 暫定実装：常にfalseを返す
		// 実際の実装では、Inventory->GetCapacityUsageRatio() >= 0.8f などを使用
		return false;
	}
	
	return false;
}

void UBTService_UpdateCurrentTask::UpdateLocationInfo(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
	auto* GridMap = GetGridMapComponent(OwnerComp);
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!Character || !GridMap || !Blackboard)
	{
		return;
	}
	
	// グリッド位置情報の更新
	FIntPoint CurrentGrid = Character->GetCurrentGridPosition();
	FVector GridPosition = FVector(CurrentGrid.X, CurrentGrid.Y, 0);
	
	Blackboard->SetValueAsVector(CurrentGridPositionKey.SelectedKeyName, GridPosition);
	
	// 現在位置の場所タイプを取得
	FGridCellData CellData = GridMap->GetCellData(CurrentGrid);
	FString LocationName = CellData.LocationType.ToString();
	
	// \"Location.Forest\" → \"Forest\" に変換
	FString SimplifiedName;
	if (LocationName.Split(TEXT("."), nullptr, &SimplifiedName))
	{
		Blackboard->SetValueAsString(CurrentLocationKey.SelectedKeyName, SimplifiedName);
	}
	else
	{
		Blackboard->SetValueAsString(CurrentLocationKey.SelectedKeyName, LocationName);
	}
}

void UBTService_UpdateCurrentTask::UpdateTaskInfo(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}
	
	// チームインデックスの更新
	int32 TeamIndex = GetCharacterTeamIndex(OwnerComp);
	Blackboard->SetValueAsInt(TeamIndexKey.SelectedKeyName, TeamIndex);
	
	// その他のタスク関連情報の更新
	// 必要に応じて追加
}

void UBTService_UpdateCurrentTask::UpdateInventoryInfo(UBehaviorTreeComponent& OwnerComp) const
{
	auto* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}
	
	// インベントリ満杯状態の更新
	bool bInventoryFull = IsInventoryFull(OwnerComp);
	Blackboard->SetValueAsBool(InventoryFullKey.SelectedKeyName, bInventoryFull);
}