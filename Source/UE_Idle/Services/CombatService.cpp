#include "CombatService.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CombatComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Managers/BattleSystemManager.h"
#include "../C_PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UCombatService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 統計情報初期化
    ResetStatistics();
    
    // 戦闘力キャッシュ初期化
    CombatPowerCache.Empty();
    LastCombatCacheUpdate = 0.0f;
    
    // 場所・敵データ初期化
    InitializeLocationEnemyData();
    
    UE_LOG(LogTemp, Log, TEXT("⚔️ CombatService initialized successfully"));
}

void UCombatService::Deinitialize()
{
    // クリーンアップ
    CombatPowerCache.Empty();
    LocationDangerMap.Empty();
    LocationEnemiesMap.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("⚔️ CombatService deinitialized"));
    
    Super::Deinitialize();
}

bool UCombatService::InitiateCombat(AC_IdleCharacter* Character, const FString& Location)
{
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Error, TEXT("⚔️❌ CombatService: Invalid character provided"));
        return false;
    }

    if (Location.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("⚔️❌ CombatService: Empty combat location"));
        return false;
    }

    // 戦闘可能性チェック
    if (!CanCharacterFight(Character, Location))
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("⚔️⚠️ CombatService: Character %s cannot fight at %s"),
            *Character->GetCharacterName(), *Location);
        return false;
    }

    // 統計更新
    TotalCombatRequests++;
    
    // 既存の戦闘ロジックを実行
    bool bSuccess = ExecuteExistingCombatLogic(Character, Location);
    
    if (bSuccess)
    {
        SuccessfulCombatStarts++;
        
        // 戦闘時間を統計に追加
        float CombatTime = EstimateCombatDuration(Character, Location);
        TotalCombatTime += CombatTime;
        
        UE_LOG(LogTemp, VeryVerbose, 
            TEXT("⚔️✅ CombatService: Character %s starting combat at %s (estimated duration: %.1fs)"),
            *Character->GetCharacterName(), *Location, CombatTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("⚔️⚠️ CombatService: Failed to start combat for character %s at %s"),
            *Character->GetCharacterName(), *Location);
    }
    
    return bSuccess;
}

bool UCombatService::IsCharacterInCombat(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    // 既存の戦闘状態チェックを使用
    return CheckExistingCombatState(Character);
}

bool UCombatService::ProcessCombatProgress(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    // 戦闘中でない場合は完了とみなす
    if (!IsCharacterInCombat(Character))
    {
        return true;
    }
    
    // 実際の戦闘進行は BattleSystemManager が管理
    // ここでは簡易的に進行状況をチェック
    return false;
}

bool UCombatService::EndCombat(AC_IdleCharacter* Character, bool bVictory)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    if (!IsCharacterInCombat(Character))
    {
        return true; // 既に戦闘中でない
    }
    
    // 統計更新
    if (bVictory)
    {
        Victories++;
    }
    else
    {
        Defeats++;
    }
    
    // TeamComponentを通じて戦闘終了処理
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex != -1)
    {
        // PlayerControllerからTeamComponentを取得
        AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
            UGameplayStatics::GetPlayerController(GetWorld(), 0));
        
        if (PlayerController)
        {
            UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
            if (TeamComp)
            {
                TeamComp->EndCombat(TeamIndex);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, 
        TEXT("⚔️ CombatService: Combat ended for character %s (%s)"),
        *Character->GetCharacterName(), bVictory ? TEXT("Victory") : TEXT("Defeat"));
    
    return true;
}

float UCombatService::CalculateCombatPower(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return 1.0f; // デフォルト戦闘力
    }
    
    // キャッシュチェック
    if (IsCombatCacheValid() && CombatPowerCache.Contains(Character))
    {
        return CombatPowerCache[Character];
    }
    
    // 基本戦闘力を計算
    float BasePower = CalculateBaseCombatPower(Character);
    
    // キャッシュに保存
    CombatPowerCache.Add(Character, BasePower);
    
    return BasePower;
}

