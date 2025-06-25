#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/CombatTypes.h"
#include "../Types/TeamTypes.h"
#include "CombatComponent.generated.h"

class AC_IdleCharacter;
class UCombatLogManager;
class UActionSystemComponent;

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatStateChanged, ECombatState, OldState, ECombatState, NewState, const FString&, StateInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatCompleted, const TArray<AC_IdleCharacter*>&, Winners, const TArray<AC_IdleCharacter*>&, Losers, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCombatAction, AC_IdleCharacter*, Actor, AC_IdleCharacter*, Target, const FString&, WeaponName, const FCombatCalculationResult&, Result);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

protected:
    virtual void BeginPlay() override;

public:
    // 戦闘開始（LocationEventManagerから呼び出し）
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool StartCombat(const FTeam& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationId);

    // 戦闘強制終了
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ForceEndCombat();

    // 戦闘状態取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    ECombatState GetCombatState() const { return CurrentState; }

    // 戦闘継続時間取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    float GetCombatDuration() const;

    // 参加中のチーム取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    TArray<AC_IdleCharacter*> GetAllyTeam() const { return AllyTeamMembers; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    TArray<AC_IdleCharacter*> GetEnemyTeam() const { return EnemyTeamMembers; }

    // 戦闘場所情報
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    FString GetCombatLocationId() const { return CombatLocationId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    FString GetCombatLocationName() const;


    // 戦闘ログ取得
    UFUNCTION(BlueprintCallable, Category = "Combat")
    TArray<FString> GetCombatLogs(int32 RecentCount = -1) const;

    // 戦闘統計
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    int32 GetTotalActions() const { return TotalActionCount; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    int32 GetTotalDamageDealt() const { return TotalDamageDealt; }

    // イベントディスパッチャー
    UPROPERTY(BlueprintAssignable, Category = "Combat Events")
    FOnCombatStateChanged OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Combat Events")
    FOnCombatCompleted OnCombatCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Combat Events")
    FOnCombatAction OnCombatAction;

    // 設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoStartCombat = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float CombatPreparationTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxEnemiesPerLocation = 3;

protected:
    // 状態変更
    void SetCombatState(ECombatState NewState, const FString& StateInfo = TEXT(""));

    // 敵生成
    bool SpawnEnemiesAtLocation(const FString& LocationId);

    // 戦闘準備完了チェック
    bool IsCombatReadyToStart() const;

    // 戦闘終了チェック
    void CheckCombatCompletion();

    // イベントハンドラー
    UFUNCTION()
    void OnActionSystemCharacterAction(AC_IdleCharacter* Actor, AC_IdleCharacter* Target, const FCombatCalculationResult& Result);

    UFUNCTION()
    void OnActionSystemCharacterDeath(AC_IdleCharacter* Character);

    // タイマー処理
    UFUNCTION()
    void StartCombatAfterPreparation();

private:
    // 戦闘状態
    UPROPERTY()
    ECombatState CurrentState;

    // 参加キャラクター
    UPROPERTY()
    TArray<AC_IdleCharacter*> AllyTeamMembers;

    UPROPERTY()
    TArray<AC_IdleCharacter*> EnemyTeamMembers;

    // 戦闘場所
    UPROPERTY()
    FString CombatLocationId;

    // 戦闘開始時刻
    UPROPERTY()
    float CombatStartTime;

    // 統計情報
    UPROPERTY()
    int32 TotalActionCount;

    UPROPERTY()
    int32 TotalDamageDealt;

    // タイマーハンドル
    FTimerHandle PreparationTimerHandle;

    // 子コンポーネント
    UPROPERTY()
    TObjectPtr<UCombatLogManager> CombatLogManager;

    UPROPERTY()
    TObjectPtr<UActionSystemComponent> ActionSystemComponent;


    // ヘルパー関数
    void InitializeSubComponents();
    void CleanupAfterCombat();
    TArray<AC_IdleCharacter*> GetAliveMembers(const TArray<AC_IdleCharacter*>& Team) const;
};