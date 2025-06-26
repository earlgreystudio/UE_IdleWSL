#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../Types/CombatTypes.h"
#include "../Types/CharacterTypes.h"
#include "CombatCalculator.generated.h"

class AC_IdleCharacter;
class UCharacterStatusComponent;
class UCharacterInventoryComponent;

UCLASS(BlueprintType)
class UE_IDLE_API UCombatCalculator : public UObject
{
    GENERATED_BODY()

public:
    // 装備重量ペナルティ計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static FEquipmentPenalty CalculateEquipmentPenalty(AC_IdleCharacter* Character);

    // 攻撃速度計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static float CalculateAttackSpeed(AC_IdleCharacter* Character, const FString& WeaponItemId);

    // 命中率計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static float CalculateHitChance(AC_IdleCharacter* Attacker, const FString& WeaponItemId);

    // 回避率計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static float CalculateDodgeChance(AC_IdleCharacter* Defender);

    // 受け流し率計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static float CalculateParryChance(AC_IdleCharacter* Defender);

    // 盾防御率計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static float CalculateShieldChance(AC_IdleCharacter* Defender);

    // クリティカル率計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static float CalculateCriticalChance(AC_IdleCharacter* Attacker, const FString& WeaponItemId);

    // 基本ダメージ計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static int32 CalculateBaseDamage(AC_IdleCharacter* Attacker, const FString& WeaponItemId);

    // 防御値計算
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static int32 CalculateDefenseValue(AC_IdleCharacter* Defender);

    // 最終ダメージ計算（防御適用後）
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static int32 CalculateFinalDamage(int32 BaseDamage, int32 DefenseValue, bool bParried, bool bShieldBlocked, bool bCritical, int32 ShieldDefense = 0, float ShieldSkill = 0.0f);

    // 総合戦闘計算（ワンショット）
    UFUNCTION(BlueprintCallable, Category = "Combat Calculator")
    static FCombatCalculationResult PerformCombatCalculation(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, const FString& WeaponItemId);

private:
    // ヘルパー関数
    static float GetSkillLevel(AC_IdleCharacter* Character, ESkillType SkillType);
    static FCharacterTalent GetCharacterTalent(AC_IdleCharacter* Character);
    static float GetWeaponWeight(const FString& WeaponItemId);
    static int32 GetWeaponAttackPower(const FString& WeaponItemId);
    static bool IsRangedWeapon(const FString& WeaponItemId);
    static ESkillType GetWeaponSkillType(const FString& WeaponItemId);
    static float GetTotalEquipmentWeight(AC_IdleCharacter* Character);
    static float GetArmorDefense(AC_IdleCharacter* Character);
    
    // 装備重量ペナルティの詳細計算
    static float CalculatePenaltyPercentage(float WeightRatio);

    // 爪牙システム対応関数
    static FString GetCharacterRace(AC_IdleCharacter* Character);
    static FString GetEffectiveWeaponId(AC_IdleCharacter* Character);
    static bool IsNaturalWeapon(const FString& WeaponId);
    static int32 CalculateNaturalWeaponDamage(AC_IdleCharacter* Attacker, const FString& NaturalWeaponId);
    static int32 CalculateArtificialWeaponDamage(AC_IdleCharacter* Attacker, const FString& WeaponItemId);
    static int32 GetNaturalWeaponPower(const FString& CharacterRace);
    
    // 盾関連ヘルパー関数
    static int32 GetShieldDefense(AC_IdleCharacter* Character);
    static float CalculateShieldDamageReduction(int32 ShieldDefense, float ShieldSkill);
};