// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/PlayerControllerInterface.h"
#include "C_PlayerController.generated.h"

class UGlobalInventoryComponent;
class UTeamComponent;
class UEventLogManager;
class AC_IdleCharacter;

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
	TObjectPtr<UGlobalInventoryComponent> GlobalInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTeamComponent> TeamComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UEventLogManager> EventLogManager;

	// IPlayerControllerInterface Implementation
	virtual void AddItemToStorage_Implementation(const FString& ItemId, int32 Quantity) override;
	virtual void AddCharacter_Implementation(AActor* NewCharacter) override;
	virtual UGlobalInventoryComponent* GetGlobalInventoryComp_Implementation() override;
	virtual TArray<AActor*> GetCharacterList_Implementation() override;
	virtual UTeamComponent* GetTeamComponent_Implementation() override;


};
