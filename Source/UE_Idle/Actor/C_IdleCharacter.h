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

	// ステータス関連（安全性チェック付き）
	UFUNCTION(BlueprintCallable, Category = "Character")
	UCharacterStatusComponent* GetStatusComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	UCharacterInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// キャラクター名設定
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetCharacterName(const FString& NewName) { CharacterName = NewName; }

	// キャラクター種族取得・設定
	UFUNCTION(BlueprintCallable, Category = "Character")
	FString GetCharacterRace() const { return CharacterRace; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetCharacterRace(const FString& NewRace) { CharacterRace = NewRace; }

protected:
	// コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterStatusComponent> StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterInventoryComponent> InventoryComponent;

	// キャラクター基本データ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FString CharacterName = TEXT("Idle Character");

	// キャラクターの種族（CharacterPresets.csvのRowNameに対応）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FString CharacterRace = TEXT("human");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	bool bIsActive = true;

};
