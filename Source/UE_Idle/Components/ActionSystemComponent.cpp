#include "ActionSystemComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Managers/CombatCalculator.h"
#include "EventLogManager.h"
#include "../C_PlayerController.h"
#include "CombatComponent.h"
#include "Engine/World.h"

UActionSystemComponent::UActionSystemComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bSystemActive = false;
    ActionCheckInterval = 0.0f;  // 即座実行
    bEnableAI = true;
    EventLogManager = nullptr;
}

void UActionSystemComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UActionSystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopActionSystem();
    Super::EndPlay(EndPlayReason);
}

void UActionSystemComponent::StartActionSystem()
{
    if (bSystemActive)
    {
        return;
    }

    bSystemActive = true;
    
    // フェイルセーフカウンターをリセット
    TotalActionCount = 0;
    
    // タイマー開始
    if (GetWorld())
    {
        if (ActionCheckInterval <= 0.0f)
        {
            // インターバルが0以下の場合は即座に実行
            UE_LOG(LogTemp, Log, TEXT("Action system started - immediate processing mode"));
            GetWorld()->GetTimerManager().SetTimer(
                ActionTimerHandle,
                this,
                &UActionSystemComponent::ProcessActions,
                0.01f,  // 最小間隔
                true
            );
        }
        else
        {
            // 指定されたインターバルで実行
            GetWorld()->GetTimerManager().SetTimer(
                ActionTimerHandle,
                this,
                &UActionSystemComponent::ProcessActions,
                ActionCheckInterval,
                true  // ループ
            );
            UE_LOG(LogTemp, Log, TEXT("Action system started with interval %.2fs"), ActionCheckInterval);
        }
    }
}

void UActionSystemComponent::StopActionSystem()
{
    if (!bSystemActive)
    {
        return;
    }

    bSystemActive = false;
    
    // タイマー停止
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ActionTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("Action system stopped"));
    }

    // 即座にすべてのアクションをクリア
    AllyActions.Empty();
    EnemyActions.Empty();
    UE_LOG(LogTemp, Log, TEXT("All actions cleared on system stop"));
}

