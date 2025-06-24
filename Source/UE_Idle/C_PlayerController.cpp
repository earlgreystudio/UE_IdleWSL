// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "Components/GlobalInventoryComponent.h"
#include "Components/TeamComponent.h"
#include "Actor/C_IdleCharacter.h"

AC_PlayerController::AC_PlayerController()
{
	// Create components
	GlobalInventory = CreateDefaultSubobject<UGlobalInventoryComponent>(TEXT("GlobalInventory"));
	TeamComponent = CreateDefaultSubobject<UTeamComponent>(TEXT("TeamComponent"));
}

void AC_PlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AC_PlayerController::AddItemToStorage_Implementation(const FString& ItemId, int32 Quantity)
{
	if (GlobalInventory)
	{
		GlobalInventory->AddItemToStorage(ItemId, Quantity);
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

UGlobalInventoryComponent* AC_PlayerController::GetGlobalInventoryComp_Implementation()
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


