#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "../Types/CharacterTypes.h"
#include "CharacterTalent.generated.h"

UCLASS()
class UE_IDLE_API UCharacterTalentGenerator : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Talent")
    static FCharacterTalent GenerateRandomTalent();

    UFUNCTION(BlueprintCallable, Category = "Character Talent")
    static FCharacterTalent ApplyClubBonus(const FCharacterTalent& BaseTalent, EClubType ClubType);

private:
    static FClubBonus GetClubBonus(EClubType ClubType);
};