void UActionSystemComponent::RegisterCharacter(AC_IdleCharacter* Character, const TArray<AC_IdleCharacter*>& Enemies)
{
    if (!Character)
    {
        return;
    }

    // 既に登録済みかチェック
    if (FindCharacterAction(Character))
    {
        return;
    }

    FCharacterAction NewAction;
    NewAction.Character = Character;
    NewAction.ActionType = EActionType::Attack;
    NewAction.TargetCharacter = nullptr;
    NewAction.NextActionTime = 0.0f;  // 即座に行動可能
    
    // 敵判定（簡易実装：Enemiesリストに含まれているキャラクターに対しては敵として扱われる）
    // つまり、このCharacterがEnemiesリストの誰かにとっての敵なら、このCharacterは味方
    // RegisterTeamから呼ばれる時：
    // - 味方登録時：Enemies=敵チーム、Character=味方 → bIsEnemyはfalse（味方）
    // - 敵登録時：Enemies=味方チーム、Character=敵 → bIsEnemyはtrue（敵）
    bool bIsEnemy = false;
    
    // 敵チーム登録時は、Enemiesに味方チームが入っているので、このロジックを反転する必要がある
    // より明確な実装：AllyActionsに既に登録されているかチェック
    for (const FCharacterAction& Action : AllyActions)
    {
        if (Action.Character == Character)
        {
            // 既に味方として登録済み
            return;
        }
    }
    
    // EnemyActionsに既に登録されているかチェック
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (Action.Character == Character)
        {
            // 既に敵として登録済み
            return;
        }
    }
    
    // RegisterTeamから呼ばれた際のコンテキストで判断
    // Enemiesリストが空でない場合、このメソッドはRegisterTeamから呼ばれている
    if (Enemies.Num() > 0)
    {
        // Characterが渡されたEnemiesリストに含まれていない = 味方
        bIsEnemy = Enemies.Contains(Character);
    }

    if (bIsEnemy)
    {
        EnemyActions.Add(NewAction);
        UE_LOG(LogTemp, Log, TEXT("Registered enemy character: %s"), 
            *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
    else
    {
        AllyActions.Add(NewAction);
        UE_LOG(LogTemp, Log, TEXT("Registered ally character: %s"), 
            *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
}

void UActionSystemComponent::UnregisterCharacter(AC_IdleCharacter* Character)
{
    if (RemoveCharacterAction(Character))
    {
        UE_LOG(LogTemp, Log, TEXT("Unregistered character: %s"), 
            Character ? *IIdleCharacterInterface::Execute_GetCharacterName(Character) : TEXT("nullptr"));
    }
}

void UActionSystemComponent::RegisterAlly(AC_IdleCharacter* Character)
{
    if (!Character || !IsValid(Character))
    {
        return;
    }

    // 重複チェック
    for (const FCharacterAction& Action : AllyActions)
    {
        if (Action.Character == Character)
        {
            return;
        }
    }

    FCharacterAction NewAction;
    NewAction.Character = Character;
    NewAction.ActionType = EActionType::Attack;
    NewAction.TargetCharacter = nullptr;
    NewAction.NextActionTime = 0.0f;
    
    AllyActions.Add(NewAction);
    UE_LOG(LogTemp, Log, TEXT("Registered ally character: %s"), 
        *IIdleCharacterInterface::Execute_GetCharacterName(Character));
}

void UActionSystemComponent::RegisterEnemy(AC_IdleCharacter* Character)
{
    if (!Character || !IsValid(Character))
    {
        return;
    }

    // 重複チェック
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (Action.Character == Character)
        {
            return;
        }
    }

    FCharacterAction NewAction;
    NewAction.Character = Character;
    NewAction.ActionType = EActionType::Attack;
    NewAction.TargetCharacter = nullptr;
    NewAction.NextActionTime = 0.0f;
    
    EnemyActions.Add(NewAction);
    UE_LOG(LogTemp, Log, TEXT("Registered enemy character: %s"), 
        *IIdleCharacterInterface::Execute_GetCharacterName(Character));
}

void UActionSystemComponent::RegisterTeam(const TArray<AC_IdleCharacter*>& AllyTeam, const TArray<AC_IdleCharacter*>& EnemyTeam)
{
    UE_LOG(LogTemp, Error, TEXT("*** ActionSystemComponent::RegisterTeam CALLED ***"));
    UE_LOG(LogTemp, Error, TEXT("AllyTeam: %d, EnemyTeam: %d"), AllyTeam.Num(), EnemyTeam.Num());
    
    // 味方チーム登録
    for (AC_IdleCharacter* Ally : AllyTeam)
    {
        if (Ally)
        {
            RegisterAlly(Ally);
        }
    }
    
    // 敵チーム登録
    for (AC_IdleCharacter* Enemy : EnemyTeam)
    {
        if (Enemy)
        {
            RegisterEnemy(Enemy);
        }
    }
    
    // 戦闘開始をログに記録
    if (UWorld* World = GetWorld())
    {
        if (AC_PlayerController* PC = Cast<AC_PlayerController>(World->GetFirstPlayerController()))
        {
            if (PC->EventLogManager)
            {
                UE_LOG(LogTemp, Error, TEXT("Calling LogCombatStart from ActionSystemComponent"));
                PC->EventLogManager->LogCombatStart(AllyTeam, EnemyTeam, TEXT("戦場"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("ActionSystemComponent: PlayerController has no EventLogManager"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ActionSystemComponent: No PlayerController found"));
        }
    }
    
    // === 行動ゲージシステム初期化 ===
    
    // 味方チームのゲージ初期化
    for (AC_IdleCharacter* Ally : AllyTeam)
    {
        if (Ally)
        {
            InitializeCharacterGauge(Ally);
        }
    }
    
    // 敵チームのゲージ初期化
    for (AC_IdleCharacter* Enemy : EnemyTeam)
    {
        if (Enemy)
        {
            InitializeCharacterGauge(Enemy);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("RegisterTeam: Initialized action gauges for %d allies and %d enemies"), 
        AllyTeam.Num(), EnemyTeam.Num());
}

void UActionSystemComponent::ClearAllCharacters()
{
    AllyActions.Empty();
    EnemyActions.Empty();
    UE_LOG(LogTemp, Log, TEXT("Cleared all registered characters"));
}

void UActionSystemComponent::HandleCharacterDeath(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return;
    }

    UnregisterCharacter(Character);
    OnCharacterDeath.Broadcast(Character);
    
    if (EventLogManager)
    {
        EventLogManager->AddCombatLog(ECombatLogType::Death, Character);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Character died: %s"), 
        *IIdleCharacterInterface::Execute_GetCharacterName(Character));
}

bool UActionSystemComponent::AreAllEnemiesDead() const
{
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (IsCharacterAlive(Action.Character))
        {
            return false;
        }
    }
    return true;
}

bool UActionSystemComponent::AreAllAlliesDead() const
{
    for (const FCharacterAction& Action : AllyActions)
    {
        if (IsCharacterAlive(Action.Character))
        {
            return false;
        }
    }
    return true;
}

TArray<AC_IdleCharacter*> UActionSystemComponent::GetAliveAllies() const
{
    TArray<AC_IdleCharacter*> AliveAllies;
    
    for (const FCharacterAction& Action : AllyActions)
    {
        if (IsCharacterAlive(Action.Character))
        {
            AliveAllies.Add(Action.Character);
        }
    }
    
    return AliveAllies;
}

TArray<AC_IdleCharacter*> UActionSystemComponent::GetAliveEnemies() const
{
    TArray<AC_IdleCharacter*> AliveEnemies;
    
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (IsCharacterAlive(Action.Character))
        {
            AliveEnemies.Add(Action.Character);
        }
    }
    
    return AliveEnemies;
}

TArray<AC_IdleCharacter*> UActionSystemComponent::GetAllAllies() const
{
    TArray<AC_IdleCharacter*> AllAllies;
    for (const FCharacterAction& Action : AllyActions)
    {
        if (Action.Character)
        {
            AllAllies.Add(Action.Character);
        }
    }
    return AllAllies;
}

TArray<AC_IdleCharacter*> UActionSystemComponent::GetAllEnemies() const
{
    TArray<AC_IdleCharacter*> AllEnemies;
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (Action.Character)
        {
            AllEnemies.Add(Action.Character);
        }
    }
    return AllEnemies;
}

void UActionSystemComponent::ProcessActions()
{
    if (!bSystemActive)
    {
        return;
    }

    // 毎回処理前に無効なキャラクター参照をクリーンアップ
    CleanupInvalidCharacters();
    
    // アクションが空の場合は処理を停止
    if (AllyActions.Num() == 0 && EnemyActions.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessActions: No valid actions remaining, stopping system"));
        StopActionSystem();
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // 味方の行動処理（インデックスベースで安全に）
    for (int32 i = 0; i < AllyActions.Num(); i++)
    {
        if (i >= AllyActions.Num()) break; // 配列サイズ変更対策
        
        FCharacterAction& Action = AllyActions[i];
        if (IsCharacterAlive(Action.Character) && CurrentTime >= Action.NextActionTime)
        {
            ProcessCharacterAction(Action);
        }
    }
    
    // 敵の行動処理（インデックスベースで安全に）
    for (int32 i = 0; i < EnemyActions.Num(); i++)
    {
        if (i >= EnemyActions.Num()) break; // 配列サイズ変更対策
        
        FCharacterAction& Action = EnemyActions[i];
        if (IsCharacterAlive(Action.Character) && CurrentTime >= Action.NextActionTime)
        {
            ProcessCharacterAction(Action);
        }
    }
    
    // 戦闘終了チェック（ログ記録継続のため復活、ただしActionSystem停止はしない）
    if (AreAllEnemiesDead() || AreAllAlliesDead())
    {
        UE_LOG(LogTemp, Log, TEXT("ProcessActions: Combat ending detected, but continuing for log recording"));
        // CombatComponentに戦闘終了を通知するが、ActionSystemは停止しない
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (UCombatComponent* CombatComp = PC->FindComponentByClass<UCombatComponent>())
                {
                    CombatComp->RequestCombatCompletion();
                }
            }
        }
    }
    
    // フェイルセーフチェックは ProcessCharacterAction で実行
}

void UActionSystemComponent::ProcessCharacterAction(FCharacterAction& Action)
{
    // システムが停止している場合は即座に終了
    if (!bSystemActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessCharacterAction: System not active, aborting"));
        return;
    }

    // フェイルセーフ：3000回の実際のキャラクターアクション後に強制終了
    TotalActionCount++;
    if (TotalActionCount >= 3000)
    {
        UE_LOG(LogTemp, Warning, TEXT("Combat force-ended after %d character actions (failsafe)"), TotalActionCount);
        StopActionSystem();
        // CombatComponentに戦闘終了を通知
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (UCombatComponent* CombatComp = PC->FindComponentByClass<UCombatComponent>())
                {
                    CombatComp->RequestCombatCompletion();
                }
            }
        }
        return;
    }
    
    // 完全防御的チェック
    if (!Action.Character || !IsValid(Action.Character))
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessCharacterAction: Invalid character in action"));
        return;
    }
    
    if (!IsCharacterAlive(Action.Character))
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessCharacterAction: Character %s is dead"), *Action.Character->GetName());
        return;
    }
    
    // StatusComponentの安全チェック
    UCharacterStatusComponent* StatusComp = nullptr;
    try 
    {
        StatusComp = Action.Character->GetStatusComponent();
    }
    catch(...)
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessCharacterAction: Exception getting StatusComponent for %s"), *Action.Character->GetName());
        return;
    }
    
    if (!StatusComp || !IsValid(StatusComp))
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessCharacterAction: Character %s has no valid StatusComponent"), *Action.Character->GetName());
        return;
    }

    // 対象選択
    TArray<AC_IdleCharacter*> Enemies;
    bool bIsAlly = AllyActions.ContainsByPredicate([&Action](const FCharacterAction& A) { 
        return A.Character == Action.Character; 
    });
    
    if (bIsAlly)
    {
        Enemies = GetAliveEnemies();
    }
    else
    {
        Enemies = GetAliveAllies();
    }
    
    if (Enemies.Num() == 0)
    {
        return;  // 攻撃対象がいない
    }

    AC_IdleCharacter* Target = SelectTarget(Action.Character, Enemies);
    if (!Target)
    {
        return;
    }

    Action.TargetCharacter = Target;
    
    // 武器選択
    FString WeaponId = SelectWeapon(Action.Character);
    
    // 戦闘計算実行
    FCombatCalculationResult Result = UCombatCalculator::PerformCombatCalculation(
        Action.Character, Target, WeaponId);
    
    // ログ記録
    if (UWorld* World = GetWorld())
    {
        if (AC_PlayerController* PC = Cast<AC_PlayerController>(World->GetFirstPlayerController()))
        {
            if (PC->EventLogManager)
            {
                UE_LOG(LogTemp, Log, TEXT("About to call AddCombatCalculationLog: %s vs %s, damage %d, hit=%s"), 
                    Action.Character ? *Action.Character->GetName() : TEXT("Unknown"),
                    Target ? *Target->GetName() : TEXT("Unknown"),
                    Result.FinalDamage,
                    Result.bHit ? TEXT("true") : TEXT("false"));
                
                // 戦闘前の HP を取得
                int32 AttackerHP = 100, AttackerMaxHP = 100, DefenderHP = 100, DefenderMaxHP = 100;
                
                if (Action.Character && IsValid(Action.Character))
                {
                    if (UCharacterStatusComponent* AttackerStatusComp = Action.Character->GetStatusComponent())
                    {
                        if (IsValid(AttackerStatusComp))
                        {
                            FCharacterStatus Status = AttackerStatusComp->GetStatus();
                            AttackerHP = FMath::RoundToInt(Status.CurrentHealth);
                            AttackerMaxHP = FMath::RoundToInt(Status.MaxHealth);
                            UE_LOG(LogTemp, Log, TEXT("ActionSystem: Attacker %s HP: %d/%d"), 
                                *IIdleCharacterInterface::Execute_GetCharacterName(Action.Character), AttackerHP, AttackerMaxHP);
                        }
                    }
                }
                
                if (Target && IsValid(Target))
                {
                    if (UCharacterStatusComponent* DefenderStatusComp = Target->GetStatusComponent())
                    {
                        if (IsValid(DefenderStatusComp))
                        {
                            FCharacterStatus Status = DefenderStatusComp->GetStatus();
                            DefenderHP = FMath::RoundToInt(Status.CurrentHealth);
                            DefenderMaxHP = FMath::RoundToInt(Status.MaxHealth);
                            UE_LOG(LogTemp, Log, TEXT("ActionSystem: Defender %s HP: %d/%d"), 
                                *IIdleCharacterInterface::Execute_GetCharacterName(Target), DefenderHP, DefenderMaxHP);
                        }
                    }
                }
                
                // 簡易版：直接戦闘ログを追加
                FEventLogEntry NewEntry;
                NewEntry.EventCategory = EEventCategory::Combat;
                NewEntry.EventType = Result.bHit ? (Result.bCritical ? EEventLogType::Critical : EEventLogType::Hit) : EEventLogType::Miss;
                NewEntry.Priority = EEventPriority::Normal;
                
                // CombatData設定
                if (Action.Character) 
                {
                    NewEntry.CombatData.AttackerName = IIdleCharacterInterface::Execute_GetCharacterName(Action.Character);
                    UE_LOG(LogTemp, Warning, TEXT("Combat Log: AttackerName = '%s'"), *NewEntry.CombatData.AttackerName);
                }
                if (Target) 
                {
                    NewEntry.CombatData.DefenderName = IIdleCharacterInterface::Execute_GetCharacterName(Target);
                    UE_LOG(LogTemp, Warning, TEXT("Combat Log: DefenderName = '%s'"), *NewEntry.CombatData.DefenderName);
                }
                NewEntry.CombatData.WeaponName = WeaponId.IsEmpty() ? TEXT("素手") : WeaponId;
                NewEntry.CombatData.Damage = Result.FinalDamage;
                NewEntry.CombatData.bIsCritical = Result.bCritical;
                
                UE_LOG(LogTemp, Warning, TEXT("Combat Log: Damage = %d, Critical = %s"), 
                    Result.FinalDamage, Result.bCritical ? TEXT("true") : TEXT("false"));
                
                // 攻撃者が味方か敵かを判定
                bool bIsAttackerAlly = AllyActions.ContainsByPredicate([&Action](const FCharacterAction& A) { 
                    return A.Character == Action.Character; 
                });
                NewEntry.CombatData.bIsPlayerAttacker = bIsAttackerAlly;
                
                // HP設定
                NewEntry.CombatData.AttackerHP = AttackerHP;
                NewEntry.CombatData.AttackerMaxHP = AttackerMaxHP;
                NewEntry.CombatData.DefenderHP = DefenderHP;
                NewEntry.CombatData.DefenderMaxHP = DefenderMaxHP;
                
                PC->EventLogManager->AddEventLog(NewEntry);
                
                UE_LOG(LogTemp, Log, TEXT("Combat log recorded successfully"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("PlayerController has no EventLogManager for combat log"));
            }
        }
    }
    
    // ダメージ適用
    if (Result.FinalDamage > 0)
    {
        if (UCharacterStatusComponent* TargetStatus = IIdleCharacterInterface::Execute_GetCharacterStatusComponent(Target))
        {
            float CurrentHP = TargetStatus->GetCurrentHealth();
            float NewHP = FMath::Max(0.0f, CurrentHP - Result.FinalDamage);
            TargetStatus->SetCurrentHealth(NewHP);
            
            UE_LOG(LogTemp, Log, TEXT("%s Health: %.1f -> %.1f (-%d damage)"), 
                *IIdleCharacterInterface::Execute_GetCharacterName(Target),
                CurrentHP, NewHP, Result.FinalDamage);
            
            // 死亡判定
            if (NewHP <= 0.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("%s has died!"), 
                    *IIdleCharacterInterface::Execute_GetCharacterName(Target));
                OnCharacterDeath.Broadcast(Target);
                
                // CombatComponentに戦闘終了チェックを依頼
                if (UWorld* World = GetWorld())
                {
                    if (APlayerController* PC = World->GetFirstPlayerController())
                    {
                        if (UCombatComponent* CombatComp = PC->FindComponentByClass<UCombatComponent>())
                        {
                            CombatComp->RequestCombatCompletion();
                        }
                    }
                }
            }
        }
    }
    
    // イベント発火
    OnCharacterAction.Broadcast(Action.Character, Target, Result);
    
    // 次回行動時間更新
    UpdateNextActionTime(Action, WeaponId);
    
    // デバッグログ
    LogActionInfo(Action, WeaponId);
}

