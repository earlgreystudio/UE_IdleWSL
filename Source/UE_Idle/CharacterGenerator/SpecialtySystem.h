#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "../Types/CharacterTypes.h"
#include "SpecialtySystem.generated.h"

UCLASS()
class UE_IDLE_API USpecialtySystem : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Specialty System")
    static ESpecialtyType GetRandomSpecialty();
};