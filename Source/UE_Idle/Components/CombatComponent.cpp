#include "CombatComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "CombatLogManager.h"
#include "ActionSystemComponent.h"
#include "../Managers/CharacterPresetManager.h"
#include "Engine/World.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    CurrentState = ECombatState::Inactive;
    CombatLocationId = TEXT("");
    CombatStartTime = 0.0f;
    TotalActionCount = 0;
    TotalDamageDealt = 0;
    
    bAutoStartCombat = true;
    CombatPreparationTime = 2.0f;
    MaxEnemiesPerLocation = 3;
    
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeSubComponents();
}

bool UCombatComponent::StartCombat(const FTeam& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationId)
{
    if (CurrentState != ECombatState::Inactive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Combat already active, cannot start new combat"));
        return false;
    }

    if (AllyTeam.Members.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start combat with empty ally team"));
        return false;
    }

    if (EnemyTeam.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start combat with empty enemy team"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Starting combat with %d allies vs %d enemies at location %s"), 
        AllyTeam.Members.Num(), EnemyTeam.Num(), *LocationId);

    // チーム設定
    AllyTeamMembers = AllyTeam.Members;
    EnemyTeamMembers = EnemyTeam;
    CombatLocationId = LocationId;
    
    // 統計リセット
    TotalActionCount = 0;
    TotalDamageDealt = 0;
    
    SetCombatState(ECombatState::Preparing, FString::Printf(TEXT("味方%d人 vs 敵%d人で%sで戦闘開始"), 
        AllyTeamMembers.Num(), EnemyTeamMembers.Num(), *LocationId));

    // 戦闘ログに開始記録
    if (CombatLogManager)
    {
        FString LocationName = GetCombatLocationName();
        CombatLogManager->LogCombatStart(AllyTeamMembers, EnemyTeamMembers, LocationName);
    }

    // 準備時間後に戦闘開始
    if (bAutoStartCombat)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(
                PreparationTimerHandle,
                this,
                &UCombatComponent::StartCombatAfterPreparation,
                CombatPreparationTime,
                false
            );
        }
    }

    return true;
}

void UCombatComponent::ForceEndCombat()
{
    if (CurrentState == ECombatState::Inactive)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Force ending combat"));
    
    SetCombatState(ECombatState::Completed, TEXT("戦闘が強制終了されました"));
    CleanupAfterCombat();
}

float UCombatComponent::GetCombatDuration() const
{
    if (CurrentState == ECombatState::Inactive)
    {
        return 0.0f;
    }

    return GetWorld() ? (GetWorld()->GetTimeSeconds() - CombatStartTime) : 0.0f;
}

FString UCombatComponent::GetCombatLocationName() const
{
    // LocationEventManagerで処理されるため、簡素にＩＤを返す
    return CombatLocationId;
}

TArray<FString> UCombatComponent::GetCombatLogs(int32 RecentCount) const
{
    if (CombatLogManager)
    {
        return CombatLogManager->GetFormattedLogs(RecentCount);
    }
    return TArray<FString>();
}

void UCombatComponent::SetCombatState(ECombatState NewState, const FString& StateInfo)
{
    if (CurrentState == NewState)
    {
        return;
    }

    ECombatState OldState = CurrentState;
    CurrentState = NewState;

    UE_LOG(LogTemp, Log, TEXT("Combat state changed: %s -> %s (%s)"), 
        *UEnum::GetValueAsString(OldState), 
        *UEnum::GetValueAsString(NewState), 
        *StateInfo);

    OnCombatStateChanged.Broadcast(OldState, NewState, StateInfo);
}

bool UCombatComponent::SpawnEnemiesAtLocation(const FString& LocationId)
{
    // 非推奨: 敵のスポーンはLocationEventManagerで処理される
    // このメソッドは互換性のために残しているが、使用しない
    UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesAtLocation is deprecated. Use LocationEventManager instead."));
    return false;
}

bool UCombatComponent::IsCombatReadyToStart() const
{
    return AllyTeamMembers.Num() > 0 && EnemyTeamMembers.Num() > 0;
}

