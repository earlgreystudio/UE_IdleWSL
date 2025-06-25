#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EventLogTypes.generated.h"

class AC_IdleCharacter;

// イベントの大分類
UENUM(BlueprintType)
enum class EEventCategory : uint8
{
    Combat      UMETA(DisplayName = "戦闘"),
    Gathering   UMETA(DisplayName = "採集"),
    Construction UMETA(DisplayName = "建築"),
    Quest       UMETA(DisplayName = "クエスト"),
    Character   UMETA(DisplayName = "キャラクター"),
    System      UMETA(DisplayName = "システム")
};

// 詳細なイベントタイプ
UENUM(BlueprintType)
enum class EEventLogType : uint8
{
    // 戦闘系 (既存)
    Attack          UMETA(DisplayName = "攻撃"),
    Hit             UMETA(DisplayName = "命中"),
    Miss            UMETA(DisplayName = "外れ"),
    Dodge           UMETA(DisplayName = "回避"),
    Parry           UMETA(DisplayName = "受け流し"),
    Critical        UMETA(DisplayName = "クリティカル"),
    Damage          UMETA(DisplayName = "ダメージ"),
    Death           UMETA(DisplayName = "死亡"),
    CombatStart     UMETA(DisplayName = "戦闘開始"),
    CombatEnd       UMETA(DisplayName = "戦闘終了"),
    
    // 採集系 (新規)
    GatheringStart    UMETA(DisplayName = "採集開始"),
    GatheringSuccess  UMETA(DisplayName = "採集成功"),
    GatheringFailed   UMETA(DisplayName = "採集失敗"),
    ResourceDepleted  UMETA(DisplayName = "資源枯渇"),
    RareItemFound     UMETA(DisplayName = "レアアイテム発見"),
    
    // 建築系 (新規)
    ConstructionStart     UMETA(DisplayName = "建設開始"),
    ConstructionProgress  UMETA(DisplayName = "建設進行"),
    ConstructionComplete  UMETA(DisplayName = "建設完了"),
    ConstructionFailed    UMETA(DisplayName = "建設失敗"),
    BuildingUpgrade       UMETA(DisplayName = "建物アップグレード"),
    
    // クエスト系 (新規)
    QuestReceived     UMETA(DisplayName = "クエスト受諾"),
    QuestProgress     UMETA(DisplayName = "クエスト進行"),
    QuestComplete     UMETA(DisplayName = "クエスト完了"),
    QuestFailed       UMETA(DisplayName = "クエスト失敗"),
    
    // キャラクター系 (新規)
    LevelUp           UMETA(DisplayName = "レベルアップ"),
    SkillGained       UMETA(DisplayName = "スキル習得"),
    ItemEquipped      UMETA(DisplayName = "装備変更"),
    StatusChanged     UMETA(DisplayName = "状態変化"),
    
    // システム系 (新規)
    GameSaved         UMETA(DisplayName = "ゲーム保存"),
    Error             UMETA(DisplayName = "エラー"),
    Debug             UMETA(DisplayName = "デバッグ")
};

// イベントの重要度
UENUM(BlueprintType)
enum class EEventPriority : uint8
{
    Low     UMETA(DisplayName = "低"),
    Normal  UMETA(DisplayName = "通常"),
    High    UMETA(DisplayName = "高"),
    Critical UMETA(DisplayName = "緊急")
};

// 型安全なイベントデータベースクラス
USTRUCT(BlueprintType)
struct UE_IDLE_API FEventDataBase
{
    GENERATED_BODY()
    
    FEventDataBase() {}
    
    // 非仮想関数（Unrealの制約のため）
    FString ToString() const { return TEXT(""); }
    TMap<FString, FString> ToStringMap() const { return TMap<FString, FString>(); }
};

// 戦闘イベント専用データ
USTRUCT(BlueprintType)
struct UE_IDLE_API FCombatEventData : public FEventDataBase
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    FString AttackerName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    FString DefenderName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    FString WeaponName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    int32 Damage = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    int32 AttackerHP = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    int32 AttackerMaxHP = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    int32 DefenderHP = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    int32 DefenderMaxHP = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bIsCritical = false;
    
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bIsPlayerAttacker = false;
    
    FString ToString() const
    {
        return FString::Printf(TEXT("Attacker: %s, Defender: %s, Weapon: %s, Damage: %d"), 
                             *AttackerName, *DefenderName, *WeaponName, Damage);
    }
    
    TMap<FString, FString> ToStringMap() const
    {
        TMap<FString, FString> Result;
        Result.Add(TEXT("AttackerName"), AttackerName);
        Result.Add(TEXT("DefenderName"), DefenderName);
        Result.Add(TEXT("WeaponName"), WeaponName);
        Result.Add(TEXT("Damage"), FString::FromInt(Damage));
        Result.Add(TEXT("AttackerHP"), FString::FromInt(AttackerHP));
        Result.Add(TEXT("AttackerMaxHP"), FString::FromInt(AttackerMaxHP));
        Result.Add(TEXT("DefenderHP"), FString::FromInt(DefenderHP));
        Result.Add(TEXT("DefenderMaxHP"), FString::FromInt(DefenderMaxHP));
        Result.Add(TEXT("bIsCritical"), bIsCritical ? TEXT("true") : TEXT("false"));
        Result.Add(TEXT("bIsPlayerAttacker"), bIsPlayerAttacker ? TEXT("true") : TEXT("false"));
        return Result;
    }
};

