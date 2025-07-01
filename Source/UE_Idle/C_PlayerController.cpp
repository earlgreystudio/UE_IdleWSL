// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "Components/InventoryComponent.h"
#include "Components/TeamComponent.h"
#include "Components/EventLogManager.h"
#include "Components/TaskManagerComponent.h"
#include "Components/TimeManagerComponent.h"
#include "Components/CraftingComponent.h"
#include "Components/LocationMovementComponent.h"
#include "Components/CombatComponent.h"
#include "Components/BaseComponent.h"
#include "Components/GridMapComponent.h"
#include "Components/MapGeneratorComponent.h"
#include "Actor/C_IdleCharacter.h"
#include "UI/C__InventoryList.h"
#include "UI/C_TaskList.h"
#include "UI/C_TaskMakeSheet.h"
#include "Blueprint/UserWidget.h"

AC_PlayerController::AC_PlayerController()
{
	// Create components
	GlobalInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("GlobalInventory"));
	if (GlobalInventory)
	{
		// Set owner ID for logging purposes
		GlobalInventory->OwnerId = TEXT("Storage");
	}
	TeamComponent = CreateDefaultSubobject<UTeamComponent>(TEXT("TeamComponent"));
	EventLogManager = CreateDefaultSubobject<UEventLogManager>(TEXT("EventLogManager"));
	
	// Create Task Management System components
	TaskManager = CreateDefaultSubobject<UTaskManagerComponent>(TEXT("TaskManager"));
	TimeManager = CreateDefaultSubobject<UTimeManagerComponent>(TEXT("TimeManager"));
	// GatheringComponent削除済み - TaskManagerに統合
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));
	MovementComponent = CreateDefaultSubobject<ULocationMovementComponent>(TEXT("MovementComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// Create Base Management System component
	BaseComponent = CreateDefaultSubobject<UBaseComponent>(TEXT("BaseComponent"));
	
	// Create Grid Map System component
	GridMapComponent = CreateDefaultSubobject<UGridMapComponent>(TEXT("GridMapComponent"));
	if (GridMapComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridMapComponent created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create GridMapComponent"));
	}
	
	// Create Map Generator component
	MapGeneratorComponent = CreateDefaultSubobject<UMapGeneratorComponent>(TEXT("MapGeneratorComponent"));
	if (MapGeneratorComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapGeneratorComponent created successfully"));
	}

	// Initialize UI variables
	InventoryListWidget = nullptr;
	TaskListWidget = nullptr;
	TaskMakeSheetWidget = nullptr;
}

void AC_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::BeginPlay - Starting initialization"));
	UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::BeginPlay - BaseComponent: %s"), 
		BaseComponent ? TEXT("Valid") : TEXT("NULL"));
	
	// BaseComponentの手動初期化を確実に実行
	if (BaseComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::BeginPlay - Manually calling BaseComponent::InitializeBaseComponent"));
		BaseComponent->InitializeBaseComponent();
	}

	// Auto-initialize inventory UI if widget class is set
	if (InventoryListWidgetClass && GlobalInventory)
	{
		// Create inventory UI widget
		InventoryListWidget = CreateWidget<UC__InventoryList>(this, InventoryListWidgetClass);
		if (InventoryListWidget)
		{
			// Initialize with global inventory
			InventoryListWidget->InitializeWithInventory(GlobalInventory);
			
			// Don't add to viewport yet - let Blueprint/UI system control visibility
			UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Inventory UI initialized successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController: Failed to create inventory UI widget"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: InventoryListWidgetClass not set or GlobalInventory is null"));
	}

	// Initialize Task List UI
	if (TaskListWidgetClass && TaskManager)
	{
		TaskListWidget = CreateWidget<UC_TaskList>(this, TaskListWidgetClass);
		if (TaskListWidget)
		{
			TaskListWidget->InitializeWithTaskManager(TaskManager);
			UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task List UI initialized successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController: Failed to create Task List UI widget"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: TaskListWidgetClass not set or TaskManager is null"));
	}

	// TaskMakeSheet UI は Widget側で自動初期化するため、ここでは作成しない
	// Widget が NativeConstruct で AutoInitializeFromPlayerController() を呼び出す
	UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: TaskMakeSheet UI will auto-initialize from Widget side"));
	
	// Initialize Task Management System
	InitializeTaskManagementSystem();

	// Add debug tasks for testing
	AddDefaultTasks();
	
	// Start the time system for task processing
	StartTaskSystemCpp();
	UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::BeginPlay - Time system started"));

	// Force BaseComponent initialization for testing
	if (BaseComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::BeginPlay - Forcing BaseComponent test facilities"));
		BaseComponent->AddTestFacilities();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController::BeginPlay - BaseComponent is NULL!"));
	}
	
	// TimeSystem already started by StartTaskSystemCpp() above
	
	// Auto-generate test map
	if (MapGeneratorComponent && GridMapComponent)
	{
		MapGeneratorComponent->GenerateMap(GridMapComponent);
		UE_LOG(LogTemp, Warning, TEXT("🗺️ Test map auto-generated"));
		
		// Test grid functionality
		TestGridMapFunctionality();
	}
	
}

