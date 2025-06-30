#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Types/CharacterTypes.h"
#include "../Types/TaskTypes.h"
#include "../Types/TeamTypes.h"
#include "TaskInformationService.generated.h"

// Forward declarations
class AC_IdleCharacter;
class UTaskManagerComponent;
class UTeamComponent;

/**
 * タスク情報提供サービス - キャラクターにタスク関連情報を提供
 * TaskManagerComponentから情報を取得し、自律的キャラクターの判断をサポート
 * 管理ではなく情報提供のみを行う
 */
UCLASS()
class UE_IDLE_API UTaskInformationService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ===========================================
    // USubsystem interface
    // ===========================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ===========================================
    // 利用可能タスク情報提供
    // ===========================================
    
    /**
     * キャラクターが利用可能なタスクオプションを取得
     * @param Character 対象キャラクター
     * @return 利用可能なタスクオプションの配列
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    TArray<FTaskOption> GetAvailableTaskOptions(AC_IdleCharacter* Character);

    /**
     * 指定タスクの詳細情報を取得
     * @param TaskId タスクID
     * @return タスク詳細情報
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    FGlobalTask GetTaskDetails(const FString& TaskId);

    /**
     * 指定場所でタスクが実行可能かチェック
     * @param TaskId タスクID
     * @param Location 場所ID
     * @return 実行可能ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    bool IsTaskPossibleAt(const FString& TaskId, const FString& Location);

    /**
     * チームが実行可能なタスクを優先度順で取得
     * @param TeamIndex チームインデックス
     * @return 実行可能タスクの配列（優先度順）
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    TArray<FGlobalTask> GetExecutableTasksForTeam(int32 TeamIndex);

    // ===========================================
    // タスク実行条件チェック
    // ===========================================
    
    /**
     * キャラクターがタスクを実行可能かチェック
     * @param Character 対象キャラクター
     * @param Task タスク情報
     * @return 実行可能ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    bool CanCharacterExecuteTask(AC_IdleCharacter* Character, const FGlobalTask& Task);

    /**
     * タスクのリソース要件をチェック
     * @param Task 対象タスク
     * @param TeamIndex チームインデックス（省略可）
     * @return 要件を満たしているならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    bool CheckTaskResourceRequirements(const FGlobalTask& Task, int32 TeamIndex = -1);

    /**
     * タスクに必要なスキルをチェック
     * @param Character 対象キャラクター
     * @param Task 対象タスク
     * @return 必要スキルを持っているならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    bool CheckTaskSkillRequirements(AC_IdleCharacter* Character, const FGlobalTask& Task);

    /**
     * タスクの推定完了時間を計算
     * @param Character 対象キャラクター
     * @param Task 対象タスク
     * @return 推定完了時間（秒）
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    float EstimateTaskCompletionTime(AC_IdleCharacter* Character, const FGlobalTask& Task);

    // ===========================================
    // タスク優先度・効率情報
    // ===========================================
    
    /**
     * 現在最も優先度の高いタスクを取得
     * @param TeamIndex チームインデックス（省略時は全体タスク）
     * @return 最高優先度タスク
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    FGlobalTask GetHighestPriorityTask(int32 TeamIndex = -1);

    /**
     * キャラクターにとって最も効率的なタスクを取得
     * @param Character 対象キャラクター
     * @param AvailableTasks 利用可能タスク配列
     * @return 最効率タスク
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    FGlobalTask GetMostEfficientTask(AC_IdleCharacter* Character, const TArray<FGlobalTask>& AvailableTasks);

    /**
     * タスクの進行状況を取得
     * @param TaskId タスクID
     * @return 進行率（0.0-1.0）
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    float GetTaskProgress(const FString& TaskId);

    /**
     * タスクが完了しているかチェック
     * @param TaskId タスクID
     * @return 完了済みならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    bool IsTaskCompleted(const FString& TaskId);

    // ===========================================
    // 場所・アイテム関連情報
    // ===========================================
    
    /**
     * 指定アイテムの目標採集量を取得
     * @param TeamIndex チームインデックス
     * @param ItemId アイテムID
     * @return 目標採集量（見つからない場合は0）
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    int32 GetTargetGatheringQuantity(int32 TeamIndex, const FString& ItemId);

    /**
     * アイテムが採集可能な場所を取得
     * @param ItemId アイテムID
     * @return 採集可能場所の配列
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    TArray<FString> GetGatheringLocationsForItem(const FString& ItemId);

    /**
     * 現在のリソース在庫量を取得
     * @param ResourceId リソースID
     * @param bIncludeTeamInventories チームインベントリも含むか
     * @return 総在庫量
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    int32 GetCurrentResourceAmount(const FString& ResourceId, bool bIncludeTeamInventories = true);

    // ===========================================
    // デバッグ・統計情報
    // ===========================================
    
    /**
     * 全グローバルタスクの一覧を取得
     * @return グローバルタスク配列
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    TArray<FGlobalTask> GetAllGlobalTasks();

    /**
     * チーム固有タスクの一覧を取得
     * @param TeamIndex チームインデックス
     * @return チームタスク配列
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    TArray<FTeamTask> GetTeamTasks(int32 TeamIndex);

    /**
     * タスク情報の統計を取得
     * @return 統計情報文字列
     */
    UFUNCTION(BlueprintCallable, Category = "Task Information")
    FString GetTaskInformationStatistics();

