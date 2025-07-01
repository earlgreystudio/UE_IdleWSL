// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/PlayerControllerInterface.h"
#include "Blueprint/UserWidget.h"
#include "C_PlayerController.generated.h"

class UInventoryComponent;
class UTeamComponent;
class UEventLogManager;
class UTaskManagerComponent;
class UTimeManagerComponent;
class UCraftingComponent;
class UBaseComponent;
class ULocationMovementComponent;
class UCombatComponent;
class UGridMapComponent;
class AC_IdleCharacter;
class UC__InventoryList;
class UC_TaskList;
class UC_TaskMakeSheet;

/**
 * 
 */
UCLASS()
class UE_IDLE_API AC_PlayerController : public APlayerController, public IPlayerControllerInterface
{
	GENERATED_BODY()

public:
	AC_PlayerController();

protected:
	virtual void BeginPlay() override;

public:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> GlobalInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTeamComponent> TeamComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UEventLogManager> EventLogManager;

	// Task Management System Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Task Management")
	TObjectPtr<UTaskManagerComponent> TaskManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Task Management")
	TObjectPtr<UTimeManagerComponent> TimeManager;

	// GatheringComponent削除済み - TaskManagerに統合

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Task Management")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Task Management")
	TObjectPtr<ULocationMovementComponent> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Task Management")
	TObjectPtr<UCombatComponent> CombatComponent;

	// Base Management System
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Base Management")
	TObjectPtr<UBaseComponent> BaseComponent;

	// Grid Map System
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid System")
	TObjectPtr<UGridMapComponent> GridMapComponent;

	// Map Generator
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid System")
	TObjectPtr<class UMapGeneratorComponent> MapGeneratorComponent;

	// IPlayerControllerInterface Implementation
	virtual void AddItemToStorage_Implementation(const FString& ItemId, int32 Quantity) override;
	virtual void AddCharacter_Implementation(AActor* NewCharacter) override;
	virtual UInventoryComponent* GetGlobalInventoryComp_Implementation() override;
	virtual TArray<AActor*> GetCharacterList_Implementation() override;
	virtual UTeamComponent* GetTeamComponent_Implementation() override;

	// Task Management Interface
	UFUNCTION(BlueprintCallable, Category = "Task Management")
	UTaskManagerComponent* GetTaskManager() const { return TaskManager; }

	UFUNCTION(BlueprintCallable, Category = "Task Management")
	UTimeManagerComponent* GetTimeManager() const { return TimeManager; }

	UFUNCTION(BlueprintCallable, Category = "Base Management")
	UBaseComponent* GetBaseComponent() const { return BaseComponent; }

	// System Control - 削除（StartTaskSystemCpp/StopTaskSystemCppに統合）

	// UI Management
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowInventoryUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideInventoryUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	UC__InventoryList* GetInventoryUI();

	// Task Management UI
	UFUNCTION(BlueprintCallable, Category = "Task UI")
	void ShowTaskListUI();

	UFUNCTION(BlueprintCallable, Category = "Task UI")
	void HideTaskListUI();

	UFUNCTION(BlueprintCallable, Category = "Task UI")
	UC_TaskList* GetTaskListUI();

	// Task Make Sheet UI
	UFUNCTION(BlueprintCallable, Category = "Task UI")
	void ShowTaskMakeSheetUI();

	UFUNCTION(BlueprintCallable, Category = "Task UI")
	void ShowTaskMakeSheetUIAtPosition(FVector2D Position);

	UFUNCTION(BlueprintCallable, Category = "Task UI")
	void HideTaskMakeSheetUI();

	UFUNCTION(BlueprintCallable, Category = "Task UI")
	UC_TaskMakeSheet* GetTaskMakeSheetUI();

protected:
	// Internal initialization
	void InitializeTaskManagementSystem();

	// Add default tasks for testing
	void AddDefaultTasks();

public:
	// Task Management System - C++ Only (Blueprint側は別途存在)
	UFUNCTION(BlueprintCallable, Category = "Task Management Debug")
	void StartTaskSystemCpp();

	UFUNCTION(BlueprintCallable, Category = "Task Management Debug")
	void StopTaskSystemCpp();

	// Debug function for gathering system
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void TestGatheringSetup();

	// Debug function for grid map system
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void TestGridMapFunctionality();

	// C++専用初期化（Blueprint機能とは独立）
	UFUNCTION(BlueprintCallable, Category = "Task Management Debug")
	void InitializeGatheringSystemCpp();

protected:
	// UI Widget Classes (set in Blueprint)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
	TSubclassOf<UC__InventoryList> InventoryListWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
	TSubclassOf<UC_TaskList> TaskListWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
	TSubclassOf<UC_TaskMakeSheet> TaskMakeSheetWidgetClass;

	// UI Widget Instances
	UPROPERTY()
	UC__InventoryList* InventoryListWidget;

	UPROPERTY()
	UC_TaskList* TaskListWidget;

	UPROPERTY()
	UC_TaskMakeSheet* TaskMakeSheetWidget;


};
