#include "EventLogManager.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "../Types/CombatTypes.h"  // 実装で必要
#include "../Components/CharacterStatusComponent.h"  // HP取得用
#include "Engine/World.h"

UEventLogManager::UEventLogManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    CombatStartTime = 0.0f;
    CurrentEventStartTime = 0.0f;
}

void UEventLogManager::BeginPlay()
{
    Super::BeginPlay();
    CombatStartTime = GetWorld()->GetTimeSeconds();
    CurrentEventStartTime = CombatStartTime;
}

// ==================== 既存の戦闘ログ機能（後方互換性） ====================

void UEventLogManager::AddCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, AC_IdleCharacter* Target, 
                                   const FString& WeaponOrItemName, int32 DamageValue, const FString& AdditionalInfo)
{
    // 既存のインターフェースを新しい構造に変換
    FEventLogEntry NewEntry = ConvertLegacyCombatLog(CombatLogType, Actor, Target, WeaponOrItemName, DamageValue, AdditionalInfo);
    
    if (bAutoFormatLogs)
    {
        NewEntry.FormattedText = FormatLogEntry(NewEntry);
    }
    
    EventLogs.Add(NewEntry);
    TrimLogsIfNeeded();
    
    // 既存のイベント発火（後方互換性）
    OnCombatLogAdded.Broadcast(NewEntry);
    
    // 新しいイベント発火
    OnEventLogAdded.Broadcast(NewEntry);
    
    // デバッグログ出力
    UE_LOG(LogTemp, Log, TEXT("EventLog: %s"), *NewEntry.FormattedText);
}

void UEventLogManager::AddCombatCalculationLog(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
                                              const FString& WeaponName, const FCombatCalculationResult& Result)
{
    if (!Attacker || !Defender)
    {
        return;
    }

    FString AttackerName = GetCharacterDisplayName(Attacker);
    FString DefenderName = GetCharacterDisplayName(Defender);
    
    if (Result.bDodged)
    {
        // 回避された
        AddCombatLog(ECombatLogType::Dodge, Attacker, Defender, WeaponName, 0, 
                    FString::Printf(TEXT("回避率%.1f%%"), Result.DodgeChance));
    }
    else if (!Result.bHit)
    {
        // 外れた
        AddCombatLog(ECombatLogType::Miss, Attacker, Defender, WeaponName, 0, 
                    FString::Printf(TEXT("命中率%.1f%%"), Result.HitChance));
    }
    else
    {
        // 命中した
        ECombatLogType HitLogType = ECombatLogType::Hit;
        FString AdditionalInfo = FString::Printf(TEXT("基本%d"), Result.BaseDamage);
        
        if (Result.bCritical)
        {
            HitLogType = ECombatLogType::Critical;
            AdditionalInfo += TEXT(" クリティカル！");
        }
        
        if (Result.bParried)
        {
            AdditionalInfo += TEXT(" 受け流された");
        }
        
        AddCombatLog(HitLogType, Attacker, Defender, WeaponName, Result.FinalDamage, AdditionalInfo);
    }
}

void UEventLogManager::LogCombatStart(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationName)
{
    FString AllyNames;
    for (int32 i = 0; i < AllyTeam.Num(); i++)
    {
        if (AllyTeam[i])
        {
            AllyNames += GetCharacterDisplayName(AllyTeam[i]);
            if (i < AllyTeam.Num() - 1)
            {
                AllyNames += TEXT(", ");
            }
        }
    }
    
    FString EnemyNames;
    for (int32 i = 0; i < EnemyTeam.Num(); i++)
    {
        if (EnemyTeam[i])
        {
            EnemyNames += GetCharacterDisplayName(EnemyTeam[i]);
            if (i < EnemyTeam.Num() - 1)
            {
                EnemyNames += TEXT(", ");
            }
        }
    }
    
    FString CombatInfo = FString::Printf(TEXT("⚔️%sで戦闘開始！ 味方：%s vs 敵：%s"), 
                                        *LocationName, *AllyNames, *EnemyNames);
    
    AddCombatLog(ECombatLogType::CombatStart, nullptr, nullptr, TEXT(""), 0, CombatInfo);
    
    // 戦闘開始時刻を記録
    CombatStartTime = GetWorld()->GetTimeSeconds();
    CurrentEventStartTime = CombatStartTime;
}

