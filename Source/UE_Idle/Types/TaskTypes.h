#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TeamTypes.h"
#include "TaskTypes.generated.h"

// Forward declarations
class UTeamComponent;
class UTaskManagerComponent;

// タスク切り替えタイプ
UENUM(BlueprintType)
enum class ETaskSwitchType : uint8
{
    Normal          UMETA(DisplayName = "通常"),
    PostCombat      UMETA(DisplayName = "戦闘後"),
    ResourceChange  UMETA(DisplayName = "リソース変化"),
    Forced          UMETA(DisplayName = "強制")
};

// 採集数量タイプ
UENUM(BlueprintType)
enum class EGatheringQuantityType : uint8
{
    Unlimited   UMETA(DisplayName = "無制限"),     // 従来の無制限採集
    Specified   UMETA(DisplayName = "個数指定"),   // 採集するたびに減る
    Keep        UMETA(DisplayName = "個数キープ")  // 常に指定数をキープ
};

// ======== タスク実行計画システム ========

// タスク実行アクション
UENUM(BlueprintType)
enum class ETaskExecutionAction : uint8
{
    None            UMETA(DisplayName = "何もしない"),
    MoveToLocation  UMETA(DisplayName = "指定場所へ移動"),
    ExecuteGathering UMETA(DisplayName = "採集実行"),
    ExecuteCombat   UMETA(DisplayName = "戦闘実行"),
    ReturnToBase    UMETA(DisplayName = "拠点帰還"),
    UnloadItems     UMETA(DisplayName = "荷下ろし"),
    WaitIdle        UMETA(DisplayName = "待機")
};

// タスク実行計画
USTRUCT(BlueprintType)
struct UE_IDLE_API FTaskExecutionPlan
{
    GENERATED_BODY()

    // 実行すべきアクション
    UPROPERTY(BlueprintReadWrite, Category = "Plan")
    ETaskExecutionAction ExecutionAction;
    
    // 対象タスクID（実行時）
    UPROPERTY(BlueprintReadWrite, Category = "Plan")
    FString TaskId;
    
    // 目標場所（移動時）
    UPROPERTY(BlueprintReadWrite, Category = "Plan")
    FString TargetLocation;
    
    // 対象アイテム（採集・戦闘時）
    UPROPERTY(BlueprintReadWrite, Category = "Plan")
    FString TargetItem;
    
    // 実行指示の理由（ログ用）
    UPROPERTY(BlueprintReadWrite, Category = "Plan")
    FString ExecutionReason;
    
    // 成功フラグ
    UPROPERTY(BlueprintReadWrite, Category = "Plan")
    bool bIsValid;
    
    FTaskExecutionPlan()
    {
        ExecutionAction = ETaskExecutionAction::None;
        TaskId = TEXT("");
        TargetLocation = TEXT("");
        TargetItem = TEXT("");
        ExecutionReason = TEXT("");
        bIsValid = false;
    }
};

// チームアクション状態とECombatStateはTeamTypes.hで定義

// ======== タスクとスキルの関連定義 ========

// タスクに関連するスキル情報
USTRUCT(BlueprintType)
struct UE_IDLE_API FTaskRelatedSkills
{
    GENERATED_BODY()

    // 主要スキル（進行速度に最も影響）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    FString PrimarySkill;
    
    // 副次スキル（進行速度に中程度影響）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    FString SecondarySkill;
    
    // 補助スキル（進行速度に小影響）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    FString TertiarySkill;
    
    // スキル影響度の重み (合計1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    float PrimaryWeight = 0.6f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    float SecondaryWeight = 0.3f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    float TertiaryWeight = 0.1f;

    FTaskRelatedSkills()
    {
        PrimarySkill = "";
        SecondarySkill = "";
        TertiarySkill = "";
        PrimaryWeight = 0.6f;
        SecondaryWeight = 0.3f;
        TertiaryWeight = 0.1f;
    }
    
    bool HasSkills() const
    {
        return !PrimarySkill.IsEmpty();
    }
};

// 全体タスク構造体
USTRUCT(BlueprintType)
struct FGlobalTask
{
    GENERATED_BODY()

    // タスクID（一意識別子）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString TaskId;
    
    // 表示名
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString DisplayName;
    
    // 優先度（1-20, 1が最高優先度）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ClampMin = "1", ClampMax = "20"))
    int32 Priority;
    
    // タスクタイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    ETaskType TaskType;
    
    // 対象アイテムID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FString TargetItemId;
    
    // 目標数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ClampMin = "1"))
    int32 TargetQuantity;
    
