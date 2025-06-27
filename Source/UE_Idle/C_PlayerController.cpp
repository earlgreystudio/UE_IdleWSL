// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "Components/InventoryComponent.h"
#include "Components/TeamComponent.h"
#include "Components/EventLogManager.h"
#include "Actor/C_IdleCharacter.h"
#include "UI/C__InventoryList.h"
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

	// Initialize UI variables
	InventoryListWidget = nullptr;
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