float UCombatService::EstimateCombatDuration(AC_IdleCharacter* Character, const FString& Location)
{
    if (!IsValid(Character))
    {
        return 5.0f; // デフォルト戦闘時間
    }
    
    float CharacterPower = CalculateCombatPower(Character);
    float EnemyPower = EstimateEnemyStrength(Location);
    
    // 戦闘力比から戦闘時間を推定
    float PowerRatio = CharacterPower / FMath::Max(EnemyPower, 1.0f);
    
    // 基本戦闘時間5秒から、戦闘力比に応じて調整
    float EstimatedTime = 5.0f / FMath::Max(PowerRatio, 0.2f);
    
    return FMath::Clamp(EstimatedTime, 2.0f, 15.0f); // 2-15秒の範囲
}

bool UCombatService::CanCharacterFight(AC_IdleCharacter* Character, const FString& Location)
{
    if (!IsValid(Character) || Location.IsEmpty())
    {
        return false;
    }
    
    // 戦闘可能な場所かチェック
    TArray<FString> CombatLocations = GetCombatLocations();
    if (!CombatLocations.Contains(Location))
    {
        return false;
    }
    
    // キャラクターの体力チェック
    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (StatusComp)
    {
        float CurrentHealth = StatusComp->GetCurrentHealth();
        if (CurrentHealth < 20.0f) // 体力が20未満では戦闘不可
        {
            return false;
        }
    }
    
    // 既に戦闘中でないかチェック
    if (IsCharacterInCombat(Character))
    {
        return false;
    }
    
    return true;
}

FCharacterAction UCombatService::DecideOptimalCombatAction(AC_IdleCharacter* Character, const FCharacterSituation& Situation)
{
    FCharacterAction CombatAction;
    
    if (!IsValid(Character))
    {
        CombatAction.ActionType = ECharacterActionType::Wait;
        CombatAction.ActionReason = TEXT("Invalid character for combat");
        return CombatAction;
    }
    
    // 基本的な攻撃行動（将来的に個性化予定）
    CombatAction.ActionType = ECharacterActionType::AttackEnemy;
    CombatAction.TargetLocation = Situation.CurrentLocation;
    CombatAction.ExpectedDuration = EstimateCombatDuration(Character, Situation.CurrentLocation);
    CombatAction.ActionReason = TEXT("Optimal combat action: attack");
    
    return CombatAction;
}

TArray<FString> UCombatService::GetCombatLocations()
{
    // 戦闘可能な場所（現在はplainsのみ）
    return {TEXT("plains")};
}

int32 UCombatService::GetLocationDangerLevel(const FString& Location)
{
    if (LocationDangerMap.Contains(Location))
    {
        return LocationDangerMap[Location];
    }
    
    return 1; // デフォルト危険度
}

TArray<FString> UCombatService::GetEnemyTypesAt(const FString& Location)
{
    if (LocationEnemiesMap.Contains(Location))
    {
        return LocationEnemiesMap[Location];
    }
    
    return TArray<FString>(); // 敵なし
}

TArray<FString> UCombatService::GetExpectedLoot(const FString& Location, AC_IdleCharacter* Character)
{
    TArray<FString> Loot;
    
    // 場所に応じた戦利品（簡易版）
    if (Location == TEXT("plains"))
    {
        Loot.Add(TEXT("monster_meat"));
        Loot.Add(TEXT("animal_hide"));
    }
    
    return Loot;
}

FString UCombatService::GetCombatStatistics()
{
    float WinRate = (Victories + Defeats > 0) ? 
        (float)Victories / (Victories + Defeats) * 100.0f : 0.0f;
    
    float SuccessRate = (TotalCombatRequests > 0) ? 
        (float)SuccessfulCombatStarts / TotalCombatRequests * 100.0f : 0.0f;
    
    float AverageCombatTime = (SuccessfulCombatStarts > 0) ? 
        TotalCombatTime / SuccessfulCombatStarts : 0.0f;
    
    return FString::Printf(
        TEXT("Combat Stats: %d requests (%.1f%% success), %d victories, %d defeats (%.1f%% win rate), avg time: %.1fs"),
        TotalCombatRequests, SuccessRate, Victories, Defeats, WinRate, AverageCombatTime);
}

FString UCombatService::GetCharacterCombatHistory(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return TEXT("Invalid character");
    }
    
    // 簡易版 - 将来的にキャラクター別履歴追跡予定
    return FString::Printf(
        TEXT("Character %s combat history: (Feature not implemented yet)"),
        *Character->GetCharacterName());
}

