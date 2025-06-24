#include "TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"

UTeamComponent::UTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTeamComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTeamComponent::AddCharacter(AC_IdleCharacter* IdleCharacter)
{
	if (IdleCharacter && !AllPlayerCharacters.Contains(IdleCharacter))
	{
		AllPlayerCharacters.Add(IdleCharacter);
	}
}

TArray<AC_IdleCharacter*> UTeamComponent::GetCharacterList() const
{
	return AllPlayerCharacters;
}

bool UTeamComponent::RemoveCharacter(AC_IdleCharacter* IdleCharacter)
{
	if (IdleCharacter)
	{
		return AllPlayerCharacters.Remove(IdleCharacter) > 0;
	}
	return false;
}

int32 UTeamComponent::GetCharacterCount() const
{
	return AllPlayerCharacters.Num();
}