#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Types/CharacterTypes.h"
#include "../Types/CombatTypes.h"
#include "CombatService.generated.h"

// Forward declarations
class AC_IdleCharacter;
class UCombatComponent;
class UBattleSystemManager;

/**
 * 戦闘専門サービス - キャラクターの戦闘機能を統括管理
 * 既存のCombatComponentロジックを移植し、自律的キャラクターに戦闘機能を提供
 */
UCLASS()
class UE_IDLE_API UCombatService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ===========================================
    // USubsystem interface
    // ===========================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ===========================================
    // 戦闘実行機能（既存機能の移植）
    // ===========================================
    
    /**
     * キャラクターの戦闘を開始（既存システムの移植）
     * @param Character 戦闘を行うキャラクター
     * @param Location 戦闘場所ID
     * @return 戦闘開始に成功した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    bool InitiateCombat(AC_IdleCharacter* Character, const FString& Location);

    /**
     * キャラクターが戦闘中かどうかチェック（既存システムの移植）
     * @param Character 対象キャラクター
     * @return 戦闘中ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    bool IsCharacterInCombat(AC_IdleCharacter* Character);

    /**
     * 戦闘進行状況を処理（ターンベース処理用）
     * @param Character 対象キャラクター
     * @return 戦闘完了した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    bool ProcessCombatProgress(AC_IdleCharacter* Character);

    /**
     * 戦闘を強制終了
     * @param Character 対象キャラクター
     * @param bVictory 勝利の場合true、敗北の場合false
     * @return 終了に成功した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    bool EndCombat(AC_IdleCharacter* Character, bool bVictory = true);

    // ===========================================
    // 戦闘能力・判定計算
    // ===========================================
    
    /**
     * キャラクターの戦闘力を計算
     * @param Character 対象キャラクター
     * @return 総合戦闘力値
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    float CalculateCombatPower(AC_IdleCharacter* Character);

    /**
     * 戦闘の推定継続時間を計算
     * @param Character 対象キャラクター
     * @param Location 戦闘場所
     * @return 推定時間（秒）
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    float EstimateCombatDuration(AC_IdleCharacter* Character, const FString& Location);

    /**
     * キャラクターが戦闘可能かチェック
     * @param Character 対象キャラクター
     * @param Location 戦闘場所
     * @return 戦闘可能ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    bool CanCharacterFight(AC_IdleCharacter* Character, const FString& Location);

    /**
     * 最適な戦闘行動を決定（将来の個性化用）
     * @param Character 対象キャラクター
     * @param Situation 戦闘状況
     * @return 推奨戦闘行動
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    FCharacterAction DecideOptimalCombatAction(AC_IdleCharacter* Character, const FCharacterSituation& Situation);

    // ===========================================
    // 戦闘場所・敵情報
    // ===========================================
    
    /**
     * 戦闘可能な場所を検索
     * @return 戦闘可能な場所IDの配列
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    TArray<FString> GetCombatLocations();

    /**
     * 場所の戦闘危険度を取得
     * @param Location 場所ID
     * @return 危険度レベル（1-10、高いほど危険）
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    int32 GetLocationDangerLevel(const FString& Location);

    /**
     * 場所の敵の種類を取得
     * @param Location 場所ID
     * @return 敵の種類の配列
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    TArray<FString> GetEnemyTypesAt(const FString& Location);

    /**
     * 推定戦利品を取得
     * @param Location 戦闘場所
     * @param Character 対象キャラクター
     * @return 期待できる戦利品ID配列
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    TArray<FString> GetExpectedLoot(const FString& Location, AC_IdleCharacter* Character);

    // ===========================================
    // デバッグ・統計情報
    // ===========================================
    
    /**
     * 戦闘統計情報を取得
     * @return 戦闘回数、勝率、平均継続時間等の情報
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    FString GetCombatStatistics();

    /**
     * キャラクターの戦闘履歴を取得
     * @param Character 対象キャラクター
     * @return 戦闘履歴情報
     */
    UFUNCTION(BlueprintCallable, Category = "Combat Service")
    FString GetCharacterCombatHistory(AC_IdleCharacter* Character);