    // 関連スキル情報
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Skills")
    FTaskRelatedSkills RelatedSkills;
    
    // 現在の進行状況
    UPROPERTY(BlueprintReadWrite, Category = "Task")
    int32 CurrentProgress;
    
    // 完了フラグ
    UPROPERTY(BlueprintReadWrite, Category = "Task")
    bool bIsCompleted;
    
    // 数量キープ型（例：木1000をキープ）【廃止予定】
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    bool bIsKeepQuantity;
    
    // 採集数量タイプ（新仕様）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    EGatheringQuantityType GatheringQuantityType;
    
    // 作成日時（デバッグ用）
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    FDateTime CreatedTime;

    FGlobalTask()
    {
        TaskId = TEXT("");
        DisplayName = TEXT("");
        Priority = 1;
        TaskType = ETaskType::Idle;
        TargetItemId = TEXT("");
        TargetQuantity = 1;
        CurrentProgress = 0;
        bIsCompleted = false;
        bIsKeepQuantity = false;
        GatheringQuantityType = EGatheringQuantityType::Unlimited;
        CreatedTime = FDateTime::Now();
    }

    // 進行率計算（0.0-1.0）
    float GetProgressRatio() const
    {
        if (TargetQuantity <= 0)
        {
            return 0.0f;
        }
        return FMath::Clamp(static_cast<float>(CurrentProgress) / static_cast<float>(TargetQuantity), 0.0f, 1.0f);
    }

    // 残り数量計算
    int32 GetRemainingQuantity() const
    {
        return FMath::Max(0, TargetQuantity - CurrentProgress);
    }

    // タスクの有効性チェック
    bool IsValid() const
    {
        // 基本フィールドチェック
        if (TaskId.IsEmpty() || DisplayName.IsEmpty() || Priority < 1 || Priority > 20)
        {
            return false;
        }
        
        // 採集タスクで無制限の場合はTargetQuantity=0でもOK
        if (TaskType == ETaskType::Gathering && GatheringQuantityType == EGatheringQuantityType::Unlimited)
        {
            return TargetQuantity >= 0;
        }
        
        // その他のタスクはTargetQuantity > 0が必要
        return TargetQuantity > 0;
    }
};

// チーム用タスク構造体
USTRUCT(BlueprintType)
struct FTeamTask
{
    GENERATED_BODY()

    // チーム内優先順位（1-3）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task", meta = (ClampMin = "1", ClampMax = "3"))
    int32 Priority;
    
    // タスクタイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task")
    ETaskType TaskType;
    
    // 必要リソース条件（ItemId -> 必要数量）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task")
    TMap<FString, int32> RequiredResources;
    
    // 必要アイテム条件（ItemId -> 必要数量）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task")
    TMap<FString, int32> RequiredItems;
    
    // 最小必要人数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task", meta = (ClampMin = "1"))
    int32 MinTeamSize;
    
    // 推定完了時間（時間単位）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task", meta = (ClampMin = "0.1"))
    float EstimatedCompletionTime;
    
    // 現在実行中かどうか
    UPROPERTY(BlueprintReadWrite, Category = "Team Task")
    bool bIsActive;
    
    // 開始時刻
    UPROPERTY(BlueprintReadWrite, Category = "Team Task")
    float StartTime;

    FTeamTask()
    {
        Priority = 1;
        TaskType = ETaskType::Idle;
        MinTeamSize = 1;
        EstimatedCompletionTime = 1.0f;
        bIsActive = false;
        StartTime = 0.0f;
    }

    // タスクの有効性チェック
    bool IsValid() const
    {
        return Priority >= 1 && Priority <= 3 &&
               MinTeamSize >= 1 &&
               EstimatedCompletionTime > 0.0f;
    }

    // 経過時間計算
    float GetElapsedTime(float CurrentTime) const
    {
        if (!bIsActive || StartTime <= 0.0f)
        {
            return 0.0f;
        }
        return FMath::Max(0.0f, CurrentTime - StartTime);
    }

    // 残り時間計算
    float GetRemainingTime(float CurrentTime) const
    {
        if (!bIsActive)
        {
            return EstimatedCompletionTime;
        }
        float ElapsedTime = GetElapsedTime(CurrentTime);
        return FMath::Max(0.0f, EstimatedCompletionTime - ElapsedTime);
    }
};

