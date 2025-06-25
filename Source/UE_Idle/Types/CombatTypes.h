#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "CombatTypes.generated.h"

class AC_IdleCharacter;

// 戦闘ログのタイプ
UENUM(BlueprintType)
enum class ECombatLogType : uint8
{
    Attack      UMETA(DisplayName = "攻撃"),
    Hit         UMETA(DisplayName = "命中"),
    Miss        UMETA(DisplayName = "外れ"),
    Dodge       UMETA(DisplayName = "回避"),
    Parry       UMETA(DisplayName = "受け流し"),
    Damage      UMETA(DisplayName = "ダメージ"),
    Critical    UMETA(DisplayName = "クリティカル"),
    Death       UMETA(DisplayName = "死亡"),
    CombatStart UMETA(DisplayName = "戦闘開始"),
    CombatEnd   UMETA(DisplayName = "戦闘終了"),
    
    Count       UMETA(Hidden)
};

// 戦闘状態
UENUM(BlueprintType)
enum class ECombatState : uint8
{
    Inactive    UMETA(DisplayName = "非戦闘"),
    Preparing   UMETA(DisplayName = "準備中"),
    InProgress  UMETA(DisplayName = "戦闘中"),
    Completed   UMETA(DisplayName = "戦闘終了"),
    
    Count       UMETA(Hidden)
};

// 行動タイプ
UENUM(BlueprintType)
enum class EActionType : uint8
{
    Attack      UMETA(DisplayName = "攻撃"),
    Move        UMETA(DisplayName = "移動"),
    Guard       UMETA(DisplayName = "ガード"),
    UseItem     UMETA(DisplayName = "アイテム使用"),
    
    Count       UMETA(Hidden)
};

// 戦闘ログエントリは EventLogTypes.h の FEventLogEntry を使用
// 後方互換性のため、このファイルでは削除済み

// 戦闘計算結果
USTRUCT(BlueprintType)
struct FCombatCalculationResult
{
    GENERATED_BODY()

    // 命中したか
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    bool bHit;

    // 回避されたか
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    bool bDodged;

    // 受け流されたか
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    bool bParried;

    // クリティカルヒットか
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    bool bCritical;

    // 最終ダメージ
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    int32 FinalDamage;

    // 基本ダメージ（防御前）
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    int32 BaseDamage;

    // 命中率
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    float HitChance;

    // 回避率
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    float DodgeChance;

    // 受け流し率
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    float ParryChance;

    // クリティカル率
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    float CriticalChance;

    FCombatCalculationResult()
    {
        bHit = false;
        bDodged = false;
        bParried = false;
        bCritical = false;
        FinalDamage = 0;
        BaseDamage = 0;
        HitChance = 0.0f;
        DodgeChance = 0.0f;
        ParryChance = 0.0f;
        CriticalChance = 0.0f;
    }
};

// キャラクターの行動情報
USTRUCT(BlueprintType)
struct FCharacterAction
{
    GENERATED_BODY()

    // 行動するキャラクター
    UPROPERTY(BlueprintReadWrite, Category = "Action")
    AC_IdleCharacter* Character;

    // 行動タイプ
    UPROPERTY(BlueprintReadWrite, Category = "Action")
    EActionType ActionType;

    // 対象キャラクター（攻撃対象など）
    UPROPERTY(BlueprintReadWrite, Category = "Action")
    AC_IdleCharacter* TargetCharacter;

    // 次の行動可能時刻
    UPROPERTY(BlueprintReadWrite, Category = "Action")
    float NextActionTime;

    // 攻撃速度（秒間行動回数）
    UPROPERTY(BlueprintReadWrite, Category = "Action")
    float AttackSpeed;

    FCharacterAction()
    {
        Character = nullptr;
        ActionType = EActionType::Attack;
        TargetCharacter = nullptr;
        NextActionTime = 0.0f;
        AttackSpeed = 1.0f;
    }
};

// 装備重量ペナルティ情報
USTRUCT(BlueprintType)
struct FEquipmentPenalty
{
    GENERATED_BODY()

    // 装備総重量
    UPROPERTY(BlueprintReadWrite, Category = "Penalty")
    float TotalWeight;

    // 積載量
    UPROPERTY(BlueprintReadWrite, Category = "Penalty")
    float CarryingCapacity;

    // 装備重量率（0.0-1.0+）
    UPROPERTY(BlueprintReadWrite, Category = "Penalty")
    float WeightRatio;

    // ペナルティ率（0.0-90.0）
    UPROPERTY(BlueprintReadWrite, Category = "Penalty")
    float PenaltyPercentage;

    FEquipmentPenalty()
    {
        TotalWeight = 0.0f;
        CarryingCapacity = 20.0f;
        WeightRatio = 0.0f;
        PenaltyPercentage = 0.0f;
    }
};