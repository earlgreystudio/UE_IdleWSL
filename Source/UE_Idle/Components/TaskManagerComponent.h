#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/TaskTypes.h"
#include "../Types/TeamTypes.h"
#include "TaskManagerComponent.generated.h"

// Forward declarations
class UInventoryComponent;
class UTeamComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UTaskManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTaskManagerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // === 全体タスク管理 ===

    // 全体タスクリスト（最大20個）
    UPROPERTY(BlueprintReadWrite, Category = "Task", meta = (AllowPrivateAccess = "true"))
    TArray<FGlobalTask> GlobalTasks;
    
    // タスク管理設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Settings")
    int32 MaxGlobalTasks = 20;

    // 処理中フラグ（安全性確保用）
    UPROPERTY(BlueprintReadOnly, Category = "Task State")
    bool bProcessingTasks = false;

    // === 参照コンポーネント ===

    // グローバルインベントリへの参照
    UPROPERTY()
    UInventoryComponent* GlobalInventoryRef = nullptr;

    // チームコンポーネントへの参照
    UPROPERTY()
    UTeamComponent* TeamComponentRef = nullptr;

public:
    // === タスク管理機能 ===

    // タスク追加
    UFUNCTION(BlueprintCallable, Category = "Task")
    int32 AddGlobalTask(const FGlobalTask& NewTask);

    // タスク削除
    UFUNCTION(BlueprintCallable, Category = "Task")
    bool RemoveGlobalTask(int32 TaskIndex);

    // タスク優先度更新
    UFUNCTION(BlueprintCallable, Category = "Task")
    bool UpdateTaskPriority(int32 TaskIndex, int32 NewPriority);

    // タスク目標量更新
    UFUNCTION(BlueprintCallable, Category = "Task")
    bool UpdateTaskTargetQuantity(int32 TaskIndex, int32 NewTargetQuantity);

    // タスク完了
    UFUNCTION(BlueprintCallable, Category = "Task")
    bool CompleteTask(int32 TaskIndex);

    // 優先度順タスク取得
    UFUNCTION(BlueprintCallable, Category = "Task")
    TArray<FGlobalTask> GetGlobalTasksByPriority() const;

    // 全タスク取得
    UFUNCTION(BlueprintPure, Category = "Task")
    const TArray<FGlobalTask>& GetGlobalTasks() const { return GlobalTasks; }

    // 特定タスク取得
    UFUNCTION(BlueprintPure, Category = "Task")
    FGlobalTask GetGlobalTask(int32 TaskIndex) const;

    // タスク数取得
    UFUNCTION(BlueprintPure, Category = "Task")
    int32 GetTaskCount() const { return GlobalTasks.Num(); }

    // === 優先度管理機能 ===

    // タスク優先度入れ替え
    UFUNCTION(BlueprintCallable, Category = "Task Priority")
    bool SwapTaskPriority(int32 TaskIndex1, int32 TaskIndex2);

    // タスクを上に移動
    UFUNCTION(BlueprintCallable, Category = "Task Priority")
    bool MoveTaskUp(int32 TaskIndex);

    // タスクを下に移動
    UFUNCTION(BlueprintCallable, Category = "Task Priority")
    bool MoveTaskDown(int32 TaskIndex);

    // 優先度の再計算（連番になるよう調整）
    UFUNCTION(BlueprintCallable, Category = "Task Priority")
    void RecalculatePriorities();

    // === 「全て」モード用タスク選択 ===

    // 次の実行可能タスク取得
    UFUNCTION(BlueprintCallable, Category = "Task Selection")
    FGlobalTask GetNextAvailableTask(int32 TeamIndex) const;

    // チームがタスクを実行可能かチェック
    UFUNCTION(BlueprintCallable, Category = "Task Selection")
    bool CanTeamExecuteTask(int32 TeamIndex, const FGlobalTask& Task) const;

    // === リソース監視・判定 ===

    // リソース要件チェック
    UFUNCTION(BlueprintCallable, Category = "Resource")
    bool CheckResourceRequirements(const FTeamTask& Task) const;

    // 総リソース量取得
    UFUNCTION(BlueprintCallable, Category = "Resource")
    int32 GetTotalResourceAmount(const FString& ResourceId) const;

    // リソース条件満たすかチェック（GlobalTask用）
    UFUNCTION(BlueprintCallable, Category = "Resource")
    bool CheckGlobalTaskRequirements(const FGlobalTask& Task, int32 TeamIndex) const;

    // === タスク完了処理 ===

    // タスク完了処理
    UFUNCTION(BlueprintCallable, Category = "Task Completion")
    void ProcessTaskCompletion(const FString& TaskId, int32 CompletedAmount);

    // タスク進行更新
    UFUNCTION(BlueprintCallable, Category = "Task Completion")
    bool UpdateTaskProgress(const FString& TaskId, int32 ProgressAmount);

    // 完了タスクをクリア
    UFUNCTION(BlueprintCallable, Category = "Task Completion")
    void ClearCompletedTasks();

    // === 参照設定 ===

    // グローバルインベントリ参照設定
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetGlobalInventoryReference(UInventoryComponent* InventoryComponent);

    // チームコンポーネント参照設定
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetTeamComponentReference(UTeamComponent* TeamComponent);

    // === 採集継続判定機能 ===

    // 採集継続判定（特定チームが特定アイテムの採集を継続すべきか）
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    bool ShouldContinueGathering(int32 TeamIndex, const FString& ItemId) const;

    // 現在のアイテム利用可能数取得（拠点倉庫 + チームメンバー所持）
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    int32 GetCurrentItemAvailability(int32 TeamIndex, const FString& ItemId) const;

    // 採集関連のアクティブタスク検索
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    FGlobalTask FindActiveGatheringTask(const FString& ItemId) const;
    
    // 指定チームが採集すべき対象アイテムを決定（TimeManagerからの委譲）
    UFUNCTION(BlueprintCallable, Category = "Task Execution")
    FString GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId) const;
    
    // タスクが目標数量に達したかチェック
    UFUNCTION(BlueprintCallable, Category = "Task Execution")
    bool IsTaskCompleted(const FString& TaskId) const;
    
    // 指定チームが実行可能な採集タスクリストを取得
    UFUNCTION(BlueprintCallable, Category = "Task Execution")
    TArray<FGlobalTask> GetExecutableGatheringTasksAtLocation(int32 TeamIndex, const FString& LocationId) const;

    // === ユーティリティ ===

    // タスクの有効性チェック
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    bool IsValidTaskIndex(int32 TaskIndex) const;

    // タスクID検索
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    int32 FindTaskByID(const FString& TaskId) const;

    // 未完了タスク数取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    int32 GetIncompleteTaskCount() const;

    // 完了タスク数取得
    UFUNCTION(BlueprintPure, Category = "Task Utils")
    int32 GetCompletedTaskCount() const;

    // === タスク実行計画システム ===

    // チーム用の実行計画を作成（新しい委譲設計の中核）
    UFUNCTION(BlueprintCallable, Category = "Task Execution Plan")
    FTaskExecutionPlan CreateExecutionPlanForTeam(int32 TeamIndex, const FString& CurrentLocation, ETaskType CurrentTask);

