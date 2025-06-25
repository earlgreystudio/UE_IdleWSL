#include "CombatCalculator.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"
#include "ItemDataTableManager.h"
#include "Engine/World.h"

FEquipmentPenalty UCombatCalculator::CalculateEquipmentPenalty(AC_IdleCharacter* Character)
{
    FEquipmentPenalty Penalty;
    
    if (!Character)
    {
        return Penalty;
    }

    // 積載量取得
    if (UCharacterStatusComponent* StatusComp = Character->GetStatusComponent())
    {
        FCharacterStatus Status = StatusComp->GetStatus();
        Penalty.CarryingCapacity = Status.CarryingCapacity;
    }

    // 装備総重量計算
    Penalty.TotalWeight = GetTotalEquipmentWeight(Character);
    
    // 装備重量率計算
    if (Penalty.CarryingCapacity > 0.0f)
    {
        Penalty.WeightRatio = Penalty.TotalWeight / Penalty.CarryingCapacity;
    }

    // ペナルティ率計算
    Penalty.PenaltyPercentage = CalculatePenaltyPercentage(Penalty.WeightRatio);

    return Penalty;
}

float UCombatCalculator::CalculateAttackSpeed(AC_IdleCharacter* Character, const FString& WeaponItemId)
{
    if (!Character)
    {
        return 1.0f;
    }

    FCharacterTalent Talent = GetCharacterTalent(Character);
    ESkillType WeaponSkill = GetWeaponSkillType(WeaponItemId);
    float SkillLevel = GetSkillLevel(Character, WeaponSkill);
    
    if (IsRangedWeapon(WeaponItemId))
    {
        // 遠距離武器の攻撃速度: 1.0 + (器用 × 0.01) + (スキルレベル × 0.03)
        return 1.0f + (Talent.Dexterity * 0.01f) + (SkillLevel * 0.03f);
    }
    else
    {
        // 近接武器の攻撃速度
        float BaseSpeed = 2.0f + (Talent.Agility * 0.02f) + (SkillLevel * 0.01f);
        float WeaponWeight = GetWeaponWeight(WeaponItemId);
        float WeightPenalty = WeaponWeight / (Talent.Strength * 0.3f);
        
        return FMath::Max(0.1f, BaseSpeed - WeightPenalty);
    }
}

float UCombatCalculator::CalculateHitChance(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    if (!Attacker)
    {
        return 50.0f;
    }

    FCharacterTalent Talent = GetCharacterTalent(Attacker);
    ESkillType WeaponSkill = GetWeaponSkillType(WeaponItemId);
    float SkillLevel = GetSkillLevel(Attacker, WeaponSkill);
    float WeaponWeight = GetWeaponWeight(WeaponItemId);
    
    // 基本命中率 = 50 + (スキルレベル × 2) + (器用 × 1.5) - (武器重量 ÷ (力 × 0.5))
    float BaseHitChance = 50.0f + (SkillLevel * 2.0f) + (Talent.Dexterity * 1.5f);
    float WeightPenalty = WeaponWeight / (Talent.Strength * 0.5f);
    
    return FMath::Max(5.0f, BaseHitChance - WeightPenalty);
}

float UCombatCalculator::CalculateDodgeChance(AC_IdleCharacter* Defender)
{
    if (!Defender)
    {
        return 10.0f;
    }

    FCharacterTalent Talent = GetCharacterTalent(Defender);
    float EvasionSkill = GetSkillLevel(Defender, ESkillType::Evasion);
    FEquipmentPenalty Penalty = CalculateEquipmentPenalty(Defender);
    
    // 基本回避率 = 10 + (敏捷 × 2) + (回避スキル × 3)
    float BaseDodgeChance = 10.0f + (Talent.Agility * 2.0f) + (EvasionSkill * 3.0f);
    
    // 装備ペナルティ適用
    float PenaltyReduction = BaseDodgeChance * (Penalty.PenaltyPercentage / 100.0f);
    
    return FMath::Max(0.0f, BaseDodgeChance - PenaltyReduction);
}

