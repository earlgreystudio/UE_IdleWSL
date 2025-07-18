#include "CombatComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../C_PlayerController.h"
#include "EventLogManager.h"
#include "ActionSystemComponent.h"
#include "CharacterStatusComponent.h"
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
    
    // 全キャラクターのHP状態をログ出力（安全チェック付き）
    UE_LOG(LogTemp, Log, TEXT("=== ALLY TEAM HP STATUS ==="));
    if (AllyTeam.Members.IsValidIndex(0))
    {
        for (int32 i = 0; i < AllyTeam.Members.Num(); i++)
        {
            if (IsValid(AllyTeam.Members[i]))
            {
                if (UCharacterStatusComponent* StatusComp = AllyTeam.Members[i]->GetStatusComponent())
                {
                    float CurrentHP = StatusComp->GetCurrentHealth();
                    float MaxHP = StatusComp->GetMaxHealth();
                    UE_LOG(LogTemp, Log, TEXT("Ally %d: %s - HP: %f/%f"), i, 
                        *AllyTeam.Members[i]->GetName(), CurrentHP, MaxHP);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Ally %d: %s - NO STATUS COMPONENT"), i, 
                        *AllyTeam.Members[i]->GetName());
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Ally %d: INVALID CHARACTER"), i);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Ally team is empty"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("=== ENEMY TEAM HP STATUS ==="));
    if (EnemyTeam.IsValidIndex(0))
    {
        for (int32 i = 0; i < EnemyTeam.Num(); i++)
        {
            if (IsValid(EnemyTeam[i]))
            {
                if (UCharacterStatusComponent* StatusComp = EnemyTeam[i]->GetStatusComponent())
                {
                    float CurrentHP = StatusComp->GetCurrentHealth();
                    float MaxHP = StatusComp->GetMaxHealth();
                    UE_LOG(LogTemp, Log, TEXT("Enemy %d: %s - HP: %f/%f"), i, 
                        *EnemyTeam[i]->GetName(), CurrentHP, MaxHP);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Enemy %d: %s - NO STATUS COMPONENT"), i, 
                        *EnemyTeam[i]->GetName());
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Enemy %d: INVALID CHARACTER"), i);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Enemy team is empty"));
    }

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
    if (EventLogManager)
    {
        FString LocationName = GetCombatLocationName();
        EventLogManager->LogCombatStart(AllyTeamMembers, EnemyTeamMembers, LocationName);
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
    if (EventLogManager)
    {
        return EventLogManager->GetFormattedLogs(RecentCount);
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
    // 既に完了済みの場合は重複処理を防ぐ
    if (CurrentState == ECombatState::Completed || CurrentState == ECombatState::Inactive)
    {
        return;
    }

    TArray<AC_IdleCharacter*> AliveAllies = GetAliveMembers(AllyTeamMembers);
    TArray<AC_IdleCharacter*> AliveEnemies = GetAliveMembers(EnemyTeamMembers);

    if (AliveAllies.Num() == 0 || AliveEnemies.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Combat ending - One side has no alive members"));
        
        // 戦闘終了処理を順序制御で実行
        ExecuteCombatEndSequence(AliveAllies, AliveEnemies);
    }
    else
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Combat continues - Both sides have alive members"));
    }
}

void UCombatComponent::ExecuteCombatEndSequence(const TArray<AC_IdleCharacter*>& AliveAllies, const TArray<AC_IdleCharacter*>& AliveEnemies)
{
    UE_LOG(LogTemp, Log, TEXT("=== Starting Combat End Sequence ==="));
    
    // Step 1: 勝者・敗者を決定（ActionSystemはまだ動作中）
    TArray<AC_IdleCharacter*> Winners = (AliveAllies.Num() > 0) ? AliveAllies : AliveEnemies;
    TArray<AC_IdleCharacter*> Losers = (AliveAllies.Num() == 0) ? AllyTeamMembers : EnemyTeamMembers;
    float Duration = GetCombatDuration();
    
    // Step 2: 状態を完了に変更
    SetCombatState(ECombatState::Completed, 
        FString::Printf(TEXT("戦闘終了！ 勝者：%d人"), Winners.Num()));
    
    // Step 3: ログ記録（同期的に実行）
    // PlayerControllerのEventLogManagerを直接使用して確実に統一
    if (UWorld* World = GetWorld())
    {
        if (AC_PlayerController* PC = Cast<AC_PlayerController>(World->GetFirstPlayerController()))
        {
            if (PC->EventLogManager)
            {
                UE_LOG(LogTemp, Log, TEXT("Recording combat end log - Winners: %d, Losers: %d"), 
                    Winners.Num(), Losers.Num());
                UE_LOG(LogTemp, Error, TEXT("CombatComponent using PC->EventLogManager: %p"), (void*)PC->EventLogManager);
                PC->EventLogManager->LogCombatEnd(Winners, Losers, Duration);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("PC->EventLogManager is NULL"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("PlayerController not found"));
        }
    }
    
    // Step 4: 短時間後にActionSystemを停止（ログ記録完了を待つ）
    if (GetWorld())
    {
        FTimerHandle ActionStopTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(ActionStopTimerHandle, [this]()
        {
            UE_LOG(LogTemp, Log, TEXT("Stopping ActionSystem after log recording"));
            StopActionSystemSafely();
        }, 0.1f, false);
    }
    
    // Step 5: さらに後でUI更新とクリーンアップを実行
    if (GetWorld())
    {
        FTimerHandle CleanupTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(CleanupTimerHandle, [this, Winners, Losers, Duration]()
        {
            // UI更新
            OnCombatCompleted.Broadcast(Winners, Losers, Duration);
            
            // 最終クリーンアップ（キャラクター削除を含む）
            CleanupAfterCombat();
            
            UE_LOG(LogTemp, Log, TEXT("=== Combat End Sequence Completed ==="));
        }, 0.2f, false);
    }
}

void UCombatComponent::StopActionSystemSafely()
{
    if (ActionSystemComponent && ActionSystemComponent->IsSystemActive())
    {
        UE_LOG(LogTemp, Log, TEXT("Stopping ActionSystem from CombatComponent"));
        ActionSystemComponent->StopActionSystem();
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
    // PlayerControllerの既存EventLogManagerを使用（新規作成しない）
    if (AC_PlayerController* PC = Cast<AC_PlayerController>(GetOwner()))
    {
        EventLogManager = PC->EventLogManager;
        UE_LOG(LogTemp, Log, TEXT("CombatComponent: Using PlayerController EventLogManager: %p"), (void*)EventLogManager);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CombatComponent: Owner is not AC_PlayerController, creating new EventLogManager"));
        EventLogManager = NewObject<UEventLogManager>(GetOwner());
        if (EventLogManager)
        {
            EventLogManager->RegisterComponent();
        }
    }

    // ActionSystemComponent作成
    ActionSystemComponent = NewObject<UActionSystemComponent>(GetOwner());
    if (ActionSystemComponent)
    {
        ActionSystemComponent->RegisterComponent();
        ActionSystemComponent->SetEventLogManager(EventLogManager);
        
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
    
    // 配列の安全性チェック
    if (Team.Num() == 0)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("GetAliveMembers: Empty team"));
        return AliveMembers;
    }
    
    for (int32 i = 0; i < Team.Num(); i++)
    {
        if (!Team.IsValidIndex(i))
        {
            UE_LOG(LogTemp, Error, TEXT("GetAliveMembers: Invalid index %d"), i);
            continue;
        }
        
        AC_IdleCharacter* Member = Team[i];
        if (IsValid(Member))
        {
            // StatusComponentからHP確認
            if (UCharacterStatusComponent* StatusComp = Member->GetStatusComponent())
            {
                float CurrentHP = StatusComp->GetCurrentHealth();
                if (CurrentHP > 0.0f)
                {
                    AliveMembers.Add(Member);
                    UE_LOG(LogTemp, VeryVerbose, TEXT("Character %s is alive with %f HP"), 
                        *Member->GetName(), CurrentHP);
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Character %s is dead (0 HP)"), *Member->GetName());
                }
            }
            else
            {
                // StatusComponentがない場合は生存とみなす（フォールバック）
                AliveMembers.Add(Member);
                UE_LOG(LogTemp, Warning, TEXT("Character %s has no StatusComponent, assuming alive"), 
                    *Member->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GetAliveMembers: Invalid character at index %d"), i);
        }
    }
    
    
    return AliveMembers;
}

// === TimeManager統合関数 ===

void UCombatComponent::ProcessCombat(float DeltaTime)
{
    if (CurrentState == ECombatState::Inactive)
    {
        return; // 戦闘中でなければ何もしない
    }
    
    switch (CurrentState)
    {
        case ECombatState::Preparing:
            // 準備状態 - タイマーで自動進行されるので何もしない
            break;
            
        case ECombatState::InProgress:
            // 行動ゲージシステム - TimeManagerの1ターンで1人行動
            if (ActionSystemComponent && ActionSystemComponent->IsSystemActive())
            {
                bool bCharacterActed = ActionSystemComponent->ProcessSingleTurnWithGauge();
                if (bCharacterActed)
                {
                    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessCombat: Character action processed with gauge system"));
                }
            }
            
            // 戦闘終了チェック
            CheckCombatCompletion();
            break;
            
        case ECombatState::Completed:
            // 終了処理中 - 何もしない（自動的に完了する）
            break;
            
        default:
            break;
    }
}

bool UCombatComponent::IsInCombat() const
{
    return CurrentState == ECombatState::Preparing || 
           CurrentState == ECombatState::InProgress || 
           CurrentState == ECombatState::Completed;
}

bool UCombatComponent::StartCombatSimple(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam)
{
    if (CurrentState != ECombatState::Inactive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Combat already active, cannot start new combat"));
        return false;
    }

    if (AllyTeam.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start combat with empty ally team"));
        return false;
    }

    if (EnemyTeam.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start combat with empty enemy team"));
        return false;
    }

    // チームメンバーを設定
    AllyTeamMembers = AllyTeam;
    EnemyTeamMembers = EnemyTeam;
    CombatLocationId = TEXT("unknown"); // 場所は後で設定可能
    
    // 戦闘開始処理
    SetCombatState(ECombatState::Preparing, TEXT("Combat starting"));
    CombatStartTime = GetWorld()->GetTimeSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("⚔️ COMBAT INITIALIZED: %d allies vs %d enemies"), 
        AllyTeam.Num(), EnemyTeam.Num());
    
    // 準備時間後に戦闘開始
    if (bAutoStartCombat)
    {
        GetWorld()->GetTimerManager().SetTimer(
            PreparationTimerHandle,
            this,
            &UCombatComponent::StartCombatAfterPreparation,
            CombatPreparationTime,
            false
        );
    }
    
    return true;
}