AC_IdleCharacter* UActionSystemComponent::SelectTarget(AC_IdleCharacter* Actor, const TArray<AC_IdleCharacter*>& Enemies)
{
    if (!bEnableAI || Enemies.Num() == 0)
    {
        return nullptr;
    }

    // 簡易AI：最初の生存敵を選択
    for (AC_IdleCharacter* Enemy : Enemies)
    {
        if (IsCharacterAlive(Enemy))
        {
            return Enemy;
        }
    }
    
    return nullptr;
}

FString UActionSystemComponent::SelectWeapon(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return TEXT("素手");
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("SelectWeapon for character: %s"), *Character->GetName());

    // 1. まず装備された武器をチェック
    if (UInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        // 装備武器のチェック
        if (InventoryComp->HasEquippedWeapon())
        {
            FString WeaponId = InventoryComp->GetEquippedWeaponId();
            UE_LOG(LogTemp, VeryVerbose, TEXT("SelectWeapon: Using equipped weapon: %s"), *WeaponId);
            return WeaponId;
        }
    }

    // 2. キャラクターの自然武器（NaturalWeaponName）をチェック
    if (Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
    {
        // キャラクターの種族/プリセット情報から自然武器を取得
        // 注：この部分は実装に依存します。現在はCharacterPresets.csvの情報を取得する仕組みが必要
        
        // 暫定的に、キャラクター名に基づいて自然武器を判定
        FString CharacterName = IIdleCharacterInterface::Execute_GetCharacterName(Character);
        
        if (CharacterName.Contains(TEXT("ネズミ")))
        {
            UE_LOG(LogTemp, Log, TEXT("SelectWeapon: Using natural weapon for %s: ネズミの歯"), *CharacterName);
            return TEXT("ネズミの歯");
        }
        else if (CharacterName.Contains(TEXT("ゴブリン")))
        {
            UE_LOG(LogTemp, Log, TEXT("SelectWeapon: Using natural weapon for %s: ゴブリンの爪"), *CharacterName);
            return TEXT("ゴブリンの爪");
        }
        else if (CharacterName.Contains(TEXT("ガエル")))
        {
            UE_LOG(LogTemp, Log, TEXT("SelectWeapon: Using natural weapon for %s: ガエルの毒舌"), *CharacterName);
            return TEXT("ガエルの毒舌");
        }
    }

    // 3. 装備武器も自然武器もない場合は素手戦闘
    UE_LOG(LogTemp, VeryVerbose, TEXT("SelectWeapon: No equipped or natural weapon found, using unarmed combat"));
    return TEXT("素手");
}

