#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "../Types/ItemTypes.h"
#include "TeamTypes.generated.h"

// Forward declarations for avoiding circular dependencies
class UTeamComponent;

// チームアクション状態（TaskTypes.hから移動を避けるため、ここで定義）
UENUM(BlueprintType)
enum class ETeamActionState : uint8
{
    Idle            UMETA(DisplayName = "待機"),
    Moving          UMETA(DisplayName = "移動中"),       // 中断可能
    Working         UMETA(DisplayName = "作業中"),       // 中断可能
    InCombat        UMETA(DisplayName = "戦闘中"),       // 中断不可
    Locked          UMETA(DisplayName = "アクション中")   // 中断不可
};

// チーム戦闘状態の詳細管理
UENUM(BlueprintType)
enum class ETeamCombatState : uint8
{
    NotInCombat     UMETA(DisplayName = "非戦闘"),
    Starting        UMETA(DisplayName = "戦闘開始中"),
    InProgress      UMETA(DisplayName = "戦闘中"),
    Ending          UMETA(DisplayName = "戦闘終了処理中"),
    Finished        UMETA(DisplayName = "戦闘完了")
};

// タスクタイプ
UENUM(BlueprintType)
enum class ETaskType : uint8
{
    Idle            UMETA(DisplayName = "待機"),
    All             UMETA(DisplayName = "全て"),      // 全体タスク優先度順実行
    Adventure       UMETA(DisplayName = "冒険"),
    Cooking         UMETA(DisplayName = "料理"),
    Construction    UMETA(DisplayName = "建築"),     // 新規追加
    Gathering       UMETA(DisplayName = "採集"),     // 新規追加
    Crafting        UMETA(DisplayName = "製作"),     // 新規追加
    
    // 追加タスクタイプ（将来実装用）
    Farming         UMETA(DisplayName = "農業"),
    Mining          UMETA(DisplayName = "採掘"),
    Hunting         UMETA(DisplayName = "狩猟"),
    Fishing         UMETA(DisplayName = "釣り"),
    Research        UMETA(DisplayName = "研究"),
    Medical         UMETA(DisplayName = "治療"),
    Taming          UMETA(DisplayName = "調教"),
    Art             UMETA(DisplayName = "芸術"),
    Trading         UMETA(DisplayName = "交易"),
    Scouting        UMETA(DisplayName = "偵察")
};

// ECarrierType削除 - 新採集システムでは運搬キャラクターを使用

class AC_IdleCharacter;

// チーム構造体
USTRUCT(BlueprintType)
struct FTeam
{
    GENERATED_BODY()

    // チームメンバー
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    TArray<AC_IdleCharacter*> Members;

    // 割り当てられたタスク
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    ETaskType AssignedTask;

    // チーム名
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    FString TeamName;

    // チームがアクティブか
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    bool bIsActive;

    // 冒険先の場所ID（AdventureタスクでのみU使用）
    UPROPERTY(BlueprintReadWrite, Category = "Adventure")
    FString AdventureLocationId;

    // 採集先の場所ID（Gatheringタスクで使用）
    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    FString GatheringLocationId;

    // 戦闘中かどうか
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bInCombat;

    // 旧運搬手段フィールド削除
    // 新採集システムでは個人キャラクターの運搬能力を使用

    // === 新しいタスク管理機能 ===

    // チームアクション状態
    UPROPERTY(BlueprintReadWrite, Category = "Team State")
    ETeamActionState ActionState;

    // 戦闘状態
    UPROPERTY(BlueprintReadWrite, Category = "Combat State")
    ETeamCombatState CombatState;

    // アクション開始時刻
    UPROPERTY(BlueprintReadWrite, Category = "Team State")
    float ActionStartTime;

    // 推定完了時刻
    UPROPERTY(BlueprintReadWrite, Category = "Team State")
    float EstimatedCompletionTime;

    // 処理中フラグ（安全性確保用）
    UPROPERTY(BlueprintReadOnly, Category = "Team State")
    bool bProcessingAction;