protected:
    // ===========================================
    // 内部実装（既存ロジックの移植）
    // ===========================================
    
    /**
     * 既存のCombatComponentロジックを実行
     * @param Character 対象キャラクター
     * @param Location 戦闘場所
     * @return 実行成功ならtrue
     */
    bool ExecuteExistingCombatLogic(AC_IdleCharacter* Character, const FString& Location);

    /**
     * 既存の戦闘状態チェックを実行
     * @param Character 対象キャラクター
     * @return 戦闘中ならtrue
     */
    bool CheckExistingCombatState(AC_IdleCharacter* Character);

    /**
     * キャラクターが所属するチームインデックスを取得
     * @param Character 対象キャラクター
     * @return チームインデックス（見つからない場合は-1）
     */
    int32 GetCharacterTeamIndex(AC_IdleCharacter* Character);

    // ===========================================
    // 戦闘計算ヘルパー
    // ===========================================
    
    /**
     * 基本戦闘能力を計算（キャラクター能力ベース）
     * @param Character 対象キャラクター
     * @return 基本戦闘力値
     */
    float CalculateBaseCombatPower(AC_IdleCharacter* Character);

    /**
     * 場所別戦闘修正を適用
     * @param BasePower 基本戦闘力
     * @param Location 戦闘場所
     * @return 修正後戦闘力
     */
    float ApplyLocationCombatModifier(float BasePower, const FString& Location);

    /**
     * 敵の強さを推定
     * @param Location 戦闘場所
     * @return 敵の推定戦闘力
     */
    float EstimateEnemyStrength(const FString& Location);

    // ===========================================
    // 参照管理
    // ===========================================
    
    /**
     * CombatComponentの参照を取得
     * @return CombatComponentの参照（nullptrの場合あり）
     */
    UCombatComponent* GetCombatComponent();

    /**
     * BattleSystemManagerの参照を取得
     * @return BattleSystemManagerの参照（nullptrの場合あり）
     */
    UBattleSystemManager* GetBattleSystemManager();

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
     * 処理された戦闘リクエスト数
     */
    int32 TotalCombatRequests;

    /**
     * 成功した戦闘開始数
     */
    int32 SuccessfulCombatStarts;

    /**
     * 勝利数
     */
    int32 Victories;

    /**
     * 敗北数
     */
    int32 Defeats;

    /**
     * 累積戦闘時間
     */
    float TotalCombatTime;

    // ===========================================
    // 戦闘力キャッシュ
    // ===========================================
    
    /**
     * キャラクター別戦闘力キャッシュ
     */
    TMap<AC_IdleCharacter*, float> CombatPowerCache;

    /**
     * キャッシュの最終更新時間
     */
    float LastCombatCacheUpdate;

    /**
     * 戦闘力キャッシュ有効期間（秒）
     */
    static constexpr float CombatCacheValidDuration = 60.0f;

    // ===========================================
    // 場所・敵データ
    // ===========================================
    
    /**
     * 場所危険度マップ
     */
    TMap<FString, int32> LocationDangerMap;

    /**
     * 場所別敵情報マップ
     */
    TMap<FString, TArray<FString>> LocationEnemiesMap;

    // ===========================================
    // 内部ヘルパー関数
    // ===========================================
    
    /**
     * 統計情報をリセット
     */
    void ResetStatistics();

    /**
     * 戦闘力キャッシュを更新
     */
    void UpdateCombatCache();

    /**
     * 戦闘力キャッシュが有効かチェック
     * @return キャッシュが有効ならtrue
     */
    bool IsCombatCacheValid() const;

    /**
     * 場所・敵データを初期化
     */
    void InitializeLocationEnemyData();
};