// 採集イベント専用データ
USTRUCT(BlueprintType)
struct UE_IDLE_API FGatheringEventData : public FEventDataBase
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    FString GathererName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    FString ResourceType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    int32 Amount = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    FString LocationName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    float SuccessRate = 0.0f;
    
    FString ToString() const
    {
        return FString::Printf(TEXT("Gatherer: %s, Resource: %s, Amount: %d, Location: %s"), 
                             *GathererName, *ResourceType, Amount, *LocationName);
    }
};

// 建築イベント専用データ
USTRUCT(BlueprintType)
struct UE_IDLE_API FConstructionEventData : public FEventDataBase
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Construction")
    FString BuildingName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Construction")
    FString BuilderName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Construction")
    float Progress = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, Category = "Construction")
    float EstimatedTime = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, Category = "Construction")
    TMap<FString, int32> RequiredMaterials;
    
    FString ToString() const
    {
        return FString::Printf(TEXT("Building: %s, Builder: %s, Progress: %.1f%%"), 
                             *BuildingName, *BuilderName, Progress * 100.0f);
    }
};

// メインのイベントログエントリ
USTRUCT(BlueprintType)
struct UE_IDLE_API FEventLogEntry
{
    GENERATED_BODY()
    
    FEventLogEntry()
    {
        EventCategory = EEventCategory::System;
        EventType = EEventLogType::Debug;
        Priority = EEventPriority::Normal;
        Timestamp = 0.0f;
    }
    
    // 基本情報
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    EEventCategory EventCategory;
    
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    EEventLogType EventType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    EEventPriority Priority;
    
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    float Timestamp;
    
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    FString FormattedText;
    
    // 関連キャラクター（柔軟性のため）
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    TArray<TObjectPtr<AC_IdleCharacter>> InvolvedCharacters;
    
    // 後方互換性のための汎用データマップ
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    TMap<FString, FString> LegacyEventData;
    
    // 型安全な専用データ（各イベントタイプで使い分け）
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    FCombatEventData CombatData;
    
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    FGatheringEventData GatheringData;
    
    UPROPERTY(BlueprintReadWrite, Category = "Event")
    FConstructionEventData ConstructionData;
};

// イベントサマリー
USTRUCT(BlueprintType)
struct UE_IDLE_API FEventSummary
{
    GENERATED_BODY()
    
    FEventSummary()
    {
        EventCategory = EEventCategory::System;
        Priority = EEventPriority::Normal;
        StartTime = 0.0f;
        EndTime = 0.0f;
        bIsSuccess = false;
    }
    
    // 基本情報
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    EEventCategory EventCategory;
    
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    EEventPriority Priority;
    
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    float StartTime;
    
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    float EndTime;
    
    // 表示用テキスト
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    FString Title;          // "⚔️洞窟でチームAがゴブリンと遭遇"
    
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    FString ResultText;     // "✅チームAの勝利"
    
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    FString DetailText;     // キャラクター別詳細情報
    
    // 結果フラグ
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    bool bIsSuccess;
    
    // 関連データ
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    TArray<FString> ParticipantNames;
    
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    TMap<FString, FString> SummaryMetrics;  // "TotalDamage", "Duration"など
    
    // 詳細ログ
    UPROPERTY(BlueprintReadWrite, Category = "Summary")
    TArray<FEventLogEntry> DetailedLogs;
};

// フィルタリング用構造体
USTRUCT(BlueprintType)
struct UE_IDLE_API FEventLogFilter
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Filter")
    TArray<EEventCategory> AllowedCategories;
    
    UPROPERTY(BlueprintReadWrite, Category = "Filter")
    TArray<EEventLogType> AllowedTypes;
    
    UPROPERTY(BlueprintReadWrite, Category = "Filter")
    EEventPriority MinPriority = EEventPriority::Low;
    
    UPROPERTY(BlueprintReadWrite, Category = "Filter")
    float TimeRangeStart = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, Category = "Filter")
    float TimeRangeEnd = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, Category = "Filter")
    FString SearchText;
};