void UEventLogManager::LogCombatEnd(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers, float CombatDuration)
{
    UE_LOG(LogTemp, Log, TEXT("LogCombatEnd called with %d winners, %d losers"), Winners.Num(), Losers.Num());
    
    FString WinnerNames;
    for (int32 i = 0; i < Winners.Num(); i++)
    {
        if (Winners[i])
        {
            WinnerNames += GetCharacterDisplayName(Winners[i]);
            if (i < Winners.Num() - 1)
            {
                WinnerNames += TEXT(", ");
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Winner names: %s"), *WinnerNames);
    
    FString CombatResult = FString::Printf(TEXT("✅戦闘終了！ 勝者：%s (戦闘時間: %.1f秒)"), 
                                          *WinnerNames, CombatDuration);
    
    AddCombatLog(ECombatLogType::CombatEnd, nullptr, nullptr, TEXT(""), 0, CombatResult);
    
    // 戦闘サマリーを自動作成
    UE_LOG(LogTemp, Log, TEXT("bAutoCreateSummaries = %s"), bAutoCreateSummaries ? TEXT("true") : TEXT("false"));
    if (bAutoCreateSummaries)
    {
        UE_LOG(LogTemp, Log, TEXT("Creating combat summary for winners: %s"), *WinnerNames);
        UE_LOG(LogTemp, Log, TEXT("Before CreateEventSummary - Total summaries: %d"), EventSummaries.Num());
        CreateEventSummary(EEventCategory::Combat, 
                          TEXT("⚔️戦闘"), 
                          FString::Printf(TEXT("✅%s の勝利"), *WinnerNames), 
                          true);
        UE_LOG(LogTemp, Log, TEXT("After CreateEventSummary - Total summaries: %d"), EventSummaries.Num());
        UE_LOG(LogTemp, Log, TEXT("Combat summary created successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Auto create summaries is disabled"));
    }
}

// ==================== 新しい統一イベントログ機能 ====================

void UEventLogManager::AddEventLog(const FEventLogEntry& EventEntry)
{
    FEventLogEntry NewEntry = EventEntry;
    NewEntry.Timestamp = GetCurrentEventTime();
    
    if (bAutoFormatLogs && NewEntry.FormattedText.IsEmpty())
    {
        NewEntry.FormattedText = FormatLogEntry(NewEntry);
    }
    
    EventLogs.Add(NewEntry);
    TrimLogsIfNeeded();
    
    OnEventLogAdded.Broadcast(NewEntry);
    
    UE_LOG(LogTemp, Log, TEXT("EventLog: %s"), *NewEntry.FormattedText);
}

void UEventLogManager::CreateEventSummary(EEventCategory Category, const FString& Title, const FString& ResultText, bool bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("Creating EventSummary - Category: %s, Title: %s"), *UEnum::GetValueAsString(Category), *Title);
    
    FEventSummary Summary = GenerateCurrentEventSummary(Category, Title);
    Summary.ResultText = ResultText;
    Summary.bIsSuccess = bSuccess;
    Summary.EndTime = GetCurrentEventTime();
    
    UE_LOG(LogTemp, Log, TEXT("Generated summary with %d detailed logs"), Summary.DetailedLogs.Num());
    
    EventSummaries.Add(Summary);
    TrimSummariesIfNeeded();
    
    UE_LOG(LogTemp, Log, TEXT("Total summaries now: %d"), EventSummaries.Num());
    
    OnEventSummaryCreated.Broadcast(Summary);
    
    UE_LOG(LogTemp, Log, TEXT("EventSummary Created: %s - %s"), *Summary.Title, *Summary.ResultText);
}

FEventSummary UEventLogManager::GenerateCurrentEventSummary(EEventCategory Category, const FString& Title) const
{
    FEventSummary Summary;
    Summary.EventCategory = Category;
    Summary.Title = Title;
    Summary.StartTime = CurrentEventStartTime;
    Summary.EndTime = GetCurrentEventTime();
    
    // 現在のログから詳細ログを抽出（戦闘開始時以降のログ）
    for (const FEventLogEntry& LogEntry : EventLogs)
    {
        if (LogEntry.Timestamp >= CurrentEventStartTime && 
            LogEntry.EventCategory == Category)
        {
            Summary.DetailedLogs.Add(LogEntry);
        }
    }
    
    return Summary;
}

void UEventLogManager::AddCombatEvent(EEventLogType EventType, AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
                                     const FString& WeaponName, int32 Damage, bool bIsCritical)
{
    FEventLogEntry NewEntry;
    NewEntry.EventCategory = EEventCategory::Combat;
    NewEntry.EventType = EventType;
    NewEntry.Priority = EEventPriority::Normal;
    
    // 戦闘データを設定
    if (Attacker) NewEntry.CombatData.AttackerName = GetCharacterDisplayName(Attacker);
    if (Defender) NewEntry.CombatData.DefenderName = GetCharacterDisplayName(Defender);
    NewEntry.CombatData.WeaponName = WeaponName;
    NewEntry.CombatData.Damage = Damage;
    NewEntry.CombatData.bIsCritical = bIsCritical;
    
    // プレイヤーかどうかを判定
    if (Attacker && Attacker->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
    {
        FString AttackerRace = Attacker->GetCharacterRace();
        NewEntry.CombatData.bIsPlayerAttacker = (AttackerRace == TEXT("human"));
    }
    
    // HPデータを取得
    if (Attacker)
    {
        UE_LOG(LogTemp, Log, TEXT("Getting HP for Attacker: %s"), *GetCharacterDisplayName(Attacker));
        if (UCharacterStatusComponent* StatusComp = Attacker->GetStatusComponent())
        {
            FCharacterStatus Status = StatusComp->GetStatus();
            NewEntry.CombatData.AttackerHP = FMath::RoundToInt(Status.CurrentHealth);
            NewEntry.CombatData.AttackerMaxHP = FMath::RoundToInt(Status.MaxHealth);
            UE_LOG(LogTemp, Log, TEXT("Attacker HP: %d/%d"), NewEntry.CombatData.AttackerHP, NewEntry.CombatData.AttackerMaxHP);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Attacker has no StatusComponent"));
        }
    }
    
    if (Defender)
    {
        UE_LOG(LogTemp, Log, TEXT("Getting HP for Defender: %s"), *GetCharacterDisplayName(Defender));
        if (UCharacterStatusComponent* StatusComp = Defender->GetStatusComponent())
        {
            FCharacterStatus Status = StatusComp->GetStatus();
            NewEntry.CombatData.DefenderHP = FMath::RoundToInt(Status.CurrentHealth);
            NewEntry.CombatData.DefenderMaxHP = FMath::RoundToInt(Status.MaxHealth);
            UE_LOG(LogTemp, Log, TEXT("Defender HP: %d/%d"), NewEntry.CombatData.DefenderHP, NewEntry.CombatData.DefenderMaxHP);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Defender has no StatusComponent"));
        }
    }
    
    AddEventLog(NewEntry);
}

void UEventLogManager::AddGatheringEvent(EEventLogType EventType, AC_IdleCharacter* Gatherer, const FString& ResourceType, 
                                        int32 Amount, const FString& LocationName, float SuccessRate)
{
    FEventLogEntry NewEntry;
    NewEntry.EventCategory = EEventCategory::Gathering;
    NewEntry.EventType = EventType;
    NewEntry.Priority = EEventPriority::Normal;
    
    // 採集データを設定
    if (Gatherer) NewEntry.GatheringData.GathererName = GetCharacterDisplayName(Gatherer);
    NewEntry.GatheringData.ResourceType = ResourceType;
    NewEntry.GatheringData.Amount = Amount;
    NewEntry.GatheringData.LocationName = LocationName;
    NewEntry.GatheringData.SuccessRate = SuccessRate;
    
    AddEventLog(NewEntry);
}

void UEventLogManager::AddConstructionEvent(EEventLogType EventType, const FString& BuildingName, const FString& BuilderName, 
                                           float Progress)
{
    FEventLogEntry NewEntry;
    NewEntry.EventCategory = EEventCategory::Construction;
    NewEntry.EventType = EventType;
    NewEntry.Priority = EEventPriority::Normal;
    
    // 建築データを設定
    NewEntry.ConstructionData.BuildingName = BuildingName;
    NewEntry.ConstructionData.BuilderName = BuilderName;
    NewEntry.ConstructionData.Progress = Progress;
    
    AddEventLog(NewEntry);
}

void UEventLogManager::AddConstructionEventWithMaterials(EEventLogType EventType, const FString& BuildingName, const FString& BuilderName, 
                                                        float Progress, const TMap<FString, int32>& RequiredMaterials)
{
    FEventLogEntry NewEntry;
    NewEntry.EventCategory = EEventCategory::Construction;
    NewEntry.EventType = EventType;
    NewEntry.Priority = EEventPriority::Normal;
    
    // 建築データを設定
    NewEntry.ConstructionData.BuildingName = BuildingName;
    NewEntry.ConstructionData.BuilderName = BuilderName;
    NewEntry.ConstructionData.Progress = Progress;
    NewEntry.ConstructionData.RequiredMaterials = RequiredMaterials;
    
    AddEventLog(NewEntry);
}

// ==================== ログ取得機能 ====================

TArray<FEventLogEntry> UEventLogManager::GetRecentLogs(int32 Count) const
{
    TArray<FEventLogEntry> RecentLogs;
    
    if (Count <= 0)
    {
        return EventLogs;
    }
    
    int32 StartIndex = FMath::Max(0, EventLogs.Num() - Count);
    for (int32 i = StartIndex; i < EventLogs.Num(); i++)
    {
        RecentLogs.Add(EventLogs[i]);
    }
    
    return RecentLogs;
}

TArray<FEventLogEntry> UEventLogManager::GetLogsByCategory(EEventCategory Category) const
{
    return EventLogs.FilterByPredicate([Category](const FEventLogEntry& Entry) {
        return Entry.EventCategory == Category;
    });
}

TArray<FEventLogEntry> UEventLogManager::GetLogsByType(EEventLogType EventType) const
{
    return EventLogs.FilterByPredicate([EventType](const FEventLogEntry& Entry) {
        return Entry.EventType == EventType;
    });
}

TArray<FEventLogEntry> UEventLogManager::GetFilteredLogs(const FEventLogFilter& Filter) const
{
    return EventLogs.FilterByPredicate([&Filter](const FEventLogEntry& Entry) {
        // カテゴリフィルター
        if (Filter.AllowedCategories.Num() > 0 && !Filter.AllowedCategories.Contains(Entry.EventCategory))
        {
            return false;
        }
        
        // タイプフィルター
        if (Filter.AllowedTypes.Num() > 0 && !Filter.AllowedTypes.Contains(Entry.EventType))
        {
            return false;
        }
        
        // 優先度フィルター
        if (Entry.Priority < Filter.MinPriority)
        {
            return false;
        }
        
        // 時間範囲フィルター
        if (Filter.TimeRangeEnd > Filter.TimeRangeStart)
        {
            if (Entry.Timestamp < Filter.TimeRangeStart || Entry.Timestamp > Filter.TimeRangeEnd)
            {
                return false;
            }
        }
        
        // テキスト検索フィルター
        if (!Filter.SearchText.IsEmpty() && !Entry.FormattedText.Contains(Filter.SearchText))
        {
            return false;
        }
        
        return true;
    });
}

TArray<FEventSummary> UEventLogManager::GetRecentEventSummaries(int32 Count) const
{
    TArray<FEventSummary> RecentSummaries;
    
    if (Count <= 0)
    {
        return EventSummaries;
    }
    
    int32 StartIndex = FMath::Max(0, EventSummaries.Num() - Count);
    for (int32 i = StartIndex; i < EventSummaries.Num(); i++)
    {
        RecentSummaries.Add(EventSummaries[i]);
    }
    
    return RecentSummaries;
}

// ==================== ログ管理機能 ====================

void UEventLogManager::ClearLogs()
{
    EventLogs.Empty();
    OnCombatLogCleared.Broadcast();  // 後方互換性
    OnEventLogCleared.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Event logs cleared"));
}

void UEventLogManager::ClearEventSummaries()
{
    EventSummaries.Empty();
    UE_LOG(LogTemp, Log, TEXT("Event summaries cleared"));
}

TArray<FString> UEventLogManager::GetFormattedLogs(int32 RecentCount) const
{
    TArray<FString> FormattedLogs;
    TArray<FEventLogEntry> LogsToFormat = (RecentCount > 0) ? GetRecentLogs(RecentCount) : EventLogs;
    
    for (const FEventLogEntry& Entry : LogsToFormat)
    {
        if (Entry.FormattedText.IsEmpty())
        {
            FormattedLogs.Add(FormatLogEntry(Entry));
        }
        else
        {
            FormattedLogs.Add(Entry.FormattedText);
        }
    }
    
    return FormattedLogs;
}

TArray<FString> UEventLogManager::GetFormattedEventSummaries(int32 RecentCount) const
{
    TArray<FString> FormattedSummaries;
    TArray<FEventSummary> SummariesToFormat = (RecentCount > 0) ? GetRecentEventSummaries(RecentCount) : EventSummaries;
    
    for (const FEventSummary& Summary : SummariesToFormat)
    {
        FormattedSummaries.Add(FormatEventSummary(Summary));
    }
    
    return FormattedSummaries;
}

// ==================== ヘルパー関数 ====================

FString UEventLogManager::FormatLogEntry(const FEventLogEntry& LogEntry) const
{
    switch (LogEntry.EventCategory)
    {
        case EEventCategory::Combat:
            return FormatCombatEntry(LogEntry);
        case EEventCategory::Gathering:
            return FormatGatheringEntry(LogEntry);
        case EEventCategory::Construction:
            return FormatConstructionEntry(LogEntry);
        default:
            return FString::Printf(TEXT("[%s] %s"), 
                                 *UEnum::GetValueAsString(LogEntry.EventType), 
                                 *LogEntry.LegacyEventData.FindRef(TEXT("Message")));
    }
}

FString UEventLogManager::FormatEventSummary(const FEventSummary& Summary) const
{
    return FString::Printf(TEXT("%s\n%s"), *Summary.Title, *Summary.ResultText);
}

FString UEventLogManager::FormatCombatEntry(const FEventLogEntry& LogEntry) const
{
    FString FormattedText;
    const FCombatEventData& CombatData = LogEntry.CombatData;
    
    // キャラクターが敵かプレイヤーかを判定
    bool bIsTargetPlayer = !CombatData.bIsPlayerAttacker; // 攻撃者の逆が被攻撃者
    
    switch (LogEntry.EventType)
    {
        case EEventLogType::Hit:
        case EEventLogType::Critical:
            {
                // ダメージの色を決定（被害者がプレイヤーなら赤、敵なら青）
                FString DamageColor = bIsTargetPlayer ? TEXT("#FF4444") : TEXT("#4444FF");
                FString CriticalText = (LogEntry.EventType == EEventLogType::Critical) ? TEXT(" クリティカル！") : TEXT("");
                
                // 攻撃者がプレイヤーの場合：武器攻撃 → （ダメージ青）
                // 攻撃者が敵の場合：（ダメージ赤） ← 武器攻撃
                if (CombatData.bIsPlayerAttacker)
                {
                    FormattedText = FString::Printf(TEXT("%s(%d/%d) ◄═══⚔️═══ %s(%d/%d)\n          %s攻撃 → <color=%s>（%d）</color>%s"), 
                                                  *CombatData.AttackerName, CombatData.AttackerHP, CombatData.AttackerMaxHP,
                                                  *CombatData.DefenderName, CombatData.DefenderHP, CombatData.DefenderMaxHP,
                                                  *CombatData.WeaponName, *DamageColor, CombatData.Damage, *CriticalText);
                }
                else
                {
                    FormattedText = FString::Printf(TEXT("%s(%d/%d) ◄═══⚔️═══ %s(%d/%d)\n          <color=%s>（%d）</color> ← %s攻撃%s"), 
                                                  *CombatData.DefenderName, CombatData.DefenderHP, CombatData.DefenderMaxHP,
                                                  *CombatData.AttackerName, CombatData.AttackerHP, CombatData.AttackerMaxHP,
                                                  *DamageColor, CombatData.Damage, *CombatData.WeaponName, *CriticalText);
                }
            }
            break;
            
        case EEventLogType::Miss:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 当たらなかった"), 
                                          *CombatData.AttackerName, *CombatData.WeaponName, *CombatData.DefenderName);
            break;
            
        case EEventLogType::Dodge:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 回避された"), 
                                          *CombatData.AttackerName, *CombatData.WeaponName, *CombatData.DefenderName);
            break;
            
        case EEventLogType::Parry:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 受け流された"), 
                                          *CombatData.AttackerName, *CombatData.WeaponName, *CombatData.DefenderName);
            break;
            
        case EEventLogType::Death:
            FormattedText = FString::Printf(TEXT("%sが死亡しました"), *CombatData.AttackerName);
            break;
            
        case EEventLogType::CombatStart:
        case EEventLogType::CombatEnd:
            FormattedText = LogEntry.LegacyEventData.FindRef(TEXT("AdditionalInfo"));
            break;
            
        default:
            FormattedText = FString::Printf(TEXT("[%s] %s"), 
                                          *UEnum::GetValueAsString(LogEntry.EventType), *CombatData.AttackerName);
            break;
    }
    
    return FormattedText;
}

