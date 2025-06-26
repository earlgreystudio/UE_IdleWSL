#include "CombatCalculator.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "ItemDataTableManager.h"
#include "CharacterPresetManager.h"
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
    // 事前計算済みの値を取得
    if (!Character || !IsValid(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("CalculateAttackSpeed: Invalid character"));
        return 1.0f;
    }

    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (!StatusComp)
    {
        FString CharName = TEXT("Unknown");
        if (Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
        {
            CharName = IIdleCharacterInterface::Execute_GetCharacterName(Character);
        }
        UE_LOG(LogTemp, Warning, TEXT("CalculateAttackSpeed: No status component for character %s"), *CharName);
        return 1.0f;
    }
    
    if (!IsValid(StatusComp))
    {
        FString CharName = TEXT("Unknown");
        if (Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
        {
            CharName = IIdleCharacterInterface::Execute_GetCharacterName(Character);
        }
        UE_LOG(LogTemp, Warning, TEXT("CalculateAttackSpeed: Invalid status component for character %s"), *CharName);
        return 1.0f;
    }

    // 事前計算済みの攻撃速度を直接返す
    return StatusComp->GetAttackSpeed();
}

float UCombatCalculator::CalculateHitChance(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    if (!Attacker || !IsValid(Attacker))
    {
        return 50.0f;
    }

    UCharacterStatusComponent* StatusComp = Attacker->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 50.0f;
    }

    // 事前計算済みの命中率を直接返す
    return StatusComp->GetHitChance();
}

float UCombatCalculator::CalculateDodgeChance(AC_IdleCharacter* Defender)
{
    if (!Defender || !IsValid(Defender))
    {
        return 10.0f;
    }

    UCharacterStatusComponent* StatusComp = Defender->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 10.0f;
    }

    // 事前計算済みの回避率を直接返す
    return StatusComp->GetDodgeChance();
}

float UCombatCalculator::CalculateParryChance(AC_IdleCharacter* Defender)
{
    if (!Defender || !IsValid(Defender))
    {
        return 5.0f;
    }

    UCharacterStatusComponent* StatusComp = Defender->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 5.0f;
    }

    // 事前計算済みの受け流し率を直接返す
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.ParryChance;
}

float UCombatCalculator::CalculateShieldChance(AC_IdleCharacter* Defender)
{
    if (!Defender || !IsValid(Defender))
    {
        return 3.0f;
    }

    UCharacterStatusComponent* StatusComp = Defender->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 3.0f;
    }

    // 事前計算済みの盾防御率を直接返す
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.ShieldChance;
}

float UCombatCalculator::CalculateCriticalChance(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    if (!Attacker || !IsValid(Attacker))
    {
        return 5.0f;
    }

    UCharacterStatusComponent* StatusComp = Attacker->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 5.0f;
    }

    // 事前計算済みのクリティカル率を直接返す
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.CriticalChance;
}

int32 UCombatCalculator::CalculateBaseDamage(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    if (!Attacker || !IsValid(Attacker))
    {
        return 1;
    }

    UCharacterStatusComponent* StatusComp = Attacker->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 1;
    }

    // 事前計算済みの基本ダメージを直接返す
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.BaseDamage;
}

int32 UCombatCalculator::CalculateDefenseValue(AC_IdleCharacter* Defender)
{
    if (!Defender || !IsValid(Defender))
    {
        return 0;
    }

    UCharacterStatusComponent* StatusComp = Defender->GetStatusComponent();
    if (!StatusComp || !IsValid(StatusComp))
    {
        return 0;
    }

    // 事前計算済みの防御値を直接返す
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.DefenseValue;
}

