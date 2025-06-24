#include "CharacterStatusComponent.h"

UCharacterStatusComponent::UCharacterStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// デフォルトステータス設定
	Status = FCharacterStatus();
	ClubType = EClubType::Baseball; // デフォルト部活動
	Talent = FCharacterTalent();    // デフォルト才能
}

void UCharacterStatusComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCharacterStatusComponent::SetStatus(const FCharacterStatus& NewStatus)
{
	Status = NewStatus;
	
	// イベント通知
	OnStatusChanged.Broadcast(NewStatus);
	OnCharacterDataUpdated.Broadcast();
}

void UCharacterStatusComponent::SetClubType(EClubType NewClubType)
{
	ClubType = NewClubType;
	
	// イベント通知
	OnClubTypeChanged.Broadcast(NewClubType);
	OnCharacterDataUpdated.Broadcast();
}

void UCharacterStatusComponent::SetTalent(const FCharacterTalent& NewTalent)
{
	Talent = NewTalent;
	
	// イベント通知
	OnTalentChanged.Broadcast(NewTalent);
	OnCharacterDataUpdated.Broadcast();
}

void UCharacterStatusComponent::SetCurrentHealth(float NewHealth)
{
	// 範囲制限
	NewHealth = FMath::Clamp(NewHealth, 0.0f, Status.MaxHealth);
	
	if (Status.CurrentHealth != NewHealth)
	{
		Status.CurrentHealth = NewHealth;
		
		// イベント通知
		OnHealthChanged.Broadcast(NewHealth);
		OnStatusChanged.Broadcast(Status);
		OnCharacterDataUpdated.Broadcast();
	}
}