void AC_PlayerController::AddItemToStorage_Implementation(const FString& ItemId, int32 Quantity)
{
	if (GlobalInventory)
	{
		GlobalInventory->AddItem(ItemId, Quantity);
	}
}

void AC_PlayerController::AddCharacter_Implementation(AActor* NewCharacter)
{
	if (TeamComponent && NewCharacter)
	{
		// AActorからAC_IdleCharacterにキャスト、またはインターフェースチェック
		if (auto* IdleCharacter = Cast<AC_IdleCharacter>(NewCharacter))
		{
			TeamComponent->AddCharacter(IdleCharacter);
		}
	}
}

UInventoryComponent* AC_PlayerController::GetGlobalInventoryComp_Implementation()
{
	return GlobalInventory;
}

TArray<AActor*> AC_PlayerController::GetCharacterList_Implementation()
{
	TArray<AActor*> Result;
	
	if (TeamComponent)
	{
		auto CharacterList = TeamComponent->GetCharacterList();
		for (auto* IdleCharacter : CharacterList)
		{
			if (IdleCharacter)
			{
				Result.Add(IdleCharacter);
			}
		}
	}
	
	return Result;
}

UTeamComponent* AC_PlayerController::GetTeamComponent_Implementation()
{
	return TeamComponent;
}

