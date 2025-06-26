#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "CharacterTypes.generated.h"

// ===========================================
// ENUMS
// ===========================================

UENUM(BlueprintType)
enum class EClubType : uint8
{
    Baseball        UMETA(DisplayName = "野球部"),
    Kendo          UMETA(DisplayName = "剣道部"),
    Chemistry      UMETA(DisplayName = "化学部"),
    Archery        UMETA(DisplayName = "アーチェリー部"),
    Karate         UMETA(DisplayName = "空手部"),
    AmericanFootball UMETA(DisplayName = "アメフト部"),
    Golf           UMETA(DisplayName = "ゴルフ部"),
    TrackAndField  UMETA(DisplayName = "陸上部"),
    Drama          UMETA(DisplayName = "演劇部"),
    TeaCeremony    UMETA(DisplayName = "茶道部"),
    Equestrian     UMETA(DisplayName = "馬術部"),
    Robotics       UMETA(DisplayName = "ロボット研究会"),
    Gardening      UMETA(DisplayName = "園芸部"),
    Astronomy      UMETA(DisplayName = "天文部"),
    TableTennis    UMETA(DisplayName = "卓球部"),
    Basketball     UMETA(DisplayName = "バスケ部"),
    Badminton      UMETA(DisplayName = "バドミントン部"),
    Tennis         UMETA(DisplayName = "テニス部"),
    Volleyball     UMETA(DisplayName = "バレー部"),
    Soccer         UMETA(DisplayName = "サッカー部"),
    Sumo           UMETA(DisplayName = "相撲部"),
    Cooking        UMETA(DisplayName = "料理部"),
    Medical        UMETA(DisplayName = "医学部"),
    Nursing        UMETA(DisplayName = "看護部"),
    
    Count          UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ESkillType : uint8
{
    OneHandedWeapons    UMETA(DisplayName = "片手武器"),
    TwoHandedWeapons    UMETA(DisplayName = "両手武器"),
    PolearmWeapons      UMETA(DisplayName = "長柄武器"),
    Archery             UMETA(DisplayName = "弓"),
    Firearms            UMETA(DisplayName = "射撃"),
    Throwing            UMETA(DisplayName = "投擲"),
    Shield              UMETA(DisplayName = "盾"),
    Lockpicking         UMETA(DisplayName = "解錠"),
    Swimming            UMETA(DisplayName = "水泳"),
    Engineering         UMETA(DisplayName = "工学"),
    Chemistry           UMETA(DisplayName = "化学"),
    Agriculture         UMETA(DisplayName = "農業"),
    Cooking             UMETA(DisplayName = "料理"),
    Tailoring           UMETA(DisplayName = "裁縫"),
    Construction        UMETA(DisplayName = "建築"),
    AnimalHandling      UMETA(DisplayName = "調教"),
    Survival            UMETA(DisplayName = "サバイバル"),
    Crafting            UMETA(DisplayName = "工作"),
    Mechanics           UMETA(DisplayName = "機械"),
    Fishing             UMETA(DisplayName = "釣り"),
    Medicine            UMETA(DisplayName = "医学"),
    Music               UMETA(DisplayName = "楽器"),
    Combat              UMETA(DisplayName = "格闘"),
    Evasion             UMETA(DisplayName = "回避"),
    Parry               UMETA(DisplayName = "受け流し"),
    
    Count               UMETA(Hidden)
};

// ===========================================
// STRUCTS
// ===========================================

USTRUCT(BlueprintType)
struct FSkillTalent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Talent")
    ESkillType SkillType;

    UPROPERTY(BlueprintReadWrite, Category = "Talent")
    float Value;