protected:
    // ===========================================
    // 内部実装ヘルパー
    // ===========================================
    
    /**
     * 指定チームの現在目標アイテムを取得（TaskManagerから）
     * @param TeamIndex チームインデックス
     * @param LocationId 現在場所ID
     * @return 目標アイテムID
     */
    FString GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId);

    /**
     * タスクタイプに応じた実行可能性をチェック
     * @param TaskType タスクタイプ
     * @param Character 対象キャラクター
     * @param Location 実行場所
     * @return 実行可能ならtrue
     */
    bool CheckTaskTypeRequirements(ETaskType TaskType, AC_IdleCharacter* Character, const FString& Location);

    /**
     * キャラクターが所属するチームインデックスを取得
     * @param Character 対象キャラクター
     * @return チームインデックス（見つからない場合は-1）
     */
    int32 GetCharacterTeamIndex(AC_IdleCharacter* Character);

    /**
     * キャラクターの現在位置を取得
     * @param Character 対象キャラクター
     * @return 現在位置ID
     */
    FString GetCharacterCurrentLocation(AC_IdleCharacter* Character);

    // ===========================================
    // 効率・能力計算ヘルパー
    // ===========================================
    
    /**
     * タスクに対するキャラクターの効率を計算
     * @param Character 対象キャラクター
     * @param Task 対象タスク
     * @return 効率値（1.0が標準）
     */
    float CalculateTaskEfficiency(AC_IdleCharacter* Character, const FGlobalTask& Task);

    /**
     * キャラクターのタスク関連能力を取得
     * @param Character 対象キャラクター
     * @param TaskType タスクタイプ
     * @return 能力値
     */
    float GetCharacterTaskAbility(AC_IdleCharacter* Character, ETaskType TaskType);

    // ===========================================
    // 参照管理
    // ===========================================
    
    /**
     * TaskManagerComponentの参照を取得
     * @return TaskManagerComponentの参照（nullptrの場合あり）
     */
    UTaskManagerComponent* GetTaskManagerComponent();

    /**
     * TeamComponentの参照を取得
     * @return TeamComponentの参照（nullptrの場合あり）
     */
    UTeamComponent* GetTeamComponent();

    /**
     * 必要な参照が有効かチェック
     * @return 全ての必要な参照が有効ならtrue
     */
    bool AreReferencesValid();

private:
    // ===========================================
    // 統計情報
    // ===========================================
    
    /**
     * 情報リクエスト数
     */
    int32 TotalInformationRequests;

    /**
     * キャッシュヒット数
     */
    int32 CacheHits;

    // ===========================================
    // 情報キャッシュ
    // ===========================================
    
    /**
     * タスク詳細キャッシュ
     */
    TMap<FString, FGlobalTask> TaskDetailsCache;

    /**
     * 実行可能性キャッシュ
     */
    TMap<FString, bool> ExecutabilityCache;

    /**
     * キャッシュの最終更新時間
     */
    float LastCacheUpdate;

    /**
     * キャッシュ有効期間（秒）
     */
    static constexpr float CacheValidDuration = 5.0f;

    // ===========================================
    // 内部ヘルパー関数
    // ===========================================
    
    /**
     * 統計情報をリセット
     */
    void ResetStatistics();

    /**
     * キャッシュを更新
     */
    void UpdateCache();

    /**
     * キャッシュが有効かチェック
     * @return キャッシュが有効ならtrue
     */
    bool IsCacheValid() const;

    /**
     * キャッシュキーを生成
     * @param Prefix プレフィックス
     * @param Params パラメータ配列
     * @return キャッシュキー
     */
    FString GenerateCacheKey(const FString& Prefix, const TArray<FString>& Params);
};