void AC_PlayerController::ShowInventoryUI()
{
	if (!InventoryListWidget)
	{
		// Try to create widget if it doesn't exist
		if (InventoryListWidgetClass && GlobalInventory)
		{
			InventoryListWidget = CreateWidget<UC__InventoryList>(this, InventoryListWidgetClass);
			if (InventoryListWidget)
			{
				InventoryListWidget->InitializeWithInventory(GlobalInventory);
			}
		}
	}

	if (InventoryListWidget)
	{
		if (!InventoryListWidget->IsInViewport())
		{
			InventoryListWidget->AddToViewport();
			// Set input mode to UI for interaction
			SetInputMode(FInputModeGameAndUI());
			bShowMouseCursor = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::ShowInventoryUI: InventoryListWidget is null"));
	}
}

void AC_PlayerController::HideInventoryUI()
{
	if (InventoryListWidget && InventoryListWidget->IsInViewport())
	{
		InventoryListWidget->RemoveFromParent();
		// Return input mode to game only
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

UC__InventoryList* AC_PlayerController::GetInventoryUI()
{
	return InventoryListWidget;
}

// === Task Management UI Implementation ===

void AC_PlayerController::ShowTaskListUI()
{
	if (!TaskListWidget)
	{
		// Try to create widget if it doesn't exist
		if (TaskListWidgetClass && TaskManager)
		{
			TaskListWidget = CreateWidget<UC_TaskList>(this, TaskListWidgetClass);
			if (TaskListWidget)
			{
				TaskListWidget->InitializeWithTaskManager(TaskManager);
			}
		}
	}

	if (TaskListWidget)
	{
		if (!TaskListWidget->IsInViewport())
		{
			TaskListWidget->AddToViewport();
			// Set input mode to UI for interaction
			SetInputMode(FInputModeGameAndUI());
			bShowMouseCursor = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::ShowTaskListUI: TaskListWidget is null"));
	}
}

void AC_PlayerController::HideTaskListUI()
{
	if (TaskListWidget && TaskListWidget->IsInViewport())
	{
		TaskListWidget->RemoveFromParent();
		// Return input mode to game only
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

UC_TaskList* AC_PlayerController::GetTaskListUI()
{
	return TaskListWidget;
}

void AC_PlayerController::ShowTaskMakeSheetUI()
{
	UE_LOG(LogTemp, Warning, TEXT("=== ShowTaskMakeSheetUI CALLED ==="));
	UE_LOG(LogTemp, Warning, TEXT("TaskMakeSheetWidget: %s"), TaskMakeSheetWidget ? TEXT("EXISTS") : TEXT("NULL"));
	
	if (!TaskMakeSheetWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("TaskMakeSheetWidget is null, trying to create..."));
		UE_LOG(LogTemp, Warning, TEXT("TaskMakeSheetWidgetClass: %s"), TaskMakeSheetWidgetClass ? TEXT("OK") : TEXT("NULL"));
		UE_LOG(LogTemp, Warning, TEXT("TaskManager: %s"), TaskManager ? TEXT("OK") : TEXT("NULL"));
		UE_LOG(LogTemp, Warning, TEXT("CraftingComponent: %s"), CraftingComponent ? TEXT("OK") : TEXT("NULL"));
		
		// Try to create widget if it doesn't exist
		if (TaskMakeSheetWidgetClass && TaskManager && CraftingComponent)
		{
			TaskMakeSheetWidget = CreateWidget<UC_TaskMakeSheet>(this, TaskMakeSheetWidgetClass);
			if (TaskMakeSheetWidget)
			{
				UE_LOG(LogTemp, Warning, TEXT("TaskMakeSheetWidget created, setting components..."));
				TaskMakeSheetWidget->SetTaskManagerComponent(TaskManager);
				TaskMakeSheetWidget->SetCraftingComponent(CraftingComponent);
				UE_LOG(LogTemp, Warning, TEXT("Components set successfully"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot create TaskMakeSheetWidget - missing requirements"));
		}
	}

	if (TaskMakeSheetWidget)
	{
		if (!TaskMakeSheetWidget->IsInViewport())
		{
			TaskMakeSheetWidget->AddToViewport();
			// Set input mode to UI for interaction
			SetInputMode(FInputModeGameAndUI());
			bShowMouseCursor = true;
			UE_LOG(LogTemp, Warning, TEXT("TaskMakeSheetWidget added to viewport"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController::ShowTaskMakeSheetUI: TaskMakeSheetWidget is still null"));
	}
}

void AC_PlayerController::ShowTaskMakeSheetUIAtPosition(FVector2D Position)
{
	ShowTaskMakeSheetUI(); // 基本表示処理
	
	if (TaskMakeSheetWidget && TaskMakeSheetWidget->IsInViewport())
	{
		TaskMakeSheetWidget->SetPositionInViewport(Position);
		UE_LOG(LogTemp, Log, TEXT("TaskMakeSheet UI positioned at (%f, %f)"), Position.X, Position.Y);
	}
}

void AC_PlayerController::HideTaskMakeSheetUI()
{
	if (TaskMakeSheetWidget && TaskMakeSheetWidget->IsInViewport())
	{
		TaskMakeSheetWidget->RemoveFromParent();
		// Return input mode to game only
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

UC_TaskMakeSheet* AC_PlayerController::GetTaskMakeSheetUI()
{
	return TaskMakeSheetWidget;
}

// === Task Management System Implementation ===

void AC_PlayerController::StartTaskSystemCpp()
{
	if (TimeManager)
	{
		TimeManager->StartTimeSystem();
		UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController: Task Management System started successfully"));
		
		// システム状態をデバッグ出力（PrintDebugInfoメソッドは削除済み）
		// TimeManager->PrintDebugInfo();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController: TimeManager is null, cannot start system"));
	}
}

void AC_PlayerController::AddDefaultTasks()
{
	if (!TaskManager)
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController::AddDefaultTasks - TaskManager is null"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Adding default tasks"));

	// デフォルトタスク1: 料理
	FGlobalTask CookingTask;
	CookingTask.TaskId = TEXT("default_cooking_001");
	CookingTask.DisplayName = TEXT("料理: スープ x3");
	CookingTask.TaskType = ETaskType::Cooking;
	CookingTask.TargetItemId = TEXT("cooking_soup");
	CookingTask.TargetQuantity = 3;
	CookingTask.Priority = 1;
	CookingTask.CurrentProgress = 0;
	CookingTask.bIsCompleted = false;
	CookingTask.bIsKeepQuantity = false;
	CookingTask.CreatedTime = FDateTime::Now();
	CookingTask.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(ETaskType::Cooking);

	// デフォルトタスク2: 建築
	FGlobalTask ConstructionTask;
	ConstructionTask.TaskId = TEXT("default_construction_001");
	ConstructionTask.DisplayName = TEXT("建築: 木の壁 x1");
	ConstructionTask.TaskType = ETaskType::Construction;
	ConstructionTask.TargetItemId = TEXT("construction_wooden_wall");
	ConstructionTask.TargetQuantity = 1;
	ConstructionTask.Priority = 2;
	ConstructionTask.CurrentProgress = 0;
	ConstructionTask.bIsCompleted = false;
	ConstructionTask.bIsKeepQuantity = false;
	ConstructionTask.CreatedTime = FDateTime::Now();
	ConstructionTask.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(ETaskType::Construction);

	// デフォルトタスク3: 冒険
	FGlobalTask AdventureTask;
	AdventureTask.TaskId = TEXT("default_adventure_001");
	AdventureTask.DisplayName = TEXT("冒険: 森の奥地");
	AdventureTask.TaskType = ETaskType::Adventure;
	AdventureTask.TargetItemId = TEXT("森の奥地");
	AdventureTask.TargetQuantity = 1;
	AdventureTask.Priority = 3;
	AdventureTask.CurrentProgress = 0;
	AdventureTask.bIsCompleted = false;
	AdventureTask.bIsKeepQuantity = false;
	AdventureTask.CreatedTime = FDateTime::Now();
	AdventureTask.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(ETaskType::Adventure);

	// デフォルトタスク4: 採集（個数指定タイプに変更）
	FGlobalTask GatheringTask;
	GatheringTask.TaskId = TEXT("default_gathering_001");
	GatheringTask.DisplayName = TEXT("採集: 木材 x10");
	GatheringTask.TaskType = ETaskType::Gathering;
	GatheringTask.TargetItemId = TEXT("wood");
	GatheringTask.TargetQuantity = 5;
	GatheringTask.Priority = 4;
	GatheringTask.CurrentProgress = 0;
	GatheringTask.bIsCompleted = false;
	GatheringTask.bIsKeepQuantity = false;
	GatheringTask.GatheringQuantityType = EGatheringQuantityType::Specified; // 個数指定タイプ
	GatheringTask.CreatedTime = FDateTime::Now();
	GatheringTask.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(ETaskType::Gathering);

	// デバッグ用冒険タスク: 平原
	FGlobalTask PlainsAdventureTask;
	PlainsAdventureTask.TaskId = TEXT("debug_adventure_plains");
	PlainsAdventureTask.DisplayName = TEXT("冒険: 平原");
	PlainsAdventureTask.TaskType = ETaskType::Adventure;
	PlainsAdventureTask.TargetItemId = TEXT("plains");
	PlainsAdventureTask.TargetQuantity = 1;
	PlainsAdventureTask.Priority = 1;
	PlainsAdventureTask.CurrentProgress = 0;
	PlainsAdventureTask.bIsCompleted = false;
	PlainsAdventureTask.bIsKeepQuantity = false;
	PlainsAdventureTask.CreatedTime = FDateTime::Now();
	PlainsAdventureTask.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(ETaskType::Adventure);

	// タスクを追加（デバッグ用冒険タスクと採集タスク）
	TaskManager->AddGlobalTask(PlainsAdventureTask);
	TaskManager->AddGlobalTask(GatheringTask);

	UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Added debug adventure task (plains) and gathering task (wood x10)"));
}

void AC_PlayerController::StopTaskSystemCpp()
{
	if (TimeManager)
	{
		TimeManager->StopTimeSystem();
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task Management System stopped"));
	}
}

// C++専用初期化システム（Blueprint機能とは独立）
void AC_PlayerController::InitializeGatheringSystemCpp()
{
	UE_LOG(LogTemp, Warning, TEXT("=== InitializeGatheringSystemCpp START ==="));
	
	// 強制的に初期化を実行
	InitializeTaskManagementSystem();
	// AddDefaultTasks(); // Default tasks removed - tasks will be created through UI
	
	// システムを開始
	StartTaskSystemCpp();
	
	UE_LOG(LogTemp, Warning, TEXT("=== InitializeGatheringSystemCpp COMPLETED ==="));
}

void AC_PlayerController::InitializeTaskManagementSystem()
{
	// Set up component references
	if (TaskManager && TeamComponent && GlobalInventory)
	{
		// TaskManager references
		TaskManager->SetGlobalInventoryReference(GlobalInventory);
		TaskManager->SetTeamComponentReference(TeamComponent);
		
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: TaskManager references set"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController: Missing components for TaskManager setup"));
	}
	
	if (TimeManager && TaskManager && TeamComponent && MovementComponent)
	{
		// TimeManager references（簡素化により削除済み）
		// TimeManager->RegisterTaskManager(TaskManager);
		// TimeManager->RegisterTeamComponent(TeamComponent);
		// GatheringComponentは削除済み
		// TimeManager->RegisterMovementComponent(MovementComponent);
		
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: TimeManager references set"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController: Missing components for TimeManager setup (TaskManager: %s, TeamComponent: %s, MovementComponent: %s)"), 
			TaskManager ? TEXT("OK") : TEXT("NULL"),
			TeamComponent ? TEXT("OK") : TEXT("NULL"),
			MovementComponent ? TEXT("OK") : TEXT("NULL"));
	}
	
	// GatheringComponent初期化は削除済み - TaskManagerに統合済み
	
	// MovementComponent initialization
	if (MovementComponent && TeamComponent)
	{
		// Set component references for MovementComponent
		MovementComponent->RegisterTeamComponent(TeamComponent);
		
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: MovementComponent references set"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController: Missing components for MovementComponent setup (TeamComponent: %s)"),
			TeamComponent ? TEXT("OK") : TEXT("NULL"));
	}
	
	// Bind combat end event（OnCombatEndedHandlerは削除済み）
	// if (TeamComponent && TimeManager)
	// {
	//     TeamComponent->OnCombatEnded.AddDynamic(TimeManager, &UTimeManagerComponent::OnCombatEndedHandler);
	//     UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Combat end event bound"));
	// }
	
	UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task Management System initialized"));
}

// === デバッグ用採集テスト関数 ===
void AC_PlayerController::TestGatheringSetup()
{
	if (!TeamComponent || !TimeManager)
	{
		UE_LOG(LogTemp, Error, TEXT("TestGatheringSetup: Missing required components"));
		return;
	}

	// チーム作成とテスト
	if (TeamComponent->GetTeamCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TestGatheringSetup: No teams found, creating test team"));
		int32 TestTeamIndex = TeamComponent->CreateTeam(TEXT("Test Gathering Team"));
		
		// テスト用の採集場所設定
		TeamComponent->SetTeamGatheringLocation(TestTeamIndex, TEXT("forest_entrance"));
		TeamComponent->SetTeamTask(TestTeamIndex, ETaskType::Gathering);
		
		UE_LOG(LogTemp, Warning, TEXT("TestGatheringSetup: Created test team %d with gathering task"), TestTeamIndex);
	}
	
	// システム状態の確認
	UE_LOG(LogTemp, Warning, TEXT("=== GATHERING SYSTEM DEBUG ==="));
	UE_LOG(LogTemp, Warning, TEXT("Teams: %d"), TeamComponent->GetTeamCount());
	UE_LOG(LogTemp, Warning, TEXT("TimeManager Active: %s"), TimeManager->IsTimeSystemActive() ? TEXT("Yes") : TEXT("No"));
	
	for (int32 i = 0; i < TeamComponent->GetTeamCount(); i++)
	{
		FTeam Team = TeamComponent->GetTeam(i);
		UE_LOG(LogTemp, Warning, TEXT("Team %d: Task=%d, Location='%s', Members=%d"), 
			i, (int32)Team.AssignedTask, *Team.GatheringLocationId, Team.Members.Num());
	}
}

void AC_PlayerController::TestGridMapFunctionality()
{
	if (!GridMapComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("TestGridMapFunctionality: GridMapComponent is null"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== GRID MAP FUNCTIONALITY TEST ==="));
	
	// Test 1: Grid info
	UE_LOG(LogTemp, Warning, TEXT("Grid Size: %dx%d"), GridMapComponent->GridWidth, GridMapComponent->GridHeight);
	
	// Test 2: Test specific cell data
	FIntPoint BasePos(GridMapComponent->GridWidth / 2, GridMapComponent->GridHeight / 2);
	FGridCellData BaseCellData = GridMapComponent->GetCellData(BasePos);
	UE_LOG(LogTemp, Warning, TEXT("Base Cell (%d, %d): Type=%s, Walkable=%s"), 
		BasePos.X, BasePos.Y, 
		*BaseCellData.LocationType.ToString(),
		BaseCellData.bIsWalkable ? TEXT("Yes") : TEXT("No"));
	
	// Test 3: Find forest cells (safe tag request)
	FGameplayTag ForestTag = FGameplayTag::RequestGameplayTag(FName("Location.Forest"), false);
	if (ForestTag.IsValid())
	{
		TArray<FIntPoint> ForestCells = GridMapComponent->GetCellsWithTag(ForestTag);
		UE_LOG(LogTemp, Warning, TEXT("Forest cells found: %d"), ForestCells.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GameplayTags not loaded - skipping forest cell test"));
	}
	
	// Test 4: Find plains cells (safe tag request) 
	FGameplayTag PlainsTag = FGameplayTag::RequestGameplayTag(FName("Location.Plains"), false);
	if (PlainsTag.IsValid())
	{
		TArray<FIntPoint> PlainsCells = GridMapComponent->GetCellsWithTag(PlainsTag);
		UE_LOG(LogTemp, Warning, TEXT("Plains cells found: %d"), PlainsCells.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GameplayTags not loaded - skipping plains cell test"));
	}
	
	// Test 5: Test pathfinding (only if ForestTag is valid)
	if (ForestTag.IsValid())
	{
		TArray<FIntPoint> ForestCells = GridMapComponent->GetCellsWithTag(ForestTag);
		if (ForestCells.Num() > 0)
		{
			TArray<FIntPoint> Path = GridMapComponent->FindPath(BasePos, ForestCells[0]);
			UE_LOG(LogTemp, Warning, TEXT("Path from base to forest: %d steps"), Path.Num());
			if (Path.Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Path: Start (%d,%d) -> End (%d,%d)"), 
					Path[0].X, Path[0].Y, Path.Last().X, Path.Last().Y);
			}
		}
		
		// Test 6: Test nearest cell search
		FIntPoint NearestForest = GridMapComponent->FindNearestCellWithTag(BasePos, ForestTag);
		if (NearestForest.X >= 0 && NearestForest.Y >= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Nearest forest to base: (%d, %d)"), NearestForest.X, NearestForest.Y);
		}
	}
	else
	{
		// Test basic pathfinding without tags
		FIntPoint TestTarget(5, 5);
		TArray<FIntPoint> Path = GridMapComponent->FindPath(BasePos, TestTarget);
		UE_LOG(LogTemp, Warning, TEXT("Basic pathfinding test: %d steps from (%d,%d) to (%d,%d)"), 
			Path.Num(), BasePos.X, BasePos.Y, TestTarget.X, TestTarget.Y);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== GRID MAP TEST COMPLETED ==="));
}


