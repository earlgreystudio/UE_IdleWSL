// Fill out your copyright notice in the Description page of Project Settings.


#include "C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"

// Sets default values
AC_IdleCharacter::AC_IdleCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// コンポーネント作成
	StatusComponent = CreateDefaultSubobject<UCharacterStatusComponent>(TEXT("StatusComponent"));
	InventoryComponent = CreateDefaultSubobject<UCharacterInventoryComponent>(TEXT("InventoryComponent"));
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

UCharacterStatusComponent* AC_IdleCharacter::GetCharacterStatusComponent_Implementation()
{
	return StatusComponent;
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

