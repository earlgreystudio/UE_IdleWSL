#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/CombatTypes.h"
#include "ActionSystemComponent.generated.h"

class AC_IdleCharacter;
class UCombatCalculator;
class UEventLogManager;

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCharacterAction, AC_IdleCharacter*, Actor, AC_IdleCharacter*, Target, const FCombatCalculationResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AC_IdleCharacter*, Character);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UActionSystemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionSystemComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // 行動システム開始/停止
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void StartActionSystem();

    UFUNCTION(BlueprintCallable, Category = "Action System")
    void StopActionSystem();

    // キャラクター登録/登録解除（非推奨）
    void RegisterCharacter(AC_IdleCharacter* Character, const TArray<AC_IdleCharacter*>& Enemies);

    UFUNCTION(BlueprintCallable, Category = "Action System")
    void UnregisterCharacter(AC_IdleCharacter* Character);

    // チーム全体を登録（推奨）
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void RegisterTeam(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam);
    
    // 明確な登録メソッド
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void RegisterAlly(AC_IdleCharacter* Character);
    
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void RegisterEnemy(AC_IdleCharacter* Character);

    // 全キャラクター登録解除
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void ClearAllCharacters();

    // キャラクターの死亡処理
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void HandleCharacterDeath(AC_IdleCharacter* Character);

    // 生存者チェック
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    bool AreAllEnemiesDead() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    bool AreAllAlliesDead() const;

    // 生存キャラクター取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    TArray<AC_IdleCharacter*> GetAliveAllies() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    TArray<AC_IdleCharacter*> GetAliveEnemies() const;

    // 全キャラクター取得（生死問わず）  
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    TArray<AC_IdleCharacter*> GetAllAllies() const;
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    TArray<AC_IdleCharacter*> GetAllEnemies() const;

    // システム状態
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action System")
    bool IsSystemActive() const { return bSystemActive; }

    // ログマネージャー設定
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void SetEventLogManager(UEventLogManager* LogManager) { EventLogManager = LogManager; }

    // イベントディスパッチャー
    UPROPERTY(BlueprintAssignable, Category = "Action Events")
    FOnCharacterAction OnCharacterAction;

    UPROPERTY(BlueprintAssignable, Category = "Action Events")
    FOnCharacterDeath OnCharacterDeath;

    // 設定（0.0f以下で即座実行）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float ActionCheckInterval = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableAI = true;
    
    // === 行動ゲージシステム（public for CombatComponent access）===
    
    // ゲージベース1ターン処理
    UFUNCTION(BlueprintCallable, Category = "Action Gauge System")
    bool ProcessSingleTurnWithGauge();

private:
    // フェイルセーフ用：実際のキャラクターアクション数をカウント
    int32 TotalActionCount = 0;

protected:
    // タイマー処理（既存のシステム）
    UFUNCTION()
    void ProcessActions();
    
    // 新しい1ターン1行動システム（既存）
    UFUNCTION(BlueprintCallable, Category = "Turn Based Combat")
    bool ProcessSingleTurn();
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn Based Combat")
    AC_IdleCharacter* GetNextActingCharacter() const;
    
    // 全キャラクターのゲージ更新
    UFUNCTION(BlueprintCallable, Category = "Action Gauge System")
    void UpdateAllGauges();
    
    // 最高ゲージキャラクター取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action Gauge System")
    AC_IdleCharacter* GetNextActingCharacterWithGauge() const;
    
    // キャラクターゲージ初期化
    UFUNCTION(BlueprintCallable, Category = "Action Gauge System")
    void InitializeCharacterGauge(AC_IdleCharacter* Character);

    // キャラクター行動処理（戦闘用構造体を使用）
    void ProcessCharacterAction(FCombatAction& Action);

    // AI行動決定
    AC_IdleCharacter* SelectTarget(AC_IdleCharacter* Actor, const TArray<AC_IdleCharacter*>& Enemies);

    // 武器選択
    FString SelectWeapon(AC_IdleCharacter* Character);

    // 攻撃速度計算と次回行動時間設定
    void UpdateNextActionTime(FCombatAction& Action, const FString& WeaponId);

    // キャラクターの生存チェック
    bool IsCharacterAlive(AC_IdleCharacter* Character) const;

    // デバッグ用（戦闘用構造体を使用）
    void LogActionInfo(const FCombatAction& Action, const FString& WeaponId) const;
    
    // 戦闘終了通知は削除 - CombatComponentで一元管理
    
    // 戦闘終了時のクリーンアップ
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void ClearAllActions();
    
    // 無効なキャラクター参照をクリーンアップ
    UFUNCTION(BlueprintCallable, Category = "Action System")
    void CleanupInvalidCharacters();

private:
    // 登録済みキャラクター（戦闘用構造体を使用）
    UPROPERTY()
    TArray<FCombatAction> AllyActions;

    UPROPERTY()
    TArray<FCombatAction> EnemyActions;

    // システム状態
    UPROPERTY()
    bool bSystemActive;

    // タイマーハンドル
    FTimerHandle ActionTimerHandle;

    // 外部コンポーネント参照
    UPROPERTY()
    TObjectPtr<UEventLogManager> EventLogManager;

    // ヘルパー関数（戦闘用構造体を使用）
    FCombatAction* FindCharacterAction(AC_IdleCharacter* Character);
    bool RemoveCharacterAction(AC_IdleCharacter* Character);
};