void UActionSystemComponent::UpdateNextActionTime(FCharacterAction& Action, const FString& WeaponId)
{
    // システムが停止している場合は即座に終了
    if (!bSystemActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateNextActionTime: System is not active, skipping"));
        return;
    }

    // 完全防御的チェック
    if (!Action.Character || !IsValid(Action.Character))
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateNextActionTime: Invalid character"));
        Action.NextActionTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.0f : 1.0f;
        Action.AttackSpeed = 1.0f;
        return;
    }
    
    // キャラクターが非アクティブな場合もスキップ
    bool bCharacterActive = false;
    if (Action.Character && Action.Character->GetClass() && 
        Action.Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
    {
        bCharacterActive = IIdleCharacterInterface::Execute_IsActive(Action.Character);
    }
    
    if (!bCharacterActive)
    {
        FString CharName = TEXT("Unknown");
        if (Action.Character && Action.Character->GetClass() && 
            Action.Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
        {
            CharName = IIdleCharacterInterface::Execute_GetCharacterName(Action.Character);
        }
        UE_LOG(LogTemp, Warning, TEXT("UpdateNextActionTime: Character %s is not active"), *CharName);
        Action.NextActionTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.0f : 1.0f;
        Action.AttackSpeed = 1.0f;
        return;
    }
    
    // StatusComponentの有効性をチェック
    UCharacterStatusComponent* StatusComp = Action.Character->GetStatusComponent();
    if (!StatusComp)
    {
        FString CharName = TEXT("Unknown");
        if (Action.Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
        {
            CharName = IIdleCharacterInterface::Execute_GetCharacterName(Action.Character);
        }
        UE_LOG(LogTemp, Error, TEXT("UpdateNextActionTime: Character %s has no valid StatusComponent"), *CharName);
        Action.NextActionTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.0f : 1.0f;
        Action.AttackSpeed = 1.0f;
        return;
    }
    
    // 攻撃速度計算（安全）
    float AttackSpeed = 1.0f;
    try 
    {
        AttackSpeed = UCombatCalculator::CalculateAttackSpeed(Action.Character, WeaponId);
        if (AttackSpeed <= 0.0f || !FMath::IsFinite(AttackSpeed))
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateNextActionTime: Invalid attack speed %.2f, using default"), AttackSpeed);
            AttackSpeed = 1.0f;
        }
    }
    catch(...)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateNextActionTime: Exception calculating attack speed for %s"), *Action.Character->GetName());
        AttackSpeed = 1.0f;
    }
    
    Action.AttackSpeed = AttackSpeed;
    
    // 次回行動時間設定（安全）
    float ActionInterval = 1.0f / AttackSpeed;
    if (GetWorld())
    {
        Action.NextActionTime = GetWorld()->GetTimeSeconds() + ActionInterval;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateNextActionTime: No valid world"));
        Action.NextActionTime = ActionInterval;
    }
}