void UCombatComponent::CheckCombatCompletion()
{
    TArray<AC_IdleCharacter*> AliveAllies = GetAliveMembers(AllyTeamMembers);
    TArray<AC_IdleCharacter*> AliveEnemies = GetAliveMembers(EnemyTeamMembers);

    if (AliveAllies.Num() == 0 || AliveEnemies.Num() == 0)
    {
        // 戦闘終了
        TArray<AC_IdleCharacter*> Winners = (AliveAllies.Num() > 0) ? AliveAllies : AliveEnemies;
        TArray<AC_IdleCharacter*> Losers = (AliveAllies.Num() == 0) ? AllyTeamMembers : EnemyTeamMembers;
        
        float Duration = GetCombatDuration();
        
        SetCombatState(ECombatState::Completed, 
            FString::Printf(TEXT("戦闘終了！ 勝者：%d人"), Winners.Num()));

        // 戦闘ログに終了記録
        if (CombatLogManager)
        {
            CombatLogManager->LogCombatEnd(Winners, Losers, Duration);
        }

        OnCombatCompleted.Broadcast(Winners, Losers, Duration);
        CleanupAfterCombat();
    }
}

void UCombatComponent::OnActionSystemCharacterAction(AC_IdleCharacter* Actor, AC_IdleCharacter* Target, const FCombatCalculationResult& Result)
{
    TotalActionCount++;
    TotalDamageDealt += Result.FinalDamage;

    // 武器名取得（簡易実装）
    FString WeaponName = TEXT("不明");
    // TODO: ActionSystemから武器情報を取得する仕組みが必要

    OnCombatAction.Broadcast(Actor, Target, WeaponName, Result);
    
    // 戦闘終了チェック
    CheckCombatCompletion();
}

void UCombatComponent::OnActionSystemCharacterDeath(AC_IdleCharacter* Character)
{
    // 戦闘終了チェック
    CheckCombatCompletion();
}

void UCombatComponent::StartCombatAfterPreparation()
{
    if (CurrentState != ECombatState::Preparing)
    {
        return;
    }

    if (!IsCombatReadyToStart())
    {
        UE_LOG(LogTemp, Error, TEXT("Combat not ready to start"));
        SetCombatState(ECombatState::Inactive);
        return;
    }

    CombatStartTime = GetWorld()->GetTimeSeconds();
    SetCombatState(ECombatState::InProgress, TEXT("戦闘開始！"));

    // ActionSystemに戦闘参加者を登録
    if (ActionSystemComponent)
    {
        ActionSystemComponent->RegisterTeam(AllyTeamMembers, EnemyTeamMembers);
        ActionSystemComponent->StartActionSystem();
    }
}

void UCombatComponent::InitializeSubComponents()
{
    // CombatLogManager作成
    CombatLogManager = NewObject<UCombatLogManager>(GetOwner());
    if (CombatLogManager)
    {
        CombatLogManager->RegisterComponent();
    }

    // ActionSystemComponent作成
    ActionSystemComponent = NewObject<UActionSystemComponent>(GetOwner());
    if (ActionSystemComponent)
    {
        ActionSystemComponent->RegisterComponent();
        ActionSystemComponent->SetCombatLogManager(CombatLogManager);
        
        // イベントバインド
        ActionSystemComponent->OnCharacterAction.AddDynamic(
            this, &UCombatComponent::OnActionSystemCharacterAction);
        ActionSystemComponent->OnCharacterDeath.AddDynamic(
            this, &UCombatComponent::OnActionSystemCharacterDeath);
    }
}

void UCombatComponent::CleanupAfterCombat()
{
    // ActionSystem停止
    if (ActionSystemComponent)
    {
        ActionSystemComponent->StopActionSystem();
        ActionSystemComponent->ClearAllCharacters();
    }

    // タイマークリア
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PreparationTimerHandle);
    }

    // 敵キャラクターの削除はLocationEventManagerが処理
    // CombatComponentは直接敵を管理しない

    // データクリア
    AllyTeamMembers.Empty();
    EnemyTeamMembers.Empty();
    CombatLocationId = TEXT("");
    
    SetCombatState(ECombatState::Inactive);
}

TArray<AC_IdleCharacter*> UCombatComponent::GetAliveMembers(const TArray<AC_IdleCharacter*>& Team) const
{
    TArray<AC_IdleCharacter*> AliveMembers;
    
    for (AC_IdleCharacter* Member : Team)
    {
        if (IsValid(Member))
        {
            // HPシステム実装後は実際の生存チェック
            // 現在は仮で全員生存とする
            AliveMembers.Add(Member);
        }
    }
    
    return AliveMembers;
}