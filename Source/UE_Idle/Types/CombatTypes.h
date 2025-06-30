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

    // 盾防御されたか
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    bool bShieldBlocked;

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

    // 盾防御率
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    float ShieldChance;

    // クリティカル率
    UPROPERTY(BlueprintReadWrite, Category = "Result")
    float CriticalChance;

    FCombatCalculationResult()
    {
        bHit = false;
        bDodged = false;
        bParried = false;
        bShieldBlocked = false;
        bCritical = false;
        FinalDamage = 0;
        BaseDamage = 0;
        HitChance = 0.0f;
        DodgeChance = 0.0f;
        ParryChance = 0.0f;
        ShieldChance = 0.0f;
        CriticalChance = 0.0f;
    }
};

// 戦闘行動情報（名前変更してCharacterTypes.hとの競合を回避）
USTRUCT(BlueprintType)
struct FCombatAction
{
    GENERATED_BODY()

    // 行動するキャラクター
    UPROPERTY(BlueprintReadWrite, Category = "Combat Action")
    AC_IdleCharacter* Character;

    // 行動タイプ
    UPROPERTY(BlueprintReadWrite, Category = "Combat Action")
    EActionType ActionType;

    // 対象キャラクター（攻撃対象など）
    UPROPERTY(BlueprintReadWrite, Category = "Combat Action")
    AC_IdleCharacter* TargetCharacter;

    // 次の行動可能時刻（既存システム用）
    UPROPERTY(BlueprintReadWrite, Category = "Combat Action")
    float NextActionTime;

    // 攻撃速度（秒間行動回数）
    UPROPERTY(BlueprintReadWrite, Category = "Combat Action")
    float AttackSpeed;

    // === 新しい行動ゲージシステム ===
    
    // 行動ゲージ（0-100）
    UPROPERTY(BlueprintReadWrite, Category = "Action Gauge")
    float ActionGauge;
    
    // ゲージ増加速度（毎ターン増加量）
    UPROPERTY(BlueprintReadWrite, Category = "Action Gauge")
    float GaugeSpeed;
    
    // 速度倍率（バフ/デバフ用）
    UPROPERTY(BlueprintReadWrite, Category = "Action Gauge")
    float SpeedMultiplier;
    
    // 行動優先度（同ゲージ値時の順序決定用）
    UPROPERTY(BlueprintReadWrite, Category = "Action Gauge")
    int32 ActionPriority;

    FCombatAction()
    {
        Character = nullptr;
        ActionType = EActionType::Attack;
        TargetCharacter = nullptr;
        NextActionTime = 0.0f;
        AttackSpeed = 1.0f;
        
        // 新システム初期化
        ActionGauge = 0.0f;
        GaugeSpeed = 10.0f;  // デフォルト速度
        SpeedMultiplier = 1.0f;
        ActionPriority = 0;
    }
    
    // 行動可能かチェック
    bool CanAct() const
    {
        return ActionGauge >= 100.0f;
    }
    
    // 行動後のゲージリセット
    void ResetAfterAction()
    {
        ActionGauge = 0.0f;
    }
    
    // ターン毎のゲージ更新
    void UpdateGauge()
    {
        ActionGauge += GaugeSpeed * SpeedMultiplier;
        ActionGauge = FMath::Clamp(ActionGauge, 0.0f, 200.0f); // オーバーフロー防止
    }
    
    // 行動優先度計算（ゲージ値 + 優先度）
    float GetActionScore() const
    {
        return ActionGauge + (ActionPriority * 0.01f); // 優先度は小数点以下に影響
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