// チームタスクリスト構造体（UPROPERTYでネストTArrayを避けるため）
USTRUCT(BlueprintType)
struct UE_IDLE_API FTeamTaskList
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Tasks")
    TArray<FTeamTask> Tasks;

    FTeamTaskList()
    {
        Tasks.Empty();
    }

    // 便利関数
    int32 Num() const { return Tasks.Num(); }
    bool IsEmpty() const { return Tasks.IsEmpty(); }
    void Empty() { Tasks.Empty(); }
    void Add(const FTeamTask& Task) { Tasks.Add(Task); }
    void RemoveAt(int32 Index) { Tasks.RemoveAt(Index); }
    bool IsValidIndex(int32 Index) const { return Tasks.IsValidIndex(Index); }
    FTeamTask& operator[](int32 Index) { return Tasks[Index]; }
    const FTeamTask& operator[](int32 Index) const { return Tasks[Index]; }

    // Tasks配列への直接アクセス
    TArray<FTeamTask>& GetTasks() { return Tasks; }
    const TArray<FTeamTask>& GetTasks() const { return Tasks; }
};

// 遅延タスク切り替え構造体
USTRUCT(BlueprintType)
struct FDelayedTaskSwitch
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Task Switch")
    int32 TeamIndex;
    
    UPROPERTY(BlueprintReadWrite, Category = "Task Switch")
    ETaskSwitchType SwitchType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Task Switch")
    float Timestamp;

    FDelayedTaskSwitch()
    {
        TeamIndex = -1;
        SwitchType = ETaskSwitchType::Normal;
        Timestamp = 0.0f;
    }

    // 有効性チェック
    bool IsValid() const
    {
        return TeamIndex >= 0 && Timestamp >= 0.0f;
    }
};

// チームタスクスナップショット（オフライン処理用）
USTRUCT(BlueprintType)
struct FTeamTaskSnapshot
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Snapshot")
    int32 TeamIndex;
    
    UPROPERTY(BlueprintReadWrite, Category = "Snapshot")
    TArray<FTeamTask> TeamTasks;
    
    UPROPERTY(BlueprintReadWrite, Category = "Snapshot")
    ETeamActionState ActionState;
    
    UPROPERTY(BlueprintReadWrite, Category = "Snapshot")
    float LastUpdateTime;

    FTeamTaskSnapshot()
    {
        TeamIndex = -1;
        ActionState = ETeamActionState::Idle;
        LastUpdateTime = 0.0f;
    }
};

// イベントディスパッチャー用デリゲート宣言

// 全体タスク関連
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGlobalTaskAdded, const FGlobalTask&, NewTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGlobalTaskCompleted, const FGlobalTask&, CompletedTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskPriorityChanged, int32, TaskIndex, int32, NewPriority);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGlobalTaskRemoved, int32, TaskIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTaskQuantityUpdated, int32, TaskIndex, int32, OldQuantity, int32, NewQuantity);

// 時間管理関連
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeSystemStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeSystemStopped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHourPassed, float, CurrentHour);

// オフライン進行関連
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOfflineProgressCalculated, float, OfflineHours, const TArray<FGlobalTask>&, CompletedTasks);

// タスクタイプユーティリティ関数
UCLASS(BlueprintType)
class UE_IDLE_API UTaskTypeUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // タスクタイプの表示名を取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    static FString GetTaskTypeDisplayName(ETaskType TaskType);

    // 文字列からタスクタイプを取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    static ETaskType GetTaskTypeFromString(const FString& TaskName);

    // 全タスクタイプの一覧を取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    static TArray<ETaskType> GetAllTaskTypes();

    // タスクタイプが有効かチェック
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    static bool IsValidTaskType(ETaskType TaskType);

    // アクション状態の表示名を取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    static FString GetActionStateDisplayName(ETeamActionState ActionState);

    // 戦闘状態の表示名を取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    static FString GetCombatStateDisplayName(ETeamCombatState CombatState);

    // ======== タスクとスキルの関連管理 ========
    
    // タスクタイプに関連するスキル情報を取得
    UFUNCTION(BlueprintPure, Category = "Task Skills")
    static FTaskRelatedSkills GetTaskRelatedSkills(ETaskType TaskType);
    
    // タスクに必要なスキルの表示名リストを取得
    UFUNCTION(BlueprintPure, Category = "Task Skills")
    static TArray<FString> GetTaskSkillDisplayNames(ETaskType TaskType);
    
    // スキル名をFDerivedStatsのプロパティ名に変換（例: "建築" → "ConstructionPower"）
    UFUNCTION(BlueprintPure, Category = "Task Skills")
    static FString ConvertSkillNameToPropertyName(const FString& SkillName);
};