bool UCombatService::ExecuteExistingCombatLogic(AC_IdleCharacter* Character, const FString& Location)
{
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    UCombatComponent* CombatComp = GetCombatComponent();
    if (!CombatComp)
    {
        return false;
    }
    
    // StartCombatは3つの引数が必要 - 現在は簡易実装でスキップ
    // CombatComp->StartCombat(AllyTeam, EnemyTeam, LocationId);
    UE_LOG(LogTemp, Warning, TEXT("🗡️ CombatService: StartCombat needs proper implementation with teams"));
    return true;
}

bool UCombatService::CheckExistingCombatState(AC_IdleCharacter* Character)
{
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    UCombatComponent* CombatComp = GetCombatComponent();
    if (!CombatComp)
    {
        return false;
    }
    
    // IsInCombatは引数なしメソッド
    return CombatComp->IsInCombat();
}

int32 UCombatService::GetCharacterTeamIndex(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return -1;
    }
    
    // PlayerControllerからTeamComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return -1;
    }
    
    UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
    if (!TeamComp)
    {
        return -1;
    }
    
    // キャラクターが所属するチームを検索
    for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
    {
        FTeam Team = TeamComp->GetTeam(i);
        if (Team.Members.Contains(Character))
        {
            return i;
        }
    }
    
    return -1;
}

float UCombatService::CalculateBaseCombatPower(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return 1.0f;
    }
    
    // CharacterStatusComponentから戦闘力を取得
    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (!StatusComp)
    {
        return 1.0f;
    }
    
    // 派生ステータスから戦闘力を取得
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.CombatPower;
}

float UCombatService::ApplyLocationCombatModifier(float BasePower, const FString& Location)
{
    // 場所別の戦闘修正
    if (Location == TEXT("base"))
    {
        return BasePower * 1.1f; // 拠点では若干有利
    }
    else if (Location == TEXT("plains"))
    {
        return BasePower * 1.0f; // 標準
    }
    
    return BasePower * 0.9f; // 未知の場所では不利
}

float UCombatService::EstimateEnemyStrength(const FString& Location)
{
    // 場所に応じた敵の強さ
    if (Location == TEXT("plains"))
    {
        return 15.0f; // 平野の敵の戦闘力
    }
    
    return 10.0f; // デフォルト敵戦闘力
}

UCombatComponent* UCombatService::GetCombatComponent()
{
    // PlayerControllerからCombatComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return nullptr;
    }
    
    return PlayerController->FindComponentByClass<UCombatComponent>();
}

UBattleSystemManager* UCombatService::GetBattleSystemManager()
{
    // GameInstanceからBattleSystemManagerを取得
    if (!GetWorld() || !GetWorld()->GetGameInstance())
    {
        return nullptr;
    }
    
    return GetWorld()->GetGameInstance()->GetSubsystem<UBattleSystemManager>();
}

bool UCombatService::AreReferencesValid()
{
    return IsValid(GetCombatComponent());
}

void UCombatService::ResetStatistics()
{
    TotalCombatRequests = 0;
    SuccessfulCombatStarts = 0;
    Victories = 0;
    Defeats = 0;
    TotalCombatTime = 0.0f;
}

void UCombatService::UpdateCombatCache()
{
    if (!IsCombatCacheValid())
    {
        CombatPowerCache.Empty();
        LastCombatCacheUpdate = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    }
}

bool UCombatService::IsCombatCacheValid() const
{
    if (!GetWorld())
    {
        return false;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    return (CurrentTime - LastCombatCacheUpdate) < CombatCacheValidDuration;
}

void UCombatService::InitializeLocationEnemyData()
{
    // 場所危険度の初期化
    LocationDangerMap.Empty();
    LocationDangerMap.Add(TEXT("base"), 1);   // 安全
    LocationDangerMap.Add(TEXT("plains"), 3); // 中程度の危険
    
    // 場所別敵情報の初期化
    LocationEnemiesMap.Empty();
    LocationEnemiesMap.Add(TEXT("plains"), {TEXT("goblin"), TEXT("wolf")});
    
    UE_LOG(LogTemp, Log, TEXT("⚔️ CombatService: Location and enemy data initialized"));
}