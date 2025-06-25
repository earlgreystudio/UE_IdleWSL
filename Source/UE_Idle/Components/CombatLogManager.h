#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/CombatTypes.h"
#include "CombatLogManager.generated.h"

class AC_IdleCharacter;

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatLogAdded, const FCombatLogEntry&, LogEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatLogCleared);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UCombatLogManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatLogManager();

protected:
    virtual void BeginPlay() override;

public:
    // ログの追加
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void AddCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, AC_IdleCharacter* Target = nullptr, 
                     const FString& WeaponOrItemName = TEXT(""), int32 DamageValue = 0, const FString& AdditionalInfo = TEXT(""));

    // 戦闘計算結果からログを自動生成
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void AddCombatCalculationLog(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
                                const FString& WeaponName, const FCombatCalculationResult& Result);

    // 全ログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat Log")
    TArray<FCombatLogEntry> GetAllLogs() const { return CombatLogs; }

    // 最新のN件のログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat Log")
    TArray<FCombatLogEntry> GetRecentLogs(int32 Count) const;

    // 特定タイプのログのみ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat Log")
    TArray<FCombatLogEntry> GetLogsByType(ECombatLogType CombatLogType) const;

    // ログクリア
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void ClearLogs();

    // ログ数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat Log")
    int32 GetLogCount() const { return CombatLogs.Num(); }

    // UI用フォーマット済みログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat Log")
    TArray<FString> GetFormattedLogs(int32 RecentCount = -1) const;

    // 戦闘開始/終了の特別ログ
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void LogCombatStart(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationName);

    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void LogCombatEnd(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers, float CombatDuration);

    // イベントディスパッチャー
    UPROPERTY(BlueprintAssignable, Category = "Combat Log Events")
    FOnCombatLogAdded OnCombatLogAdded;

    UPROPERTY(BlueprintAssignable, Category = "Combat Log Events")
    FOnCombatLogCleared OnCombatLogCleared;

    // 設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxLogEntries = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoFormatLogs = true;

protected:
    // ログリスト
    UPROPERTY()
    TArray<FCombatLogEntry> CombatLogs;

    // 戦闘開始時刻
    UPROPERTY()
    float CombatStartTime;

private:
    // ヘルパー関数
    FString FormatLogEntry(const FCombatLogEntry& LogEntry) const;
    FString GetCharacterDisplayName(AC_IdleCharacter* Character) const;
    float GetCurrentCombatTime() const;
    void TrimLogsIfNeeded();
};