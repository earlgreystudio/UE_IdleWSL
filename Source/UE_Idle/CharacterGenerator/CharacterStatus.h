#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "../Types/CharacterTypes.h"
#include "CharacterStatus.generated.h"

UCLASS()
class UE_IDLE_API UCharacterStatusManager : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Status")
    static FCharacterStatus CalculateMaxStatus(const FCharacterTalent& Talent);
};