float UCombatCalculator::CalculateParryChance(AC_IdleCharacter* Defender)
{
    if (!Defender)
    {
        return 5.0f;
    }

    FCharacterTalent Talent = GetCharacterTalent(Defender);
    float ParrySkill = GetSkillLevel(Defender, ESkillType::Parry);
    FEquipmentPenalty Penalty = CalculateEquipmentPenalty(Defender);
    
    // 基本受け流し率 = 5 + (器用 × 1.5) + (受け流しスキル × 3)
    float BaseParryChance = 5.0f + (Talent.Dexterity * 1.5f) + (ParrySkill * 3.0f);
    
    // 装備ペナルティ適用
    float PenaltyReduction = BaseParryChance * (Penalty.PenaltyPercentage / 100.0f);
    
    return FMath::Max(0.0f, BaseParryChance - PenaltyReduction);
}

float UCombatCalculator::CalculateCriticalChance(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    if (!Attacker)
    {
        return 5.0f;
    }

    FCharacterTalent Talent = GetCharacterTalent(Attacker);
    ESkillType WeaponSkill = GetWeaponSkillType(WeaponItemId);
    float SkillLevel = GetSkillLevel(Attacker, WeaponSkill);
    
    // クリティカル率 = 5 + (器用 × 0.5) + (スキルレベル × 0.3)
    return 5.0f + (Talent.Dexterity * 0.5f) + (SkillLevel * 0.3f);
}

int32 UCombatCalculator::CalculateBaseDamage(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    if (!Attacker)
    {
        return 1;
    }

    FCharacterTalent Talent = GetCharacterTalent(Attacker);
    ESkillType WeaponSkill = GetWeaponSkillType(WeaponItemId);
    float SkillLevel = GetSkillLevel(Attacker, WeaponSkill);
    int32 WeaponAttackPower = GetWeaponAttackPower(WeaponItemId);
    
    // 武器ダメージ = 武器攻撃力 × (1 + (スキルレベル ÷ 20))
    float WeaponDamage = WeaponAttackPower * (1.0f + (SkillLevel / 20.0f));
    
    // 能力補正 = 力 × 0.5 (近接武器) または 器用 × 0.5 (遠距離武器)
    float AbilityModifier;
    if (IsRangedWeapon(WeaponItemId))
    {
        AbilityModifier = Talent.Dexterity * 0.5f;
    }
    else
    {
        AbilityModifier = Talent.Strength * 0.5f;
    }
    
    // 基本ダメージ = 武器ダメージ + 能力補正
    return FMath::Max(1, FMath::RoundToInt(WeaponDamage + AbilityModifier));
}

int32 UCombatCalculator::CalculateDefenseValue(AC_IdleCharacter* Defender)
{
    if (!Defender)
    {
        return 0;
    }

    FCharacterTalent Talent = GetCharacterTalent(Defender);
    float ArmorDefense = GetArmorDefense(Defender);
    
    // 防御値 = 防具防御力 + (頑丈 × 0.3)
    return FMath::Max(0, FMath::RoundToInt(ArmorDefense + (Talent.Toughness * 0.3f)));
}

int32 UCombatCalculator::CalculateFinalDamage(int32 BaseDamage, int32 DefenseValue, bool bParried, bool bCritical)
{
    float FinalDamage = BaseDamage;
    
    // クリティカルヒット処理
    if (bCritical)
    {
        FinalDamage *= 2.0f;
    }
    
    // 受け流し処理（80%カット）
    if (bParried)
    {
        FinalDamage *= 0.2f;
    }
    
    // 防御計算: 最終ダメージ = 基本ダメージ × (100 ÷ (100 + 防御値))
    FinalDamage = FinalDamage * (100.0f / (100.0f + DefenseValue));
    
    return FMath::Max(1, FMath::RoundToInt(FinalDamage));
}

