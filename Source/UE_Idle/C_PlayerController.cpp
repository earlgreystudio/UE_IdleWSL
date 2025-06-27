// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "Components/InventoryComponent.h"
#include "Components/TeamComponent.h"
#include "Components/EventLogManager.h"
#include "Components/TaskManagerComponent.h"
#include "Components/TimeManagerComponent.h"
#include "Components/CraftingComponent.h"
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
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));

	// Initialize UI variables
	InventoryListWidget = nullptr;
	TaskListWidget = nullptr;
	TaskMakeSheetWidget = nullptr;
}

void AC_PlayerController::BeginPlay()
{
	Super::BeginPlay();

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

	// Initialize Task Make Sheet UI
	if (TaskMakeSheetWidgetClass && TaskManager && CraftingComponent)
	{
		TaskMakeSheetWidget = CreateWidget<UC_TaskMakeSheet>(this, TaskMakeSheetWidgetClass);
		if (TaskMakeSheetWidget)
		{
			TaskMakeSheetWidget->SetTaskManagerComponent(TaskManager);
			TaskMakeSheetWidget->SetCraftingComponent(CraftingComponent);
			UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task Make Sheet UI initialized successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController: Failed to create Task Make Sheet UI widget"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: TaskMakeSheetWidgetClass not set or required components are null"));
	}
	
	// Initialize Task Management System
	InitializeTaskManagementSystem();
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
	if (!TaskMakeSheetWidget)
	{
		// Try to create widget if it doesn't exist
		if (TaskMakeSheetWidgetClass && TaskManager && CraftingComponent)
		{
			TaskMakeSheetWidget = CreateWidget<UC_TaskMakeSheet>(this, TaskMakeSheetWidgetClass);
			if (TaskMakeSheetWidget)
			{
				TaskMakeSheetWidget->SetTaskManagerComponent(TaskManager);
				TaskMakeSheetWidget->SetCraftingComponent(CraftingComponent);
			}
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
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AC_PlayerController::ShowTaskMakeSheetUI: TaskMakeSheetWidget is null"));
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

void AC_PlayerController::StartTaskManagementSystem()
{
	if (TimeManager)
	{
		TimeManager->StartTimeSystem();
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task Management System started"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController: TimeManager is null, cannot start system"));
	}
}

void AC_PlayerController::StopTaskManagementSystem()
{
	if (TimeManager)
	{
		TimeManager->StopTimeSystem();
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task Management System stopped"));
	}
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
	
	if (TimeManager && TaskManager && TeamComponent)
	{
		// TimeManager references
		TimeManager->RegisterTaskManager(TaskManager);
		TimeManager->RegisterTeamComponent(TeamComponent);
		
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: TimeManager references set"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AC_PlayerController: Missing components for TimeManager setup"));
	}
	
	// Bind combat end event
	if (TeamComponent && TimeManager)
	{
		TeamComponent->OnCombatEnded.AddDynamic(TimeManager, &UTimeManagerComponent::OnCombatEndedHandler);
		UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Combat end event bound"));
	}
	
	UE_LOG(LogTemp, Log, TEXT("AC_PlayerController: Task Management System initialized"));
}