bool UActionSystemComponent::IsCharacterAlive(AC_IdleCharacter* Character) const
{
    if (!Character || !IsValid(Character))
    {
        return false;
    }

    // StatusComponentから実際のHP状態を確認
    if (UCharacterStatusComponent* StatusComp = Character->GetStatusComponent())
    {
        if (IsValid(StatusComp))
        {
            float CurrentHP = StatusComp->GetCurrentHealth();
            bool bIsAlive = CurrentHP > 0.0f;
            UE_LOG(LogTemp, VeryVerbose, TEXT("IsCharacterAlive: %s HP=%.1f, alive=%s"), 
                *Character->GetName(), CurrentHP, bIsAlive ? TEXT("true") : TEXT("false"));
            return bIsAlive;
        }
    }

    // StatusComponentがない場合は生存扱い（フォールバック）
    UE_LOG(LogTemp, Warning, TEXT("IsCharacterAlive: %s has no StatusComponent, assuming alive"), *Character->GetName());
    return true;
}

// TriggerCombatEnd関数は削除 - 戦闘終了はCombatComponentで一元管理

void UActionSystemComponent::LogActionInfo(const FCharacterAction& Action, const FString& WeaponId) const
{
    // 完全に無効化してクラッシュを防ぐ
    return;
}

FCharacterAction* UActionSystemComponent::FindCharacterAction(AC_IdleCharacter* Character)
{
    // 味方チェック
    for (FCharacterAction& Action : AllyActions)
    {
        if (Action.Character == Character)
        {
            return &Action;
        }
    }
    
    // 敵チェック
    for (FCharacterAction& Action : EnemyActions)
    {
        if (Action.Character == Character)
        {
            return &Action;
        }
    }
    
    return nullptr;
}

