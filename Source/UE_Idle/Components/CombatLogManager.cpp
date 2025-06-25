#include "CombatLogManager.h"
#include "../Actor/C_IdleCharacter.h"
#include "Engine/World.h"

UCombatLogManager::UCombatLogManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    CombatStartTime = 0.0f;
}

void UCombatLogManager::BeginPlay()
{
    Super::BeginPlay();
    CombatStartTime = GetWorld()->GetTimeSeconds();
}

void UCombatLogManager::AddCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, AC_IdleCharacter* Target, 
                                   const FString& WeaponOrItemName, int32 DamageValue, const FString& AdditionalInfo)
{
    FCombatLogEntry NewEntry;
    NewEntry.LogType = CombatLogType;
    NewEntry.Actor = Actor;
    NewEntry.Target = Target;
    NewEntry.WeaponOrItemName = WeaponOrItemName;
    NewEntry.DamageValue = DamageValue;
    NewEntry.AdditionalInfo = AdditionalInfo;
    NewEntry.Timestamp = GetCurrentCombatTime();
    
    if (bAutoFormatLogs)
    {
        NewEntry.FormattedText = FormatLogEntry(NewEntry);
    }
    
    CombatLogs.Add(NewEntry);
    TrimLogsIfNeeded();
    
    // イベント発火
    OnCombatLogAdded.Broadcast(NewEntry);
    
    // デバッグログ出力
    UE_LOG(LogTemp, Log, TEXT("CombatLog: %s"), *NewEntry.FormattedText);
}

void UCombatLogManager::AddCombatCalculationLog(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
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
        
        // ダメージログも追加
        if (Result.FinalDamage > 0)
        {
            AddCombatLog(ECombatLogType::Damage, Defender, nullptr, TEXT(""), Result.FinalDamage);
        }
    }
}

TArray<FCombatLogEntry> UCombatLogManager::GetRecentLogs(int32 Count) const
{
    TArray<FCombatLogEntry> RecentLogs;
    
    if (Count <= 0)
    {
        return CombatLogs;
    }
    
    int32 StartIndex = FMath::Max(0, CombatLogs.Num() - Count);
    for (int32 i = StartIndex; i < CombatLogs.Num(); i++)
    {
        RecentLogs.Add(CombatLogs[i]);
    }
    
    return RecentLogs;
}

TArray<FCombatLogEntry> UCombatLogManager::GetLogsByType(ECombatLogType CombatLogType) const
{
    return CombatLogs.FilterByPredicate([CombatLogType](const FCombatLogEntry& Entry) {
        return Entry.LogType == CombatLogType;
    });
}

void UCombatLogManager::ClearLogs()
{
    CombatLogs.Empty();
    OnCombatLogCleared.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("Combat logs cleared"));
}

TArray<FString> UCombatLogManager::GetFormattedLogs(int32 RecentCount) const
{
    TArray<FString> FormattedLogs;
    TArray<FCombatLogEntry> LogsToFormat = (RecentCount > 0) ? GetRecentLogs(RecentCount) : CombatLogs;
    
    for (const FCombatLogEntry& Entry : LogsToFormat)
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

void UCombatLogManager::LogCombatStart(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationName)
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
    
    FString CombatInfo = FString::Printf(TEXT("%sで戦闘開始！ 味方：%s vs 敵：%s"), 
                                        *LocationName, *AllyNames, *EnemyNames);
    
    AddCombatLog(ECombatLogType::CombatStart, nullptr, nullptr, TEXT(""), 0, CombatInfo);
}

void UCombatLogManager::LogCombatEnd(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers, float CombatDuration)
{
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
    
    FString CombatResult = FString::Printf(TEXT("戦闘終了！ 勝者：%s (戦闘時間: %.1f秒)"), 
                                          *WinnerNames, CombatDuration);
    
    AddCombatLog(ECombatLogType::CombatEnd, nullptr, nullptr, TEXT(""), 0, CombatResult);
}

FString UCombatLogManager::FormatLogEntry(const FCombatLogEntry& LogEntry) const
{
    FString FormattedText;
    FString ActorName = GetCharacterDisplayName(LogEntry.Actor);
    FString TargetName = GetCharacterDisplayName(LogEntry.Target);
    
    switch (LogEntry.LogType)
    {
        case ECombatLogType::Attack:
            if (LogEntry.Target)
            {
                FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃"), 
                                              *ActorName, *LogEntry.WeaponOrItemName, *TargetName);
            }
            break;
            
        case ECombatLogType::Hit:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 命中！ %dダメージ"), 
                                          *ActorName, *LogEntry.WeaponOrItemName, *TargetName, LogEntry.DamageValue);
            break;
            
        case ECombatLogType::Miss:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 当たらなかった"), 
                                          *ActorName, *LogEntry.WeaponOrItemName, *TargetName);
            break;
            
        case ECombatLogType::Dodge:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 回避された"), 
                                          *ActorName, *LogEntry.WeaponOrItemName, *TargetName);
            break;
            
        case ECombatLogType::Parry:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → 受け流された"), 
                                          *ActorName, *LogEntry.WeaponOrItemName, *TargetName);
            break;
            
        case ECombatLogType::Critical:
            FormattedText = FString::Printf(TEXT("%sが%sで%sを攻撃 → クリティカルヒット！ %dダメージ"), 
                                          *ActorName, *LogEntry.WeaponOrItemName, *TargetName, LogEntry.DamageValue);
            break;
            
        case ECombatLogType::Damage:
            FormattedText = FString::Printf(TEXT("%sが%dダメージを受けた"), 
                                          *ActorName, LogEntry.DamageValue);
            break;
            
        case ECombatLogType::Death:
            FormattedText = FString::Printf(TEXT("%sが死亡しました"), *ActorName);
            break;
            
        case ECombatLogType::CombatStart:
        case ECombatLogType::CombatEnd:
            FormattedText = LogEntry.AdditionalInfo;
            break;
            
        default:
            FormattedText = FString::Printf(TEXT("[%s] %s"), 
                                          *UEnum::GetValueAsString(LogEntry.LogType), *ActorName);
            break;
    }
    
    // タイムスタンプ付加
    if (LogEntry.Timestamp > 0.0f)
    {
        FormattedText = FString::Printf(TEXT("[%.1fs] %s"), LogEntry.Timestamp, *FormattedText);
    }
    
    return FormattedText;
}

FString UCombatLogManager::GetCharacterDisplayName(AC_IdleCharacter* Character) const
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

float UCombatLogManager::GetCurrentCombatTime() const
{
    if (GetWorld())
    {
        return GetWorld()->GetTimeSeconds() - CombatStartTime;
    }
    return 0.0f;
}

void UCombatLogManager::TrimLogsIfNeeded()
{
    if (CombatLogs.Num() > MaxLogEntries)
    {
        int32 ExcessCount = CombatLogs.Num() - MaxLogEntries;
        CombatLogs.RemoveAt(0, ExcessCount);
        UE_LOG(LogTemp, Log, TEXT("Trimmed %d excess combat log entries"), ExcessCount);
    }
}