#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Types/CharacterTypes.h"
#include "MovementService.generated.h"

// Forward declarations
class AC_IdleCharacter;
class ULocationMovementComponent;

/**
 * 移動専門サービス - キャラクターの移動機能を統括管理
 * 既存のLocationMovementComponentロジックを移植し、自律的キャラクターに移動機能を提供
 */
UCLASS()
class UE_IDLE_API UMovementService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ===========================================
    // USubsystem interface
    // ===========================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ===========================================
    // 移動実行機能（既存機能の移植）
    // ===========================================
    
    /**
     * キャラクターを指定場所に移動させる（既存システムの移植）
     * @param Character 移動対象のキャラクター
     * @param TargetLocation 目標場所ID ("base", "plains" etc.)
     * @return 移動開始に成功した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    bool MoveCharacterToLocation(AC_IdleCharacter* Character, const FString& TargetLocation);

    /**
     * 移動時間を計算（既存システムの移植）
     * @param Character 移動するキャラクター
     * @param TargetLocation 目標場所ID
     * @return 推定移動時間（秒）
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    float CalculateMovementTime(AC_IdleCharacter* Character, const FString& TargetLocation);

    /**
     * キャラクターの現在位置を取得（既存システムの移植）
     * @param Character 対象キャラクター
     * @return 現在の場所ID
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    FString GetCharacterCurrentLocation(AC_IdleCharacter* Character);

    /**
     * キャラクターが移動中かどうかチェック
     * @param Character 対象キャラクター
     * @return 移動中ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    bool IsCharacterMoving(AC_IdleCharacter* Character);

    /**
     * 移動進行状況を処理（ターンベース処理用）
     * @param Character 対象キャラクター
     * @return 移動完了した場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    bool ProcessMovementProgress(AC_IdleCharacter* Character);

    /**
     * 移動完了状況をチェック（状態を変更しない）
     * @param Character 対象キャラクター
     * @return 移動完了済みの場合true
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    bool CheckMovementProgress(AC_IdleCharacter* Character);

    // ===========================================
    // 場所情報・距離計算
    // ===========================================
    
    /**
     * 2つの場所間の距離を取得
     * @param FromLocation 出発地ID
     * @param ToLocation 目的地ID
     * @return 距離（ゲーム内単位）
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    float GetDistanceBetweenLocations(const FString& FromLocation, const FString& ToLocation);

    /**
     * キャラクターの拠点からの距離を取得
     * @param Character 対象キャラクター
     * @return 拠点からの距離
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    float GetDistanceFromBase(AC_IdleCharacter* Character);

    /**
     * 指定場所が移動可能かチェック
     * @param LocationId 場所ID
     * @return 移動可能ならtrue
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    bool IsLocationAccessible(const FString& LocationId);

    // ===========================================
    // デバッグ・情報取得
    // ===========================================
    
    /**
     * 全ての利用可能な場所IDを取得
     * @return 場所IDの配列
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    TArray<FString> GetAllAvailableLocations();

    /**
     * 移動状況の統計情報を取得
     * @return 移動処理数、平均移動時間等の情報
     */
    UFUNCTION(BlueprintCallable, Category = "Movement Service")
    FString GetMovementStatistics();

protected:
    // ===========================================
    // 内部実装（既存ロジックの移植）
    // ===========================================
    
    /**
     * 既存のLocationMovementComponentロジックを実行
     * @param Character 対象キャラクター
     * @param TargetLocation 目標場所
     * @return 実行成功ならtrue
     */
    bool ExecuteExistingMovementLogic(AC_IdleCharacter* Character, const FString& TargetLocation);

    /**
     * 既存の移動時間計算を実行
     * @param FromLocation 出発地
     * @param ToLocation 目的地  
     * @return 移動時間
     */
    float GetExistingMovementTime(const FString& FromLocation, const FString& ToLocation);

    /**
     * キャラクターが所属するチームインデックスを取得
     * @param Character 対象キャラクター
     * @return チームインデックス（見つからない場合は-1）
     */
    int32 GetCharacterTeamIndex(AC_IdleCharacter* Character);

    // ===========================================
    // 参照管理
    // ===========================================
    
    /**
     * LocationMovementComponentの参照を取得
     * @return LocationMovementComponentの参照（nullptrの場合あり）
     */
    ULocationMovementComponent* GetLocationMovementComponent();

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
     * 処理された移動リクエスト数
     */
    int32 TotalMovementRequests;

    /**
     * 成功した移動数
     */
    int32 SuccessfulMovements;

    /**
     * 累積移動時間
     */
    float TotalMovementTime;

    // ===========================================
    // キャッシュ管理
    // ===========================================
    
    /**
     * 場所間距離のキャッシュ
     */
    TMap<FString, float> DistanceCache;

    /**
     * キャッシュの最終更新時間
     */
    float LastCacheUpdate;

    /**
     * キャッシュ有効期間（秒）
     */
    static constexpr float CacheValidDuration = 60.0f;

    // ===========================================
    // 内部ヘルパー関数
    // ===========================================
    
    /**
     * 統計情報をリセット
     */
    void ResetStatistics();

    /**
     * 距離キャッシュを更新
     */
    void UpdateDistanceCache();

    /**
     * キャッシュが有効かチェック
     * @return キャッシュが有効ならtrue
     */
    bool IsCacheValid() const;
};