    FTeam()
    {
        AssignedTask = ETaskType::Idle;  // デフォルトは待機
        bIsActive = true;
        AdventureLocationId = TEXT("");
        GatheringLocationId = TEXT("");
        bInCombat = false;
        // 旧運搬手段フィールド削除済み
        
        // 新しいフィールドの初期化
        ActionState = ETeamActionState::Idle;
        CombatState = ETeamCombatState::NotInCombat;
        ActionStartTime = 0.0f;
        EstimatedCompletionTime = 0.0f;
        bProcessingAction = false;
    }

    // 旧積載量計算・運搬手段メソッド削除
    // 新採集システムでは個人キャラクターの積載量をGatheringComponentで管理

    // === 新しいヘルパー関数 ===

    // アクション中断可能かチェック
    bool CanInterruptAction() const
    {
        return !bProcessingAction && 
               (ActionState == ETeamActionState::Idle || ActionState == ETeamActionState::Working);
    }

    // 戦闘中かチェック
    bool IsInCombat() const
    {
        return ActionState == ETeamActionState::InCombat || 
               CombatState == ETeamCombatState::InProgress ||
               CombatState == ETeamCombatState::Starting;
    }

    // 戦闘終了チェック（安全）
    bool IsCombatFinished() const
    {
        return CombatState == ETeamCombatState::Finished && !bProcessingAction;
    }

    // アクション残り時間計算
    float GetRemainingActionTime(float CurrentTime) const
    {
        if (ActionState == ETeamActionState::Idle || EstimatedCompletionTime <= 0.0f)
        {
            return 0.0f;
        }

        if (ActionStartTime <= 0.0f)
        {
            return EstimatedCompletionTime; // まだ開始していない
        }

        float ElapsedTime = FMath::Max(0.0f, CurrentTime - ActionStartTime);
        return FMath::Max(0.0f, EstimatedCompletionTime - ElapsedTime);
    }

    // アクション進行率計算（0.0-1.0）
    float GetActionProgressRatio(float CurrentTime) const
    {
        if (ActionState == ETeamActionState::Idle || EstimatedCompletionTime <= 0.0f)
        {
            return 0.0f;
        }

        if (ActionStartTime <= 0.0f)
        {
            return 0.0f; // まだ開始していない
        }

        float ElapsedTime = FMath::Max(0.0f, CurrentTime - ActionStartTime);
        return FMath::Clamp(ElapsedTime / EstimatedCompletionTime, 0.0f, 1.0f);
    }

    // 安全な戦闘開始
    void StartCombatSafe(float CurrentTime, float EstimatedDuration)
    {
        if (!bProcessingAction && !IsInCombat())
        {
            ActionState = ETeamActionState::InCombat;
            CombatState = ETeamCombatState::Starting;
            ActionStartTime = CurrentTime;
            EstimatedCompletionTime = EstimatedDuration;
            bInCombat = true; // 後方互換性のため
        }
    }

    // 安全な戦闘終了
    void EndCombatSafe()
    {
        if (!bProcessingAction)
        {
            CombatState = ETeamCombatState::Finished;
            // ActionStateとbInCombatは外部で安全に設定される
        }
    }

    // チームの有効性チェック
    bool IsValidTeam() const
    {
        return bIsActive && 
               !TeamName.IsEmpty() && 
               Members.Num() >= 0; // 0人チームも一時的に許可
    }

    // アクション状態の表示名取得
    FString GetActionStateDisplayName() const
    {
        switch (ActionState)
        {
            case ETeamActionState::Idle:
                return TEXT("待機");
            case ETeamActionState::Working:
                return TEXT("作業中");
            case ETeamActionState::InCombat:
                return TEXT("戦闘中");
            case ETeamActionState::Locked:
                return TEXT("アクション中");
            default:
                return TEXT("不明");
        }
    }

    // 戦闘状態の表示名取得
    FString GetCombatStateDisplayName() const
    {
        switch (CombatState)
        {
            case ETeamCombatState::NotInCombat:
                return TEXT("非戦闘");
            case ETeamCombatState::Starting:
                return TEXT("戦闘開始中");
            case ETeamCombatState::InProgress:
                return TEXT("戦闘中");
            case ETeamCombatState::Ending:
                return TEXT("戦闘終了処理中");
            case ETeamCombatState::Finished:
                return TEXT("戦闘完了");
            default:
                return TEXT("不明");
        }
    }
};