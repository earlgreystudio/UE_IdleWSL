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