FString UEventLogManager::FormatGatheringEntry(const FEventLogEntry& LogEntry) const
{
    const FGatheringEventData& GatheringData = LogEntry.GatheringData;
    
    switch (LogEntry.EventType)
    {
        case EEventLogType::GatheringSuccess:
            return FString::Printf(TEXT("🌿%sが%sで%s×%dを採集しました"), 
                                 *GatheringData.GathererName, *GatheringData.LocationName, 
                                 *GatheringData.ResourceType, GatheringData.Amount);
        case EEventLogType::GatheringFailed:
            return FString::Printf(TEXT("❌%sが%sでの採集に失敗しました"), 
                                 *GatheringData.GathererName, *GatheringData.LocationName);
        case EEventLogType::ResourceDepleted:
            return FString::Printf(TEXT("⚠️%sの%sが枯渇しました"), 
                                 *GatheringData.LocationName, *GatheringData.ResourceType);
        default:
            return FString::Printf(TEXT("🌿%s: %s"), 
                                 *UEnum::GetValueAsString(LogEntry.EventType), *GatheringData.ToString());
    }
}

FString UEventLogManager::FormatConstructionEntry(const FEventLogEntry& LogEntry) const
{
    const FConstructionEventData& ConstructionData = LogEntry.ConstructionData;
    
    switch (LogEntry.EventType)
    {
        case EEventLogType::ConstructionStart:
            return FString::Printf(TEXT("🏗️%sが%sの建設を開始しました"), 
                                 *ConstructionData.BuilderName, *ConstructionData.BuildingName);
        case EEventLogType::ConstructionComplete:
            return FString::Printf(TEXT("✅%sの建設が完了しました"), *ConstructionData.BuildingName);
        case EEventLogType::ConstructionProgress:
            return FString::Printf(TEXT("🔨%sの建設進行中（%.1f%%）"), 
                                 *ConstructionData.BuildingName, ConstructionData.Progress * 100.0f);
        default:
            return FString::Printf(TEXT("🏗️%s: %s"), 
                                 *UEnum::GetValueAsString(LogEntry.EventType), *ConstructionData.ToString());
    }
}

