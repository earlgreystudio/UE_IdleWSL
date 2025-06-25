#include "ActionSystemComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"
#include "../Managers/CombatCalculator.h"
#include "EventLogManager.h"
#include "../C_PlayerController.h"
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
    
    // 戦闘終了チェック（実際のHP判定）
    if (AreAllEnemiesDead() || AreAllAlliesDead())
    {
        UE_LOG(LogTemp, Log, TEXT("Combat ended - One side has been defeated"));
        TriggerCombatEnd();
        StopActionSystem();
    }
    
    // フェイルセーフ：1000回のアクション後に強制終了
    static int ActionCount = 0;
    ActionCount++;
    if (ActionCount >= 1000)
    {
        UE_LOG(LogTemp, Warning, TEXT("Combat force-ended after %d actions (failsafe)"), ActionCount);
        ActionCount = 0;
        TriggerCombatEnd();
        StopActionSystem();
    }
}

void UActionSystemComponent::ProcessCharacterAction(FCharacterAction& Action)
{
    // 完全防御的チェック
    if (!Action.Character)
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessCharacterAction: Null character in action"));
        return;
    }
    
    if (!IsValid(Action.Character))
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessCharacterAction: Invalid character object"));
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
                if (Action.Character) NewEntry.CombatData.AttackerName = IIdleCharacterInterface::Execute_GetCharacterName(Action.Character);
                if (Target) NewEntry.CombatData.DefenderName = IIdleCharacterInterface::Execute_GetCharacterName(Target);
                NewEntry.CombatData.WeaponName = WeaponId.IsEmpty() ? TEXT("素手") : WeaponId;
                NewEntry.CombatData.Damage = Result.FinalDamage;
                NewEntry.CombatData.bIsCritical = Result.bCritical;
                
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
    if (UCharacterInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        FItemDataRow EquippedWeapon = InventoryComp->GetEquippedWeaponData();
        if (!EquippedWeapon.Name.IsEmpty())
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("SelectWeapon: Using equipped weapon: %s"), *EquippedWeapon.Name.ToString());
            return EquippedWeapon.Name.ToString();
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
    // 完全防御的チェック
    if (!Action.Character || !IsValid(Action.Character))
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateNextActionTime: Invalid character"));
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

void UActionSystemComponent::TriggerCombatEnd()
{
    UE_LOG(LogTemp, Error, TEXT("*** TriggerCombatEnd: Combat End Analysis ***"));
    
    // 生存しているキャラクターを取得
    TArray<AC_IdleCharacter*> AliveAllies = GetAliveAllies();
    TArray<AC_IdleCharacter*> AliveEnemies = GetAliveEnemies();
    
    UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: Alive Allies: %d, Alive Enemies: %d"), 
        AliveAllies.Num(), AliveEnemies.Num());
    
    // 勝者と敗者を決定（どちらかが全滅するまで戦う）
    TArray<AC_IdleCharacter*> Winners;
    TArray<AC_IdleCharacter*> Losers;
    
    if (AliveAllies.Num() > 0 && AliveEnemies.Num() == 0)
    {
        // 味方の勝利（敵が全滅）
        Winners = AliveAllies;
        Losers = GetAllEnemies();
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: Ally Victory - %d allies survived, enemies eliminated"), Winners.Num());
    }
    else if (AliveEnemies.Num() > 0 && AliveAllies.Num() == 0)
    {
        // 敵の勝利（味方が全滅）
        Winners = AliveEnemies;
        Losers = GetAllAllies();
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: Enemy Victory - %d enemies survived, allies eliminated"), Winners.Num());
    }
    else
    {
        // まだ戦闘継続中（これは本来呼ばれるべきではない）
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: Battle should continue - Allies: %d, Enemies: %d"), 
            AliveAllies.Num(), AliveEnemies.Num());
        Winners.Empty();
        Losers.Empty();
    }
    
    // 戦闘時間を計算（適当な値）
    float Duration = 10.0f;
    
    UE_LOG(LogTemp, Log, TEXT("Combat end: %d winners, %d losers, duration %.1f"), 
        Winners.Num(), Losers.Num(), Duration);
    
    // PlayerControllerのEventLogManagerを使用
    UE_LOG(LogTemp, Error, TEXT("*** TriggerCombatEnd: Attempting to log combat end ***"));
    if (UWorld* World = GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: World found"));
        if (AC_PlayerController* PC = Cast<AC_PlayerController>(World->GetFirstPlayerController()))
        {
            UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: PlayerController found"));
            if (PC->EventLogManager)
            {
                UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: EventLogManager found, calling LogCombatEnd"));
                PC->EventLogManager->LogCombatEnd(Winners, Losers, Duration);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: PlayerController has no EventLogManager"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: No PlayerController found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEnd: No World found"));
    }
}

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