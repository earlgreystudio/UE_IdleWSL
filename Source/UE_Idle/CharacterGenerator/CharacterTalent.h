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
    static FCharacterTalent ApplySpecialtyBonus(const FCharacterTalent& BaseTalent, ESpecialtyType SpecialtyType);

    // バランス調整用係数設定（エディタで調整可能）
    UFUNCTION(BlueprintCallable, Category = "Character Talent")
    static void SetBalanceMultipliers(float BaseStatMultiplier, float SpecialtyBonusMultiplier);

private:
    static FSpecialtyBonus GetSpecialtyBonus(ESpecialtyType SpecialtyType);
    
    // バランス調整係数（静的変数）
    static float BaseStatRangeMultiplier;   // 基本能力値範囲の係数（1.0=1-30, 0.4=1-12）
    static float SpecialtyBonusRangeMultiplier;  // 専門性ボーナスの係数（1.0=100%, 0.375=37.5%）
};