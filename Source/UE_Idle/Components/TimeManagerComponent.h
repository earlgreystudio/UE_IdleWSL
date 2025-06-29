#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/TaskTypes.h"
#include "../Types/TeamTypes.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "TimeManagerComponent.generated.h"

// Forward declarations
class UTaskManagerComponent;
class UTeamComponent;
class UGatheringComponent;
class ULocationMovementComponent;
class AC_IdleCharacter;
class AC_PlayerController;
class UInventoryComponent;
class UCharacterStatusComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UTimeManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimeManagerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // === 時間管理設定 ===

    // 時間システムアクティブフラグ
    UPROPERTY(BlueprintReadWrite, Category = "Time System")
    bool bTimeSystemActive = false;
    
    // 更新間隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time System", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float TimeUpdateInterval = 1.0f;

    // 高速処理モード（デバッグ用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time System")
    bool bFastProcessingMode = false;

    // 放置時間閾値（この時間以上で放置判定）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time System")
    float IdleThreshold = 300.0f; // 5分

    // 安全性設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
    bool bEnableDefensiveProgramming = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
    int32 MaxProcessingRetries = 3;

    // === 処理状態管理 ===

    // 処理中フラグ
    UPROPERTY(BlueprintReadOnly, Category = "Processing State")
    bool bProcessingTimeUpdate = false;

    // 遅延処理キュー
    UPROPERTY()
    TArray<FDelayedTaskSwitch> PendingTaskSwitches;

    // タイマーハンドル
    UPROPERTY()
    FTimerHandle TimeUpdateTimerHandle;

    // === 参照コンポーネント ===

    // タスクマネージャーへの参照
    UPROPERTY()
    UTaskManagerComponent* TaskManager = nullptr;
    
    // チームコンポーネントへの参照
    UPROPERTY()
    TArray<UTeamComponent*> TeamComponents;

    // 採集コンポーネントへの参照
    UPROPERTY()
    UGatheringComponent* GatheringComponent = nullptr;

    UPROPERTY()
    ULocationMovementComponent* MovementComponent = nullptr;

    // 現在時刻（ゲーム内時間）
    UPROPERTY(BlueprintReadOnly, Category = "Time")
    float CurrentGameTime = 0.0f;

    // 最後の処理時刻
    UPROPERTY()
    float LastProcessTime = 0.0f;