bool UActionSystemComponent::RemoveCharacterAction(AC_IdleCharacter* Character)
{
    // 味方から削除試行
    int32 RemovedFromAllies = AllyActions.RemoveAll([Character](const FCharacterAction& Action) {
        return Action.Character == Character;
    });
    
    // 敵から削除試行
    int32 RemovedFromEnemies = EnemyActions.RemoveAll([Character](const FCharacterAction& Action) {
        return Action.Character == Character;
    });
    
    return (RemovedFromAllies + RemovedFromEnemies) > 0;
}

void UActionSystemComponent::ClearAllActions()
{
    UE_LOG(LogTemp, Warning, TEXT("ClearAllActions: Clearing %d ally actions and %d enemy actions"), 
           AllyActions.Num(), EnemyActions.Num());
    
    AllyActions.Empty();
    EnemyActions.Empty();
    TotalActionCount = 0;
    
    // タイマーも停止
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ActionTimerHandle);
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClearAllActions: All actions cleared and timer stopped"));
}

void UActionSystemComponent::CleanupInvalidCharacters()
{
    int32 RemovedAllies = 0;
    int32 RemovedEnemies = 0;
    
    // 無効な味方キャラクターを削除
    RemovedAllies = AllyActions.RemoveAll([](const FCharacterAction& Action) {
        if (!Action.Character || !IsValid(Action.Character))
        {
            UE_LOG(LogTemp, Warning, TEXT("CleanupInvalidCharacters: Removing null/invalid ally character"));
            return true;
        }
        
        // インターフェース関数の正しい呼び出し方法
        bool bCharacterActive = false;
        if (Action.Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
        {
            bCharacterActive = IIdleCharacterInterface::Execute_IsActive(Action.Character);
        }
        
        if (!bCharacterActive)
        {
            UE_LOG(LogTemp, Warning, TEXT("CleanupInvalidCharacters: Removing inactive ally character"));
            return true;
        }
        
        return false;
    });
    
    // 無効な敵キャラクターを削除
    RemovedEnemies = EnemyActions.RemoveAll([](const FCharacterAction& Action) {
        if (!Action.Character || !IsValid(Action.Character))
        {
            UE_LOG(LogTemp, Warning, TEXT("CleanupInvalidCharacters: Removing null/invalid enemy character"));
            return true;
        }
        
        // インターフェース関数の正しい呼び出し方法
        bool bCharacterActive = false;
        if (Action.Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass()))
        {
            bCharacterActive = IIdleCharacterInterface::Execute_IsActive(Action.Character);
        }
        
        if (!bCharacterActive)
        {
            UE_LOG(LogTemp, Warning, TEXT("CleanupInvalidCharacters: Removing inactive enemy character"));
            return true;
        }
        
        return false;
    });
    
    if (RemovedAllies > 0 || RemovedEnemies > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("CleanupInvalidCharacters: Removed %d allies and %d enemies"), 
               RemovedAllies, RemovedEnemies);
    }
}

