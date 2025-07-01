#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimeManagerComponent.generated.h"

// Forward declarations
class AC_IdleCharacter;

/**
 * Phase 3: 簡素化されたTimeManagerComponent
 * 唯一の責任：全キャラクターにターン開始通知を送ること
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UTimeManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimeManagerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // ===========================================
    // Phase 3: 簡素化された時間システム
    // ===========================================

    // 更新間隔（秒）- 1秒 = 1ターン
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autonomous Time System", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float TimeUpdateInterval = 1.0f;

    // === ゲーム速度制御 ===

    // ゲーム速度プリセット
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Speed Control")
    TMap<FString, float> GameSpeedPresets = {
        {"Slow", 2.0f},
        {"Normal", 1.0f}, 
        {"Fast", 0.5f},
        {"Ultra", 0.1f}
    };

    // 現在のゲーム速度設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Speed Control")
    FString CurrentGameSpeed = "Normal";

    // ポーズ状態
    UPROPERTY(BlueprintReadOnly, Category = "Game Speed Control")
    bool bGamePaused = false;

    // 時間システムアクティブフラグ
    UPROPERTY(BlueprintReadWrite, Category = "Autonomous Time System")
    bool bTimeSystemActive = false;
    
    // タイマーハンドル
    UPROPERTY()
    FTimerHandle TimeUpdateTimerHandle;

    // 現在のターン番号
    UPROPERTY(BlueprintReadOnly, Category = "Autonomous Time System")
    int32 CurrentTurn = 0;

public:
    // ===========================================
    // Phase 3: 簡素化されたパブリックAPI
    // ===========================================

    /** 時間システム開始 */
    UFUNCTION(BlueprintCallable, Category = "Autonomous Time System")
    void StartTimeSystem();
    
    /** 時間システム停止 */
    UFUNCTION(BlueprintCallable, Category = "Autonomous Time System")
    void StopTimeSystem();

    /** システム状態取得 */
    UFUNCTION(BlueprintPure, Category = "Autonomous Time System")
    bool IsTimeSystemActive() const { return bTimeSystemActive; }

    /** 現在のターン番号取得 */
    UFUNCTION(BlueprintPure, Category = "Autonomous Time System")
    int32 GetCurrentTurn() const { return CurrentTurn; }

    // === ゲーム速度制御API ===

    /** ゲーム速度設定 */
    UFUNCTION(BlueprintCallable, Category = "Game Speed Control")
    void SetGameSpeed(const FString& SpeedName);

    /** カスタム間隔設定 */
    UFUNCTION(BlueprintCallable, Category = "Game Speed Control")
    void SetCustomInterval(float NewInterval);

    /** ゲームポーズ/再開 */
    UFUNCTION(BlueprintCallable, Category = "Game Speed Control")
    void PauseGame();

    UFUNCTION(BlueprintCallable, Category = "Game Speed Control")
    void ResumeGame();

    /** 現在の実効間隔取得 */
    UFUNCTION(BlueprintPure, Category = "Game Speed Control")
    float GetCurrentInterval() const { return TimeUpdateInterval; }

    /** ポーズ状態取得 */
    UFUNCTION(BlueprintPure, Category = "Game Speed Control")
    bool IsGamePaused() const { return bGamePaused; }

    /** 
     * メインの時間更新処理 - 唯一の責任：全キャラクターにターン開始通知
     * 実装計画書に記載された新しい設計に従う
     */
    UFUNCTION()
    void ProcessTimeUpdate();

private:
    // ===========================================
    // Phase 3: 簡素化された内部実装
    // ===========================================

    /** タイマー設定 */
    void SetupTimer();

    /** タイマークリア */
    void ClearTimer();
};