FString UEventLogManager::GetCharacterDisplayName(AC_IdleCharacter* Character) const
{
    if (!Character)
    {
        return TEXT("不明");
    }
    
    // IIdleCharacterInterface経由で名前取得
    if (Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
    {
        return IIdleCharacterInterface::Execute_GetCharacterName(Character);
    }
    
    return TEXT("名無し");
}

float UEventLogManager::GetCurrentEventTime() const
{
    if (GetWorld())
    {
        return GetWorld()->GetTimeSeconds() - CurrentEventStartTime;
    }
    return 0.0f;
}

void UEventLogManager::TrimLogsIfNeeded()
{
    if (EventLogs.Num() > MaxLogEntries)
    {
        int32 ExcessCount = EventLogs.Num() - MaxLogEntries;
        EventLogs.RemoveAt(0, ExcessCount);
        UE_LOG(LogTemp, Log, TEXT("Trimmed %d excess event log entries"), ExcessCount);
    }
}

void UEventLogManager::TrimSummariesIfNeeded()
{
    if (EventSummaries.Num() > MaxSummaryEntries)
    {
        int32 ExcessCount = EventSummaries.Num() - MaxSummaryEntries;
        EventSummaries.RemoveAt(0, ExcessCount);
        UE_LOG(LogTemp, Log, TEXT("Trimmed %d excess event summary entries"), ExcessCount);
    }
}

FEventLogEntry UEventLogManager::ConvertLegacyCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, 
                                                       AC_IdleCharacter* Target, const FString& WeaponOrItemName, 
                                                       int32 DamageValue, const FString& AdditionalInfo) const
{
    FEventLogEntry NewEntry;
    NewEntry.EventCategory = EEventCategory::Combat;
    NewEntry.Priority = EEventPriority::Normal;
    NewEntry.Timestamp = GetCurrentEventTime();
    
    // 戦闘ログタイプを新しいイベントログタイプに変換
    switch (CombatLogType)
    {
        case ECombatLogType::Attack:
            NewEntry.EventType = EEventLogType::Attack;
            break;
        case ECombatLogType::Hit:
            NewEntry.EventType = EEventLogType::Hit;
            break;
        case ECombatLogType::Miss:
            NewEntry.EventType = EEventLogType::Miss;
            break;
        case ECombatLogType::Dodge:
            NewEntry.EventType = EEventLogType::Dodge;
            break;
        case ECombatLogType::Parry:
            NewEntry.EventType = EEventLogType::Parry;
            break;
        case ECombatLogType::Critical:
            NewEntry.EventType = EEventLogType::Critical;
            break;
        case ECombatLogType::Damage:
            NewEntry.EventType = EEventLogType::Damage;
            break;
        case ECombatLogType::Death:
            NewEntry.EventType = EEventLogType::Death;
            break;
        case ECombatLogType::CombatStart:
            NewEntry.EventType = EEventLogType::CombatStart;
            break;
        case ECombatLogType::CombatEnd:
            NewEntry.EventType = EEventLogType::CombatEnd;
            break;
        default:
            NewEntry.EventType = EEventLogType::Debug;
            break;
    }
    
    // 戦闘データを設定
    if (Actor) NewEntry.CombatData.AttackerName = GetCharacterDisplayName(Actor);
    if (Target) NewEntry.CombatData.DefenderName = GetCharacterDisplayName(Target);
    NewEntry.CombatData.WeaponName = WeaponOrItemName;
    NewEntry.CombatData.Damage = DamageValue;
    NewEntry.CombatData.bIsCritical = (CombatLogType == ECombatLogType::Critical);
    
    // プレイヤーかどうかを判定
    if (Actor && Actor->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
    {
        FString AttackerRace = Actor->GetCharacterRace();
        NewEntry.CombatData.bIsPlayerAttacker = (AttackerRace == TEXT("human"));
    }
    
    // 関連キャラクターを設定
    if (Actor) NewEntry.InvolvedCharacters.Add(Actor);
    if (Target) NewEntry.InvolvedCharacters.Add(Target);
    
    // 後方互換性のためのレガシーデータ
    NewEntry.LegacyEventData.Add(TEXT("WeaponOrItemName"), WeaponOrItemName);
    NewEntry.LegacyEventData.Add(TEXT("DamageValue"), FString::FromInt(DamageValue));
    NewEntry.LegacyEventData.Add(TEXT("AdditionalInfo"), AdditionalInfo);
    
    return NewEntry;
}

EEventCategory UEventLogManager::DetermineEventCategory(EEventLogType EventType) const
{
    switch (EventType)
    {
        case EEventLogType::Attack:
        case EEventLogType::Hit:
        case EEventLogType::Miss:
        case EEventLogType::Dodge:
        case EEventLogType::Parry:
        case EEventLogType::Critical:
        case EEventLogType::Damage:
        case EEventLogType::Death:
        case EEventLogType::CombatStart:
        case EEventLogType::CombatEnd:
            return EEventCategory::Combat;
            
        case EEventLogType::GatheringStart:
        case EEventLogType::GatheringSuccess:
        case EEventLogType::GatheringFailed:
        case EEventLogType::ResourceDepleted:
        case EEventLogType::RareItemFound:
            return EEventCategory::Gathering;
            
        case EEventLogType::ConstructionStart:
        case EEventLogType::ConstructionProgress:
        case EEventLogType::ConstructionComplete:
        case EEventLogType::ConstructionFailed:
        case EEventLogType::BuildingUpgrade:
            return EEventCategory::Construction;
            
        case EEventLogType::QuestReceived:
        case EEventLogType::QuestProgress:
        case EEventLogType::QuestComplete:
        case EEventLogType::QuestFailed:
            return EEventCategory::Quest;
            
        case EEventLogType::LevelUp:
        case EEventLogType::SkillGained:
        case EEventLogType::ItemEquipped:
        case EEventLogType::StatusChanged:
            return EEventCategory::Character;
            
        default:
            return EEventCategory::System;
    }
}