// === 新しい1ターン1行動システム ===

AC_IdleCharacter* UActionSystemComponent::GetNextActingCharacter() const
{
    if (!bSystemActive)
    {
        return nullptr;
    }
    
    // const関数なので無効キャラクターのクリーンアップはしない
    
    AC_IdleCharacter* NextCharacter = nullptr;
    float EarliestTime = FLT_MAX;
    
    // 味方チームから最も早い行動時間のキャラクターを検索
    for (const FCharacterAction& Action : AllyActions)
    {
        if (IsCharacterAlive(Action.Character) && Action.NextActionTime < EarliestTime)
        {
            EarliestTime = Action.NextActionTime;
            NextCharacter = Action.Character;
        }
    }
    
    // 敵チームから最も早い行動時間のキャラクターを検索
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (IsCharacterAlive(Action.Character) && Action.NextActionTime < EarliestTime)
        {
            EarliestTime = Action.NextActionTime;
            NextCharacter = Action.Character;
        }
    }
    
    return NextCharacter;
}

bool UActionSystemComponent::ProcessSingleTurn()
{
    if (!bSystemActive)
    {
        return false;
    }
    
    // 無効なキャラクター参照をクリーンアップ
    CleanupInvalidCharacters();
    
    // 次に行動するキャラクターを取得
    AC_IdleCharacter* NextCharacter = GetNextActingCharacter();
    if (!NextCharacter)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessSingleTurn: No character ready to act"));
        return false;
    }
    
    // そのキャラクターの行動データを取得
    FCharacterAction* CharacterAction = FindCharacterAction(NextCharacter);
    if (!CharacterAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessSingleTurn: Character action not found"));
        return false;
    }
    
    // 現在時刻をチェック（デバッグ情報として使用）
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessSingleTurn: %s acting (NextActionTime: %f, CurrentTime: %f)"), 
        *NextCharacter->GetName(), CharacterAction->NextActionTime, CurrentTime);
    
    // そのキャラクターのみ行動
    ProcessCharacterAction(*CharacterAction);
    
    return true;
}