private:
    // 各タスクタイプ別の実行計画作成（内部実装）
    FTaskExecutionPlan CreateGatheringExecutionPlan(int32 TeamIndex, const FString& CurrentLocation);
    FTaskExecutionPlan CreateAdventureExecutionPlan(int32 TeamIndex, const FString& CurrentLocation);
    FTaskExecutionPlan CreateAllModeExecutionPlan(int32 TeamIndex, const FString& CurrentLocation);

public:
    // === 安全性確保機能 ===

    // 処理中かチェック
    UFUNCTION(BlueprintPure, Category = "Safety")
    bool IsProcessingTasks() const { return bProcessingTasks; }

    // 安全な処理実行
    template<typename FunctionType>
    bool ExecuteSafely(FunctionType Function);

public:
    // === イベントディスパッチャー ===

    UPROPERTY(BlueprintAssignable, Category = "Task Events")
    FOnGlobalTaskAdded OnGlobalTaskAdded;
    
    UPROPERTY(BlueprintAssignable, Category = "Task Events")
    FOnGlobalTaskCompleted OnGlobalTaskCompleted;
    
    UPROPERTY(BlueprintAssignable, Category = "Task Events")
    FOnTaskPriorityChanged OnTaskPriorityChanged;

    UPROPERTY(BlueprintAssignable, Category = "Task Events")
    FOnGlobalTaskRemoved OnGlobalTaskRemoved;
    
    UPROPERTY(BlueprintAssignable, Category = "Task Events")
    FOnTaskQuantityUpdated OnTaskQuantityUpdated;

private:
    // === 内部ヘルパー関数 ===

    // タスクの整合性チェック
    bool ValidateTask(const FGlobalTask& Task) const;

    // 優先度の重複チェック
    bool HasDuplicatePriority(int32 Priority, int32 ExcludeIndex = -1) const;

    // 安全な配列アクセス
    bool IsValidArrayAccess(int32 Index) const;

    // ログ出力
    void LogTaskOperation(const FString& Operation, const FGlobalTask& Task) const;

    // エラーログ出力
    void LogError(const FString& ErrorMessage) const;

    // === タスクマッチング用ヘルパー関数 ===
    
    // メインマッチング関数：チームタスクに対応するグローバルタスクを探す
    FString FindMatchingGlobalTask(const FTeamTask& TeamTask, int32 TeamIndex, const FString& LocationId) const;
    
    // 各タスクタイプ専用マッチング関数
    FString FindMatchingGatheringTask(int32 TeamIndex, const FString& LocationId) const;
    FString FindMatchingAdventureTask(int32 TeamIndex, const FString& LocationId) const;
    FString FindMatchingConstructionTask(int32 TeamIndex) const;
    FString FindMatchingCookingTask(int32 TeamIndex) const;
    FString FindMatchingCraftingTask(int32 TeamIndex) const;

    // === 内部状態管理 ===
    
    // タスク処理中フラグ（再入防止）
    bool bIsProcessingTasks = false;
};