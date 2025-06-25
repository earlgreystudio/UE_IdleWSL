// Fill out your copyright notice in the Description page of Project Settings.


#include "C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"

// Sets default values
AC_IdleCharacter::AC_IdleCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// コンポーネント作成（防御的チェック付き）
	StatusComponent = CreateDefaultSubobject<UCharacterStatusComponent>(TEXT("StatusComponent"));
	if (!StatusComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create StatusComponent"));
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("StatusComponent created successfully"));
	}
	
	InventoryComponent = CreateDefaultSubobject<UCharacterInventoryComponent>(TEXT("InventoryComponent"));
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create InventoryComponent"));
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("InventoryComponent created successfully"));
	}
}

// Called when the game starts or when spawned
void AC_IdleCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AC_IdleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// IIdleCharacterInterface Implementation
FString AC_IdleCharacter::GetCharacterName_Implementation()
{
	return CharacterName;
}



bool AC_IdleCharacter::IsActive_Implementation()
{
	return bIsActive;
}

UCharacterStatusComponent* AC_IdleCharacter::GetStatusComponent() const
{
	if (!StatusComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("GetStatusComponent: StatusComponent is null for character %s"), *CharacterName);
	}
	else if (!IsValid(StatusComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("GetStatusComponent: StatusComponent is invalid for character %s"), *CharacterName);
	}
	return StatusComponent;
}

UCharacterStatusComponent* AC_IdleCharacter::GetCharacterStatusComponent_Implementation()
{
	return GetStatusComponent();  // 安全なGetStatusComponent()を使用
}

UCharacterInventoryComponent* AC_IdleCharacter::GetCharacterInventoryComponent_Implementation()
{
	return InventoryComponent;
}

FCharacterTalent AC_IdleCharacter::GetCharacterTalent_Implementation()
{
	if (StatusComponent)
	{
		return StatusComponent->GetTalent();
	}
	return FCharacterTalent();
}