int32 UCombatCalculator::CalculateFinalDamage(int32 BaseDamage, int32 DefenseValue, bool bParried, bool bShieldBlocked, bool bCritical, int32 ShieldDefense, float ShieldSkill)
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
    
    // 盾防御処理（新システム）
    if (bShieldBlocked)
    {
        float ShieldReduction = CalculateShieldDamageReduction(ShieldDefense, ShieldSkill);
        FinalDamage *= ShieldReduction;
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
    Result.ShieldChance = CalculateShieldChance(Defender);
    Result.CriticalChance = CalculateCriticalChance(Attacker, WeaponItemId);
    
    // 判定順序：回避 → 命中 → 受け流し → 盾防御 → クリティカル → ダメージ計算
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
    
    // 4. 盾防御判定
    RandomValue = FMath::FRand() * 100.0f;
    if (RandomValue < Result.ShieldChance)
    {
        Result.bShieldBlocked = true;
    }
    
    // 5. クリティカル判定
    RandomValue = FMath::FRand() * 100.0f;
    if (RandomValue < Result.CriticalChance)
    {
        Result.bCritical = true;
    }
    
    // 6. ダメージ計算
    Result.BaseDamage = CalculateBaseDamage(Attacker, WeaponItemId);
    int32 DefenseValue = CalculateDefenseValue(Defender);
    int32 ShieldDefense = GetShieldDefense(Defender);
    float ShieldSkill = GetSkillLevel(Defender, ESkillType::Shield);
    Result.FinalDamage = CalculateFinalDamage(Result.BaseDamage, DefenseValue, Result.bParried, Result.bShieldBlocked, Result.bCritical, ShieldDefense, ShieldSkill);
    
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
    // 完全にStatusComponentアクセスを回避してデフォルト値のみ使用
    FCharacterTalent DefaultTalent;
    DefaultTalent.Strength = 10.0f;
    DefaultTalent.Toughness = 10.0f;
    DefaultTalent.Intelligence = 10.0f;
    DefaultTalent.Dexterity = 10.0f;
    DefaultTalent.Agility = 10.0f;
    DefaultTalent.Willpower = 10.0f;
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("GetCharacterTalent: Using safe default values"));
    return DefaultTalent;
}

float UCombatCalculator::GetWeaponWeight(const FString& WeaponItemId)
{
    // 空文字列や"unarmed"の場合は素手重量
    if (WeaponItemId.IsEmpty() || WeaponItemId == TEXT("unarmed"))
    {
        return 0.5f; // 素手の重量
    }
    
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
    
    return 0.5f; // 武器が見つからない場合も素手扱い
}

int32 UCombatCalculator::GetWeaponAttackPower(const FString& WeaponItemId)
{
    // 空文字列や"unarmed"の場合は素手攻撃力（格闘スキル依存）
    if (WeaponItemId.IsEmpty() || WeaponItemId == TEXT("unarmed"))
    {
        return 3; // 基本素手攻撃力（格闘スキルで上昇）
    }
    
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
    
    return 3; // 武器が見つからない場合も素手扱い
}

bool UCombatCalculator::IsRangedWeapon(const FString& WeaponItemId)
{
    // 空文字列や"unarmed"の場合は近接武器（素手）
    if (WeaponItemId.IsEmpty() || WeaponItemId == TEXT("unarmed"))
    {
        return false;
    }
    
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
    // 素手戦闘の場合
    if (WeaponItemId.IsEmpty() || WeaponItemId == TEXT("unarmed"))
    {
        return ESkillType::Combat;
    }
    
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
    
    // デフォルトは片手武器
    return ESkillType::OneHandedWeapons;
}

float UCombatCalculator::GetTotalEquipmentWeight(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return 0.0f;
    }

    float TotalWeight = 0.0f;
    
    if (UInventoryComponent* InventoryComp = Character->GetInventoryComponent())
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
    
    if (UInventoryComponent* InventoryComp = Character->GetInventoryComponent())
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

// 爪牙システム対応関数の実装

FString UCombatCalculator::GetCharacterRace(AC_IdleCharacter* Character)
{
    if (!Character || !IsValid(Character))
    {
        return TEXT("human"); // デフォルトは人間
    }

    // キャラクターから種族を取得（直接メンバー関数を呼び出し）
    return Character->GetCharacterRace();
}

FString UCombatCalculator::GetEffectiveWeaponId(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return TEXT("");
    }

    // 1. 装備武器の確認
    // TODO: 装備システムが実装されたら、ここで装備武器をチェック
    FString EquippedWeapon = TEXT(""); // 一時的
    
    if (!EquippedWeapon.IsEmpty())
    {
        return EquippedWeapon;
    }

    // 2. 装備武器がない場合、キャラクターの種族に応じた自然武器を返す
    FString CharacterRace = GetCharacterRace(Character);
    
    // CharacterPresetsから自然武器データを取得
    UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
    if (GameInstance)
    {
        UCharacterPresetManager* PresetManager = GameInstance->GetSubsystem<UCharacterPresetManager>();
        if (PresetManager)
        {
            if (PresetManager->DoesPresetExist(CharacterRace))
            {
                FCharacterPresetDataRow PresetData = PresetManager->GetCharacterPreset(CharacterRace);
                if (!PresetData.NaturalWeaponId.IsEmpty())
                {
                    return PresetData.NaturalWeaponId;
                }
            }
        }
    }

    // デフォルトは人間の素手
    return TEXT("");
}

