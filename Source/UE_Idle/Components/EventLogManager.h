#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/EventLogTypes.h"
#include "../Types/CombatTypes.h"  // FCombatCalculationResult, ECombatLogType用
#include "EventLogManager.generated.h"

class AC_IdleCharacter;

// 後方互換性のためのデリゲート宣言（既存コードで使用）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatLogAddedEvent, const FEventLogEntry&, LogEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatLogClearedEvent);

// 新しいイベントログ用デリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventLogAdded, const FEventLogEntry&, LogEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventSummaryCreated, const FEventSummary&, Summary);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEventLogCleared);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UEventLogManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UEventLogManager();

protected:
    virtual void BeginPlay() override;

public:
    // ==================== 既存の戦闘ログ機能（後方互換性） ====================
    
    // 既存の戦闘ログ追加（CombatLogManagerと同じインターフェース）
    UFUNCTION(BlueprintCallable, Category = "Combat Log", meta = (CallInEditor = "true"))
    void AddCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, AC_IdleCharacter* Target = nullptr, 
                     const FString& WeaponOrItemName = TEXT(""), int32 DamageValue = 0, const FString& AdditionalInfo = TEXT(""));

    // 戦闘計算結果からログを自動生成（既存機能）
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void AddCombatCalculationLog(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
                                const FString& WeaponName, const FCombatCalculationResult& Result);

    // 戦闘開始/終了の特別ログ（既存機能）
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void LogCombatStart(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationName);

    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void LogCombatEnd(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers, float CombatDuration);

    // ==================== 新しい統一イベントログ機能 ====================
    
    // 汎用イベントログ追加
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void AddEventLog(const FEventLogEntry& EventEntry);
    
    // イベントサマリー作成
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void CreateEventSummary(EEventCategory Category, const FString& Title, const FString& ResultText, bool bSuccess = true);
    
    // 現在のログからサマリーを生成
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    FEventSummary GenerateCurrentEventSummary(EEventCategory Category, const FString& Title) const;
    
    // 戦闘専用の便利関数
    UFUNCTION(BlueprintCallable, Category = "Combat Log")
    void AddCombatEvent(EEventLogType EventType, AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
                       const FString& WeaponName, int32 Damage = 0, bool bIsCritical = false);
    
    // 採集イベント追加
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void AddGatheringEvent(EEventLogType EventType, AC_IdleCharacter* Gatherer, const FString& ResourceType, 
                          int32 Amount, const FString& LocationName, float SuccessRate = 100.0f);
    
    // 建築イベント追加
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void AddConstructionEvent(EEventLogType EventType, const FString& BuildingName, const FString& BuilderName, 
                             float Progress);
    
    // 建築イベント追加（材料指定版）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void AddConstructionEventWithMaterials(EEventLogType EventType, const FString& BuildingName, const FString& BuilderName, 
                                          float Progress, const TMap<FString, int32>& RequiredMaterials);

    // ==================== ログ取得機能 ====================
    
    // 全ログ取得（後方互換性）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventLogEntry> GetAllLogs() const { return EventLogs; }

    // 最新のN件のログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventLogEntry> GetRecentLogs(int32 Count) const;

    // 特定カテゴリのログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventLogEntry> GetLogsByCategory(EEventCategory Category) const;
    
    // 特定タイプのログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventLogEntry> GetLogsByType(EEventLogType EventType) const;
    
    // フィルタリングしたログ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventLogEntry> GetFilteredLogs(const FEventLogFilter& Filter) const;

    // 全サマリー取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventSummary> GetAllEventSummaries() const;
    
    // 最新のN件のサマリー取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FEventSummary> GetRecentEventSummaries(int32 Count) const;

    // ==================== ログ管理機能 ====================
    
    // ログクリア
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void ClearLogs();
    
    // サマリークリア
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void ClearEventSummaries();

    // ログ数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    int32 GetLogCount() const { return EventLogs.Num(); }
    
    // サマリー数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    int32 GetEventSummaryCount() const { return EventSummaries.Num(); }

    // UI用フォーマット済みログ取得（後方互換性）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FString> GetFormattedLogs(int32 RecentCount = -1) const;
    
    // UI用フォーマット済みサマリー取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    TArray<FString> GetFormattedEventSummaries(int32 RecentCount = -1) const;

    // ==================== イベントディスパッチャー ====================
    
    // 後方互換性用（既存コードで使用）
    UPROPERTY(BlueprintAssignable, Category = "Combat Log Events")
    FOnCombatLogAddedEvent OnCombatLogAdded;

    UPROPERTY(BlueprintAssignable, Category = "Combat Log Events")
    FOnCombatLogClearedEvent OnCombatLogCleared;
    
    // 新しいイベント
    UPROPERTY(BlueprintAssignable, Category = "Event Log Events")
    FOnEventLogAdded OnEventLogAdded;
    
    UPROPERTY(BlueprintAssignable, Category = "Event Log Events")
    FOnEventSummaryCreated OnEventSummaryCreated;

    UPROPERTY(BlueprintAssignable, Category = "Event Log Events")
    FOnEventLogCleared OnEventLogCleared;

    // ==================== 設定 ====================
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxLogEntries = 1000;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxSummaryEntries = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoFormatLogs = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bAutoCreateSummaries = true;

protected:
    // ログデータ
    UPROPERTY()
    TArray<FEventLogEntry> EventLogs;
    
    // サマリーデータ
    UPROPERTY()
    TArray<FEventSummary> EventSummaries;

    // 戦闘開始時刻（後方互換性）
    UPROPERTY()
    float CombatStartTime;
    
    // 現在のイベント開始時刻
    UPROPERTY()
    float CurrentEventStartTime;
    
    // 戦闘情報（サマリー用）
    UPROPERTY()
    FString CurrentCombatLocation;
    
    UPROPERTY()
    FString CurrentAllyTeamNames;
    
    UPROPERTY()
    FString CurrentEnemyTeamNames;

private:
    // ==================== ヘルパー関数 ====================
    
    // フォーマット関数
    FString FormatLogEntry(const FEventLogEntry& LogEntry) const;
    FString FormatEventSummary(const FEventSummary& Summary) const;
    
    // 戦闘専用フォーマット（後方互換性）
    FString FormatCombatEntry(const FEventLogEntry& LogEntry) const;
    
    // 各イベントタイプ専用フォーマット
    FString FormatGatheringEntry(const FEventLogEntry& LogEntry) const;
    FString FormatConstructionEntry(const FEventLogEntry& LogEntry) const;
    
    // ユーティリティ関数
    FString GetCharacterDisplayName(AC_IdleCharacter* Character) const;
    float GetCurrentEventTime() const;
    void TrimLogsIfNeeded();
    void TrimSummariesIfNeeded();
    
    // 戦闘ログから新しい形式に変換
    FEventLogEntry ConvertLegacyCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, 
                                         AC_IdleCharacter* Target, const FString& WeaponOrItemName, 
                                         int32 DamageValue, const FString& AdditionalInfo) const;
    
    // イベントカテゴリを自動判定
    EEventCategory DetermineEventCategory(EEventLogType EventType) const;
};