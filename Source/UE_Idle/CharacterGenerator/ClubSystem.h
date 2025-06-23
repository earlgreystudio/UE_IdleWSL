#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "../Types/CharacterTypes.h"
#include "ClubSystem.generated.h"

UCLASS()
class UE_IDLE_API UClubSystem : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Club System")
    static EClubType GetRandomClub();
};