public:
    // === 時間システム制御 ===

    // 時間システム開始
    UFUNCTION(BlueprintCallable, Category = "Time System")
    void StartTimeSystem();
    
    // 時間システム停止
    UFUNCTION(BlueprintCallable, Category = "Time System")
    void StopTimeSystem();
    
    // 時間システム一時停止
    UFUNCTION(BlueprintCallable, Category = "Time System")
    void PauseTimeSystem();
    
    // 時間システム再開
    UFUNCTION(BlueprintCallable, Category = "Time System")
    void ResumeTimeSystem();

    // システム状態取得
    UFUNCTION(BlueprintPure, Category = "Time System")
    bool IsTimeSystemActive() const { return bTimeSystemActive; }

    // === 時間進行処理 ===

    // メイン時間更新処理
    UFUNCTION()
    void ProcessTimeUpdate();

    // 全体タスク進行処理
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessGlobalTasks();
    
    // チームタスク進行処理
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessTeamTasks();

    // リソース条件変化の監視
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void CheckResourceConditions();

    // タスク完了・切り替えの判定
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessTaskSwitching();

    // 時間イベント発信
    UFUNCTION(BlueprintCallable, Category = "Time Events")
    void BroadcastTimeEvents();

    // === 個別タスク処理 ===

    // 「全て」モード処理
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessAllModeTask(int32 TeamIndex);

    // 特定タスク処理
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessSpecificTask(int32 TeamIndex, ETaskType TaskType);

    // 採集タスク専用処理
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessGatheringTask(int32 TeamIndex);

    // 中断不可能アクション監視
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void MonitorLockedAction(int32 TeamIndex);

    // 戦闘終了後処理
    UFUNCTION(BlueprintCallable, Category = "Task Processing")
    void ProcessPostCombatTask(int32 TeamIndex);

    // === シンプルなターンベースロジック ===
    
    // 現在地を取得
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    FString GetCurrentLocation(int32 TeamIndex) const;
    
    // 現在地でアイテム採集が可能かチェック
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    bool CanExecuteGatheringAt(const FString& ItemId, const FString& Location) const;
    
    // アイテムが採集可能な場所を検索
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    FString FindLocationForItem(const FString& ItemId) const;
    
    // 採集実行
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    void ExecuteGathering(int32 TeamIndex, const FString& ItemId);
    
    // 指定場所への1ターン分移動実行
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    void ExecuteMovementStep(int32 TeamIndex, const FString& TargetLocation);
    
    // 拠点への帰還処理
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    void ProcessReturnToBase(int32 TeamIndex);
    
    // 場所の拠点からの距離取得
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    int32 GetLocationDistance(const FString& LocationId) const;
    
    // チーム移動速度取得
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    int32 GetTeamMovementSpeed(int32 TeamIndex) const;
    
    // チームが次の採集でアイテムを運べるかチェック
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    bool CanTeamCarryNextGather(int32 TeamIndex, const FString& ItemId);
    
    // 自動荷下ろし処理
    UFUNCTION(BlueprintCallable, Category = "Simple Logic")
    void AutoUnloadResourceItems(int32 TeamIndex);

    // === タスク切り替えロジック ===

    // タスク中断・切り替え
    UFUNCTION(BlueprintCallable, Category = "Task Switching")
    bool InterruptAndSwitchTask(int32 TeamIndex);

    // タスク実行開始
    UFUNCTION(BlueprintCallable, Category = "Task Switching")
    void StartTaskExecution(int32 TeamIndex, const FGlobalTask& Task);

    // チームを待機状態にセット
    UFUNCTION(BlueprintCallable, Category = "Task Switching")
    void SetTeamToIdle(int32 TeamIndex);

    // 遅延タスク切り替え処理
    UFUNCTION(BlueprintCallable, Category = "Task Switching")
    void ProcessPendingTaskSwitches();

    // === 戦闘終了イベントハンドラー ===

    // 戦闘終了イベント処理
    UFUNCTION()
    void OnCombatEndedHandler(int32 TeamIndex);

    // === コンポーネント登録 ===

    // タスクマネージャー登録
    UFUNCTION(BlueprintCallable, Category = "Component Setup")
    void RegisterTaskManager(UTaskManagerComponent* InTaskManager);
    
    // チームコンポーネント登録
    UFUNCTION(BlueprintCallable, Category = "Component Setup")
    void RegisterTeamComponent(UTeamComponent* InTeamComponent);

    // 採集コンポーネント登録
    UFUNCTION(BlueprintCallable, Category = "Component Setup")
    void RegisterGatheringComponent(UGatheringComponent* InGatheringComponent);

    UFUNCTION(BlueprintCallable, Category = "Component Setup")
    void RegisterMovementComponent(ULocationMovementComponent* InMovementComponent);

    // 登録済みコンポーネントクリア
    UFUNCTION(BlueprintCallable, Category = "Component Setup")
    void ClearRegisteredComponents();
    
    // 移動完了イベントハンドラー
    UFUNCTION()
    void OnMovementCompletedHandler(int32 TeamIndex, const FString& ArrivedLocation);

    // === 時間関連ユーティリティ ===

    // 現在のゲーム時間取得
    UFUNCTION(BlueprintPure, Category = "Time Utils")
    float GetCurrentGameTime() const { return CurrentGameTime; }

    // 経過時間計算
    UFUNCTION(BlueprintPure, Category = "Time Utils")
    float GetElapsedTime(float StartTime) const;

    // 時間を進める（デバッグ用）
    UFUNCTION(BlueprintCallable, Category = "Time Utils")
    void AdvanceTime(float Hours);

    // === 安全性確保機能 ===

    // 安全な時間更新処理
    void ProcessTimeUpdateSafe();

    // 安全なチームタスク処理
    void ProcessTeamTaskSafe(int32 TeamIndex);

    // 安全な戦闘監視
    void MonitorCombatSafe(int32 TeamIndex);

    // 安全な通常タスク処理
    void ProcessNormalTaskSafe(int32 TeamIndex);

    // === 検証・デバッグ ===

    // コンポーネント参照の有効性チェック
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool AreReferencesValid() const;

    // 処理統計取得
    UFUNCTION(BlueprintPure, Category = "Debug")
    FString GetProcessingStats() const;

    // デバッグ情報出力
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void PrintDebugInfo() const;

public:
    // === イベントディスパッチャー ===

    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnTimeSystemStarted OnTimeSystemStarted;
    
    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnTimeSystemStopped OnTimeSystemStopped;
    
    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnHourPassed OnHourPassed;

private:
    // === 内部ヘルパー関数 ===

    // タイマー設定
    void SetupTimer();

    // タイマークリア
    void ClearTimer();

    // リソース要件チェック
    bool CheckResourceRequirements(int32 TeamIndex, const FTeamTask& Task);

    // チームの有効性チェック
    bool IsValidTeam(int32 TeamIndex) const;

    // ログ出力
    void LogTimeOperation(const FString& Operation) const;

    // エラーログ出力
    void LogTimeError(const FString& ErrorMessage) const;

    // 統計情報
    UPROPERTY()
    int32 TotalUpdatesProcessed = 0;

    UPROPERTY()
    int32 TaskSwitchesPerformed = 0;

    UPROPERTY()
    float TotalProcessingTime = 0.0f;
};