FCombatCalculationResult UCombatCalculator::PerformCombatCalculation(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, const FString& WeaponItemId)
{
    FCombatCalculationResult Result;
    
    if (!Attacker || !Defender)
    {
        return Result;
    }

    // 各種確率計算
    Result.HitChance = CalculateHitChance(Attacker, WeaponItemId);
    Result.DodgeChance = CalculateDodgeChance(Defender);
    Result.ParryChance = CalculateParryChance(Defender);
    Result.CriticalChance = CalculateCriticalChance(Attacker, WeaponItemId);
    
    // 判定順序：回避 → 受け流し → 通常攻撃
    float RandomValue = FMath::FRand() * 100.0f;
    
    // 1. 回避判定
    if (RandomValue < Result.DodgeChance)
    {
        Result.bDodged = true;
        Result.bHit = false;
        Result.FinalDamage = 0;
        return Result;
    }
    
    // 2. 命中判定
    RandomValue = FMath::FRand() * 100.0f;
    if (RandomValue >= Result.HitChance)
    {
        Result.bHit = false;
        Result.FinalDamage = 0;
        return Result;
    }
    
    Result.bHit = true;
    
    // 3. 受け流し判定
    RandomValue = FMath::FRand() * 100.0f;
    if (RandomValue < Result.ParryChance)
    {
        Result.bParried = true;
    }
    
    // 4. クリティカル判定
    RandomValue = FMath::FRand() * 100.0f;
    if (RandomValue < Result.CriticalChance)
    {
        Result.bCritical = true;
    }
    
    // 5. ダメージ計算
    Result.BaseDamage = CalculateBaseDamage(Attacker, WeaponItemId);
    int32 DefenseValue = CalculateDefenseValue(Defender);
    Result.FinalDamage = CalculateFinalDamage(Result.BaseDamage, DefenseValue, Result.bParried, Result.bCritical);
    
    return Result;
}

// ヘルパー関数実装

float UCombatCalculator::GetSkillLevel(AC_IdleCharacter* Character, ESkillType SkillType)
{
    if (!Character)
    {
        return 1.0f;
    }

    FCharacterTalent Talent = GetCharacterTalent(Character);
    
    for (const FSkillTalent& Skill : Talent.Skills)
    {
        if (Skill.SkillType == SkillType)
        {
            return Skill.Value;
        }
    }
    
    return 1.0f; // デフォルトスキルレベル
}

FCharacterTalent UCombatCalculator::GetCharacterTalent(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return FCharacterTalent();
    }

    if (UCharacterStatusComponent* StatusComp = Character->GetStatusComponent())
    {
        return StatusComp->GetTalent();
    }
    
    return FCharacterTalent();
}

float UCombatCalculator::GetWeaponWeight(const FString& WeaponItemId)
{
    // ItemDataTableManagerから武器の重量を取得
    UGameInstance* GameInstance = nullptr;
    UWorld* World = GEngine->GetCurrentPlayWorld();
    if (World)
    {
        GameInstance = World->GetGameInstance();
    }
    
    if (GameInstance)
    {
        UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        if (ItemManager)
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(WeaponItemId, ItemData))
            {
                return ItemData.Weight;
            }
        }
    }
    
    return 2.0f; // デフォルト重量
}

int32 UCombatCalculator::GetWeaponAttackPower(const FString& WeaponItemId)
{
    // ItemDataTableManagerから武器の攻撃力を取得
    UGameInstance* GameInstance = nullptr;
    UWorld* World = GEngine->GetCurrentPlayWorld();
    if (World)
    {
        GameInstance = World->GetGameInstance();
    }
    
    if (GameInstance)
    {
        UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        if (ItemManager)
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(WeaponItemId, ItemData))
            {
                return ItemData.AttackPower;
            }
        }
    }
    
    return 10; // デフォルト攻撃力
}

bool UCombatCalculator::IsRangedWeapon(const FString& WeaponItemId)
{
    // 武器種類の判定（アイテムデータから判断）
    UGameInstance* GameInstance = nullptr;
    UWorld* World = GEngine->GetCurrentPlayWorld();
    if (World)
    {
        GameInstance = World->GetGameInstance();
    }
    
    if (GameInstance)
    {
        UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        if (ItemManager)
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(WeaponItemId, ItemData))
            {
                // 弓、投擲、射撃武器かどうかをチェック
                return WeaponItemId.Contains(TEXT("bow")) || 
                       WeaponItemId.Contains(TEXT("gun")) || 
                       WeaponItemId.Contains(TEXT("throwing"));
            }
        }
    }
    
    return false;
}

