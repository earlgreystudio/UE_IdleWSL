// Fill out your copyright notice in the Description page of Project Settings.


#include "C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/InventoryComponent.h"

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
	
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	if (InventoryComponent)
	{
		// Set owner ID for character inventory
		InventoryComponent->OwnerId = TEXT("Character");
		UE_LOG(LogTemp, VeryVerbose, TEXT("InventoryComponent created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create InventoryComponent"));
	}
}

// Called when the game starts or when spawned
void AC_IdleCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// デバッグログ追加
	UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter::BeginPlay - %s, InventoryComponent: %s"), 
		*GetName(), InventoryComponent ? TEXT("Valid") : TEXT("NULL"));
	
	// インベントリの装備変更イベントをステータスコンポーネントの再計算に接続
	if (InventoryComponent && StatusComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter: Binding equipment change events"));
		
		// Dynamic multicast delegatesはAddDynamicを使用
		InventoryComponent->OnItemEquipped.AddDynamic(this, &AC_IdleCharacter::HandleItemEquipped);
		InventoryComponent->OnItemUnequipped.AddDynamic(this, &AC_IdleCharacter::HandleItemUnequipped);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("C_IdleCharacter: Failed to bind equipment events - missing components"));
	}
}

// Called every frame
void AC_IdleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Equipment change handlers
void AC_IdleCharacter::HandleItemEquipped(const FString& ItemId, EEquipmentSlot Slot)
{
	UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter: Item equipped - %s in slot %d"), *ItemId, (int32)Slot);
	if (StatusComponent)
	{
		StatusComponent->OnEquipmentChanged();
	}
}

void AC_IdleCharacter::HandleItemUnequipped(const FString& ItemId, EEquipmentSlot Slot)
{
	UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter: Item unequipped - %s from slot %d"), *ItemId, (int32)Slot);
	if (StatusComponent)
	{
		StatusComponent->OnEquipmentChanged();
	}
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
		UE_LOG(LogTemp, Error, TEXT("GetStatusComponent: StatusComponent is null for character %s"), 
			   CharacterName.IsEmpty() ? TEXT("Unknown") : *CharacterName);
		return nullptr;
	}
	else if (!IsValid(StatusComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("GetStatusComponent: StatusComponent is invalid for character %s"), 
			   CharacterName.IsEmpty() ? TEXT("Unknown") : *CharacterName);
		return nullptr;
	}
	return StatusComponent;
}

UCharacterStatusComponent* AC_IdleCharacter::GetCharacterStatusComponent_Implementation()
{
	return GetStatusComponent();  // 安全なGetStatusComponent()を使用
}

UInventoryComponent* AC_IdleCharacter::GetInventoryComponent() const
{
	// 詳細なデバッグログ
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("AC_IdleCharacter::GetInventoryComponent - InventoryComponent is NULL for %s"), *GetName());
		
		// const_castを使って、FindComponentByClassを試す（デバッグ用）
		AC_IdleCharacter* MutableThis = const_cast<AC_IdleCharacter*>(this);
		UInventoryComponent* FoundComp = MutableThis->FindComponentByClass<UInventoryComponent>();
		if (FoundComp)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("AC_IdleCharacter::GetInventoryComponent - Found via FindComponentByClass"));
			return FoundComp;
		}
	}
	
	return InventoryComponent;
}

UInventoryComponent* AC_IdleCharacter::GetCharacterInventoryComponent_Implementation()
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

FDerivedStats AC_IdleCharacter::GetDerivedStats() const
{
	if (StatusComponent)
	{
		return StatusComponent->GetDerivedStats();
	}
	return FDerivedStats();
}