    FSkillTalent()
    {
        SkillType = ESkillType::OneHandedWeapons;
        Value = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FCharacterTalent
{
    GENERATED_BODY()

    // 基本能力値 (1-30)
    UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
    float Strength;

    UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
    float Toughness;

    UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
    float Intelligence;

    UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
    float Dexterity;

    UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
    float Agility;

    UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
    float Willpower;

    // スキル (1-4個, 各1-20)
    UPROPERTY(BlueprintReadWrite, Category = "Talent|Skills")
    TArray<FSkillTalent> Skills;

    FCharacterTalent()
    {
        Strength = 1.0f;
        Toughness = 1.0f;
        Intelligence = 1.0f;
        Dexterity = 1.0f;
        Agility = 1.0f;
        Willpower = 1.0f;
    }
};

USTRUCT(BlueprintType)
struct FCharacterStatus
{
    GENERATED_BODY()

    // 変動ステータスの最大値
    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float MaxHealth;

    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float MaxStamina;

    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float MaxMental;

    // 現在の値（通常は最大値と同じで開始）
    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float CurrentHealth;

    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float CurrentStamina;

    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float CurrentMental;

    // 積載量 (Kg)
    UPROPERTY(BlueprintReadWrite, Category = "Status")
    float CarryingCapacity;

    FCharacterStatus()
    {
        MaxHealth = 100.0f;
        MaxStamina = 100.0f;
        MaxMental = 100.0f;
        CurrentHealth = MaxHealth;
        CurrentStamina = MaxStamina;
        CurrentMental = MaxMental;
        CarryingCapacity = 20.0f;
    }
};

USTRUCT(BlueprintType)
struct FClubBonus
{
    GENERATED_BODY()

    // 基本能力値ボーナス
    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    float Strength;

    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    float Toughness;

    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    float Intelligence;

    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    float Dexterity;

    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    float Agility;

    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    float Willpower;

    // スキルボーナス
    UPROPERTY(BlueprintReadWrite, Category = "Club Bonus")
    TArray<FSkillTalent> SkillBonuses;

    FClubBonus()
    {
        Strength = 0.0f;
        Toughness = 0.0f;
        Intelligence = 0.0f;
        Dexterity = 0.0f;
        Agility = 0.0f;
        Willpower = 0.0f;
    }
};

// 派生ステータス（事前計算済み）
USTRUCT(BlueprintType)
struct UE_IDLE_API FDerivedStats
{
    GENERATED_BODY()

    // 戦闘関連 - 実際の計算で使用
    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    float AttackSpeed = 1.0f;        // 攻撃速度

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    float HitChance = 50.0f;         // 命中率

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    float CriticalChance = 5.0f;     // クリティカル率

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    int32 BaseDamage = 1;            // 基本ダメージ

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    float DodgeChance = 10.0f;       // 回避率

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    float ParryChance = 5.0f;        // 受け流し率

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    float ShieldChance = 3.0f;       // 盾防御率

    UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
    int32 DefenseValue = 0;          // 防御値

    // 作業関連
    UPROPERTY(BlueprintReadOnly, Category = "Work Stats")
    float ConstructionPower = 10.0f; // 建築能力

    UPROPERTY(BlueprintReadOnly, Category = "Work Stats")
    float ProductionPower = 10.0f;   // 生産能力

    UPROPERTY(BlueprintReadOnly, Category = "Work Stats")
    float GatheringPower = 10.0f;    // 採集能力

    UPROPERTY(BlueprintReadOnly, Category = "Work Stats")
    float CookingPower = 10.0f;      // 料理能力

    UPROPERTY(BlueprintReadOnly, Category = "Work Stats")
    float CraftingPower = 10.0f;     // 工作能力

    // UI表示用（戦闘計算では使用しない）
    UPROPERTY(BlueprintReadOnly, Category = "Display Stats")
    float DPS = 1.0f;                // ダメージ/秒（表示用）

    UPROPERTY(BlueprintReadOnly, Category = "Display Stats")
    float TotalDefensePower = 10.0f; // 総合防御能力（表示用）

    UPROPERTY(BlueprintReadOnly, Category = "Display Stats")
    float CombatPower = 25.0f;       // 戦闘力総合値（表示用）

    UPROPERTY(BlueprintReadOnly, Category = "Display Stats")
    float WorkPower = 50.0f;         // 作業力総合値（表示用）

    FDerivedStats()
    {
        // デフォルト値は上記で設定済み
    }

    // UI用ヘルパー関数
    float GetOverallCombatRating() const
    {
        return (DPS * 0.4f) + (TotalDefensePower * 0.3f) + (HitChance * 0.2f) + (CriticalChance * 0.1f);
    }

    float GetOverallWorkRating() const
    {
        return (ConstructionPower + ProductionPower + GatheringPower + CookingPower + CraftingPower) / 5.0f;
    }
};