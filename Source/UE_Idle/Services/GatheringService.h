#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Types/CharacterTypes.h"
#include "GatheringService.generated.h"

// Forward declarations
class AC_IdleCharacter;
class UGatheringComponent;
class UTeamComponent;

/**
 * 採集専門サービス - キャラクターの採集機能を統括管理
 * 既存のGatheringComponentロジックを移植し、自律的キャラクターに採集機能を提供
 */
UCLASS()
class UE_IDLE_API UGatheringService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ===========================================
    // USubsystem interface
    // ===========================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ===========================================
    // 採集実行機能（既存機能の移植）
    // ===========================================
    
    /**
     * キャラクターが採集を実行（既存システムの移植）
     * @param Character 採集を行うキャラクター
     * @param TargetItem 採集対象アイテムID
     * @return 採集開始に成功した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    bool ExecuteGathering(AC_IdleCharacter* Character, const FString& TargetItem);

    /**
     * 指定場所で採集可能なアイテムを取得（既存システムの移植）
     * @param Location 場所ID
     * @return 採集可能アイテムIDの配列
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    TArray<FString> GetGatherableItemsAt(const FString& Location);

    /**
     * キャラクターが採集中かどうかチェック
     * @param Character 対象キャラクター
     * @return 採集中ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    bool IsCharacterGathering(AC_IdleCharacter* Character);

    /**
     * 採集進行状況を処理（ターンベース処理用）
     * @param Character 対象キャラクター
     * @return 採集完了した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    bool ProcessGatheringProgress(AC_IdleCharacter* Character);

    // ===========================================
    // 採集効率・能力計算
    // ===========================================
    
    /**
     * キャラクターの採集効率を計算
     * @param Character 対象キャラクター
     * @param ItemType 採集アイテムタイプ
     * @return 採集効率値
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    float CalculateGatheringEfficiency(AC_IdleCharacter* Character, const FString& ItemType);

    /**
     * 採集に必要な推定時間を計算
     * @param Character 対象キャラクター
     * @param ItemType 採集アイテムタイプ
     * @param Quantity 採集数量
     * @return 推定時間（秒）
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    float EstimateGatheringTime(AC_IdleCharacter* Character, const FString& ItemType, int32 Quantity = 1);

    /**
     * キャラクターが特定アイテムを採集可能かチェック
     * @param Character 対象キャラクター
     * @param ItemType アイテムタイプ
     * @param Location 採集場所
     * @return 採集可能ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    bool CanCharacterGatherItem(AC_IdleCharacter* Character, const FString& ItemType, const FString& Location);

    // ===========================================
    // 場所・アイテム情報
    // ===========================================
    
    /**
     * 指定アイテムが採集可能な場所を検索
     * @param ItemType アイテムタイプ
     * @return 採集可能な場所IDの配列
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    TArray<FString> FindLocationsForItem(const FString& ItemType);

    /**
     * 場所の採集難易度を取得
     * @param Location 場所ID
     * @return 難易度レベル（1-10、高いほど困難）
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    int32 GetLocationDifficultyLevel(const FString& Location);

    /**
     * アイテムの希少度を取得
     * @param ItemType アイテムタイプ
     * @return 希少度レベル（1-10、高いほど希少）
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    int32 GetItemRarityLevel(const FString& ItemType);

    // ===========================================
    // デバッグ・統計情報
    // ===========================================
    
    /**
     * 採集統計情報を取得
     * @return 採集回数、成功率、平均効率等の情報
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    FString GetGatheringStatistics();

    /**
     * 全ての採集可能アイテムを取得
     * @return アイテムIDの配列
     */
    UFUNCTION(BlueprintCallable, Category = "Gathering Service")
    TArray<FString> GetAllGatherableItems();

protected:
    // ===========================================
    // 内部実装（既存ロジックの移植）
    // ===========================================
    
    /**
     * 既存のGatheringComponentロジックを実行
     * @param Character 対象キャラクター
     * @param ItemType アイテムタイプ
     * @return 実行成功ならtrue
     */
    bool ExecuteExistingGatheringLogic(AC_IdleCharacter* Character, const FString& ItemType);

    /**
     * 既存の採集可能アイテム取得ロジックを実行
     * @param Location 場所ID
     * @return 採集可能アイテムIDの配列
     */
    TArray<FString> GetExistingGatherableItems(const FString& Location);

    /**
     * キャラクターが所属するチームインデックスを取得
     * @param Character 対象キャラクター
     * @return チームインデックス（見つからない場合は-1）
     */
    int32 GetCharacterTeamIndex(AC_IdleCharacter* Character);

    // ===========================================
    // 効率計算ヘルパー
    // ===========================================
    
    /**
     * 基本採集効率を計算（キャラクター能力ベース）
     * @param Character 対象キャラクター
     * @return 基本効率値
     */
    float CalculateBaseGatheringPower(AC_IdleCharacter* Character);

    /**
     * アイテム別効率修正を適用
     * @param BaseEfficiency 基本効率
     * @param ItemType アイテムタイプ
     * @return 修正後効率
     */
    float ApplyItemTypeModifier(float BaseEfficiency, const FString& ItemType);

    /**
     * 場所別効率修正を適用
     * @param BaseEfficiency 基本効率
     * @param Location 場所ID
     * @return 修正後効率
     */
    float ApplyLocationModifier(float BaseEfficiency, const FString& Location);

    // ===========================================
    // 参照管理
    // ===========================================
    
    /**
     * GatheringComponentの参照を取得
     * @return GatheringComponentの参照（nullptrの場合あり）
     */
    UGatheringComponent* GetGatheringComponent();

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
     * 処理された採集リクエスト数
     */
    int32 TotalGatheringRequests;

    /**
     * 成功した採集数
     */
    int32 SuccessfulGatherings;

    /**
     * 累積採集時間
     */
    float TotalGatheringTime;

    /**
     * 累積採集効率
     */
    float TotalEfficiency;

    // ===========================================
    // 効率キャッシュ
    // ===========================================
    
    /**
     * キャラクター別効率キャッシュ
     */
    TMap<AC_IdleCharacter*, float> EfficiencyCache;

    /**
     * キャッシュの最終更新時間
     */
    float LastEfficiencyCacheUpdate;

    /**
     * 効率キャッシュ有効期間（秒）
     */
    static constexpr float EfficiencyCacheValidDuration = 30.0f;

    // ===========================================
    // アイテム・場所データ
    // ===========================================
    
    /**
     * アイテム希少度マップ
     */
    TMap<FString, int32> ItemRarityMap;

    /**
     * 場所難易度マップ
     */
    TMap<FString, int32> LocationDifficultyMap;

    // ===========================================
    // 内部ヘルパー関数
    // ===========================================
    
    /**
     * 統計情報をリセット
     */
    void ResetStatistics();

    /**
     * 効率キャッシュを更新
     */
    void UpdateEfficiencyCache();

    /**
     * 効率キャッシュが有効かチェック
     * @return キャッシュが有効ならtrue
     */
    bool IsEfficiencyCacheValid() const;

    /**
     * アイテム・場所データを初期化
     */
    void InitializeItemLocationData();
};