// === 行動ゲージシステム実装 ===

void UActionSystemComponent::UpdateAllGauges()
{
    // 味方チームのゲージ更新
    for (FCharacterAction& Action : AllyActions)
    {
        if (IsCharacterAlive(Action.Character))
        {
            Action.UpdateGauge();
        }
    }
    
    // 敵チームのゲージ更新
    for (FCharacterAction& Action : EnemyActions)
    {
        if (IsCharacterAlive(Action.Character))
        {
            Action.UpdateGauge();
        }
    }
}

AC_IdleCharacter* UActionSystemComponent::GetNextActingCharacterWithGauge() const
{
    if (!bSystemActive)
    {
        return nullptr;
    }
    
    AC_IdleCharacter* NextCharacter = nullptr;
    float HighestScore = 0.0f;
    
    // 味方チームから最高スコアキャラクター検索
    for (const FCharacterAction& Action : AllyActions)
    {
        if (IsCharacterAlive(Action.Character) && Action.CanAct())
        {
            float Score = Action.GetActionScore();
            if (Score > HighestScore)
            {
                HighestScore = Score;
                NextCharacter = Action.Character;
            }
        }
    }
    
    // 敵チームから最高スコアキャラクター検索
    for (const FCharacterAction& Action : EnemyActions)
    {
        if (IsCharacterAlive(Action.Character) && Action.CanAct())
        {
            float Score = Action.GetActionScore();
            if (Score > HighestScore)
            {
                HighestScore = Score;
                NextCharacter = Action.Character;
            }
        }
    }
    
    return NextCharacter;
}

void UActionSystemComponent::InitializeCharacterGauge(AC_IdleCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    
    // キャラクターのアクションデータを検索
    FCharacterAction* CharacterAction = FindCharacterAction(Character);
    if (!CharacterAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeCharacterGauge: Character action not found for %s"), 
            *Character->GetName());
        return;
    }
    
    // AGI値からゲージ速度を計算
    if (UCharacterStatusComponent* StatusComp = Character->GetStatusComponent())
    {
        float AGI = StatusComp->GetAgility();
        CharacterAction->GaugeSpeed = FMath::Max(1.0f, AGI * 0.5f); // AGI値の半分、最低1.0
        CharacterAction->ActionPriority = FMath::RoundToInt(AGI);     // AGI値を優先度に
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("InitializeCharacterGauge: %s - AGI:%.1f, GaugeSpeed:%.1f, Priority:%d"), 
            *Character->GetName(), AGI, CharacterAction->GaugeSpeed, CharacterAction->ActionPriority);
    }
    else
    {
        // StatusComponentがない場合のデフォルト値
        CharacterAction->GaugeSpeed = 10.0f;
        CharacterAction->ActionPriority = 10;
        
        UE_LOG(LogTemp, Warning, TEXT("InitializeCharacterGauge: %s has no StatusComponent, using defaults"), 
            *Character->GetName());
    }
    
    // ゲージを初期化
    CharacterAction->ActionGauge = 0.0f;
    CharacterAction->SpeedMultiplier = 1.0f;
}

bool UActionSystemComponent::ProcessSingleTurnWithGauge()
{
    if (!bSystemActive)
    {
        return false;
    }
    
    // 無効なキャラクター参照をクリーンアップ
    CleanupInvalidCharacters();
    
    // 全キャラクターのゲージを更新
    UpdateAllGauges();
    
    // 次に行動するキャラクターを取得
    AC_IdleCharacter* NextCharacter = GetNextActingCharacterWithGauge();
    if (!NextCharacter)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessSingleTurnWithGauge: No character ready to act"));
        return false;
    }
    
    // そのキャラクターの行動データを取得
    FCharacterAction* CharacterAction = FindCharacterAction(NextCharacter);
    if (!CharacterAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessSingleTurnWithGauge: Character action not found"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ProcessSingleTurnWithGauge: %s acting (Gauge: %.1f, Score: %.2f)"), 
        *NextCharacter->GetName(), CharacterAction->ActionGauge, CharacterAction->GetActionScore());
    
    // そのキャラクターのみ行動
    ProcessCharacterAction(*CharacterAction);
    
    // 行動後ゲージリセット
    CharacterAction->ResetAfterAction();
    
    return true;
}