ESkillType UCombatCalculator::GetWeaponSkillType(const FString& WeaponItemId)
{
    // 武器名から対応するスキルタイプを判定
    if (WeaponItemId.Contains(TEXT("sword")) || WeaponItemId.Contains(TEXT("axe")) || WeaponItemId.Contains(TEXT("mace")))
    {
        return ESkillType::OneHandedWeapons;
    }
    else if (WeaponItemId.Contains(TEXT("two_hand")) || WeaponItemId.Contains(TEXT("great")))
    {
        return ESkillType::TwoHandedWeapons;
    }
    else if (WeaponItemId.Contains(TEXT("spear")) || WeaponItemId.Contains(TEXT("halberd")))
    {
        return ESkillType::PolearmWeapons;
    }
    else if (WeaponItemId.Contains(TEXT("bow")))
    {
        return ESkillType::Archery;
    }
    else if (WeaponItemId.Contains(TEXT("gun")))
    {
        return ESkillType::Firearms;
    }
    else if (WeaponItemId.Contains(TEXT("throwing")))
    {
        return ESkillType::Throwing;
    }
    
    // デフォルトは格闘
    return ESkillType::Combat;
}

float UCombatCalculator::GetTotalEquipmentWeight(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return 0.0f;
    }

    float TotalWeight = 0.0f;
    
    if (UCharacterInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        // 装備中のアイテムの重量を合計
        // 現在の実装では装備システムが未完成のため、インベントリ内の武器・防具の重量を概算
        TArray<FInventorySlot> AllSlots = InventoryComp->GetAllInventorySlots();
        
        UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
        if (GameInstance)
        {
            UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
            if (ItemManager)
            {
                for (const FInventorySlot& Slot : AllSlots)
                {
                    FItemDataRow ItemData;
                    if (ItemManager->GetItemData(Slot.ItemId, ItemData))
                    {
                        if (ItemData.ItemType == EItemTypeTable::Weapon || ItemData.ItemType == EItemTypeTable::Armor)
                        {
                            TotalWeight += ItemData.Weight * Slot.Quantity;
                        }
                    }
                }
            }
        }
    }
    
    return TotalWeight;
}

float UCombatCalculator::GetArmorDefense(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return 0.0f;
    }

    float TotalDefense = 0.0f;
    
    if (UCharacterInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        // 装備中の防具の防御力を合計
        TArray<FInventorySlot> AllSlots = InventoryComp->GetAllInventorySlots();
        
        UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
        if (GameInstance)
        {
            UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
            if (ItemManager)
            {
                for (const FInventorySlot& Slot : AllSlots)
                {
                    FItemDataRow ItemData;
                    if (ItemManager->GetItemData(Slot.ItemId, ItemData))
                    {
                        if (ItemData.ItemType == EItemTypeTable::Armor)
                        {
                            TotalDefense += ItemData.Defense;
                        }
                    }
                }
            }
        }
    }
    
    return TotalDefense;
}

float UCombatCalculator::CalculatePenaltyPercentage(float WeightRatio)
{
    // CombatCalculation.mdの計算式を実装
    if (WeightRatio >= 0.7f)
    {
        return 90.0f;
    }
    else if (WeightRatio >= 0.5f)
    {
        return 60.0f + (WeightRatio - 0.5f) * 150.0f;
    }
    else if (WeightRatio >= 0.2f)
    {
        return 20.0f + (WeightRatio - 0.2f) * 133.0f;
    }
    else if (WeightRatio >= 0.1f)
    {
        return 5.0f + (WeightRatio - 0.1f) * 150.0f;
    }
    else
    {
        return WeightRatio * 50.0f;
    }
}