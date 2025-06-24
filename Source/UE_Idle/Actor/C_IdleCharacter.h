// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "C_IdleCharacter.generated.h"

class UCharacterStatusComponent;
class UCharacterInventoryComponent;

UCLASS()
class UE_IDLE_API AC_IdleCharacter : public AActor, public IIdleCharacterInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_IdleCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// IIdleCharacterInterface Implementation
	virtual FString GetCharacterName_Implementation() override;
	virtual bool IsActive_Implementation() override;
	virtual UCharacterStatusComponent* GetCharacterStatusComponent_Implementation() override;
	virtual UCharacterInventoryComponent* GetCharacterInventoryComponent_Implementation() override;
	virtual FCharacterTalent GetCharacterTalent_Implementation() override;

	// ステータス関連
	UFUNCTION(BlueprintCallable, Category = "Character")
	UCharacterStatusComponent* GetStatusComponent() const { return StatusComponent; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	UCharacterInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:
	// コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterStatusComponent> StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterInventoryComponent> InventoryComponent;

	// キャラクター基本データ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FString CharacterName = TEXT("Idle Character");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	bool bIsActive = true;

};