bool UCombatCalculator::IsNaturalWeapon(const FString& WeaponId)
{
    // 自然武器の判定
    if (WeaponId.IsEmpty() || WeaponId == TEXT("unarmed"))
    {
        return true; // 素手は自然武器扱い
    }
    
    // 自然武器のIDパターンを確認
    return WeaponId.Contains(TEXT("_bite")) || 
           WeaponId.Contains(TEXT("_claw")) || 
           WeaponId.Contains(TEXT("_fang")) || 
           WeaponId.Contains(TEXT("_tongue"));
}

int32 UCombatCalculator::CalculateNaturalWeaponDamage(AC_IdleCharacter* Attacker, const FString& NaturalWeaponId)
{
    if (!Attacker)
    {
        return 1;
    }

    FCharacterTalent Talent = GetCharacterTalent(Attacker);
    float CombatSkill = GetSkillLevel(Attacker, ESkillType::Combat);
    
    // 自然武器の基本攻撃力を取得
    int32 BaseAttackPower = 2; // 人間の素手攻撃力（デフォルト）
    
    if (!NaturalWeaponId.IsEmpty() && NaturalWeaponId != TEXT("unarmed"))
    {
        // CharacterPresetsから自然武器攻撃力を取得
        FString CharacterRace = GetCharacterRace(Attacker);
        BaseAttackPower = GetNaturalWeaponPower(CharacterRace);
    }
    
    // 自然武器ダメージ = 自然武器攻撃力 + (格闘スキル × 0.8) + (力 × 0.7)
    float NaturalDamage = BaseAttackPower + (CombatSkill * 0.8f) + (Talent.Strength * 0.7f);
    
    return FMath::Max(1, FMath::RoundToInt(NaturalDamage));
}

int32 UCombatCalculator::CalculateArtificialWeaponDamage(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
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

int32 UCombatCalculator::GetNaturalWeaponPower(const FString& CharacterRace)
{
    if (CharacterRace.IsEmpty() || CharacterRace == TEXT("human"))
    {
        return 2; // 人間の素手攻撃力
    }

    // CharacterPresetsから自然武器攻撃力を取得
    UWorld* World = GEngine->GetCurrentPlayWorld();
    if (World)
    {
        UGameInstance* GameInstance = World->GetGameInstance();
        if (GameInstance)
        {
            UCharacterPresetManager* PresetManager = GameInstance->GetSubsystem<UCharacterPresetManager>();
            if (PresetManager)
            {
                if (PresetManager->DoesPresetExist(CharacterRace))
                {
                    FCharacterPresetDataRow PresetData = PresetManager->GetCharacterPreset(CharacterRace);
                    return PresetData.NaturalWeaponPower;
                }
            }
        }
    }
    
    return 2; // 見つからない場合は人間の素手攻撃力
}

int32 UCombatCalculator::GetShieldDefense(AC_IdleCharacter* Character)
{
    if (!Character || !IsValid(Character))
    {
        return 0;
    }

    // TODO: 装備システム実装後に正式実装
    // 現在は暫定的にインベントリから盾を検索
    UInventoryComponent* InventoryComp = Character->GetInventoryComponent();
    if (!InventoryComp)
    {
        return 0;
    }

    TArray<FInventorySlot> AllSlots = InventoryComp->GetAllInventorySlots();
    
    UGameInstance* GameInstance = Character->GetWorld()->GetGameInstance();
    if (!GameInstance)
    {
        return 0;
    }

    UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
    if (!ItemManager)
    {
        return 0;
    }

    for (const FInventorySlot& Slot : AllSlots)
    {
        FItemDataRow ItemData;
        if (ItemManager->GetItemData(Slot.ItemId, ItemData))
        {
            // 盾は防具の一種として判定（Shieldスロット）
            if (ItemData.ItemType == EItemTypeTable::Armor && ItemData.EquipmentSlot == EEquipmentSlotTable::Shield)
            {
                return ItemData.GetModifiedDefense(); // 品質修正済み防御力
            }
        }
    }
    
    return 0; // 盾が見つからない場合
}

float UCombatCalculator::CalculateShieldDamageReduction(int32 ShieldDefense, float ShieldSkill)
{
    // 盾ダメージカット率計算
    // 防御値 = 盾防御力 + 盾スキル
    float DefenseValue = ShieldDefense + ShieldSkill;
    
    // ダメージ倍率計算: 1/(1 + defense_value * 0.5)
    // 最低2%は通すため、最大98%カット
    float DamageMultiplier = FMath::Max(0.02f, 1.0f / (1.0f + DefenseValue * 0.5f));
    
    return DamageMultiplier;
}