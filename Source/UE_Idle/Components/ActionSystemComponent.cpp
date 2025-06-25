#include "ActionSystemComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"
#include "../Managers/CombatCalculator.h"
#include "EventLogManager.h"
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

void UActionSystemComponent::ProcessActions()
{
    if (!bSystemActive)
    {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // 味方の行動処理
    for (FCharacterAction& Action : AllyActions)
    {
        if (IsCharacterAlive(Action.Character) && CurrentTime >= Action.NextActionTime)
        {
            ProcessCharacterAction(Action);
        }
    }
    
    // 敵の行動処理
    for (FCharacterAction& Action : EnemyActions)
    {
        if (IsCharacterAlive(Action.Character) && CurrentTime >= Action.NextActionTime)
        {
            ProcessCharacterAction(Action);
        }
    }
    
    // 戦闘終了チェック
    if (AreAllEnemiesDead() || AreAllAlliesDead())
    {
        StopActionSystem();
    }
}

void UActionSystemComponent::ProcessCharacterAction(FCharacterAction& Action)
{
    if (!Action.Character || !IsCharacterAlive(Action.Character))
    {
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
    if (EventLogManager)
    {
        EventLogManager->AddCombatCalculationLog(Action.Character, Target, WeaponId, Result);
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
        return TEXT("");
    }

    // インベントリから武器を探す
    if (UCharacterInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        TArray<FInventorySlot> AllSlots = InventoryComp->GetAllInventorySlots();
        
        for (const FInventorySlot& Slot : AllSlots)
        {
            // アイテムタイプが武器かチェック（ItemDataTableManagerで確認が必要）
            if (Slot.ItemId.Contains(TEXT("sword")) || 
                Slot.ItemId.Contains(TEXT("axe")) || 
                Slot.ItemId.Contains(TEXT("weapon")))
            {
                return Slot.ItemId;
            }
        }
    }
    
    // 武器がない場合は素手（格闘）
    // 素手用のデフォルト値を返すか、装備された武器をチェック
    if (UCharacterInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        FItemDataRow EquippedWeapon = InventoryComp->GetEquippedWeaponData();
        if (!EquippedWeapon.Name.IsEmpty())
        {
            return EquippedWeapon.Name.ToString(); // 装備中の武器のNameを使用
        }
    }
    
    // 完全に武器がない場合は素手戦闘
    return TEXT(""); // 空文字列で格闘スキル使用
}

void UActionSystemComponent::UpdateNextActionTime(FCharacterAction& Action, const FString& WeaponId)
{
    // 攻撃速度計算
    float AttackSpeed = UCombatCalculator::CalculateAttackSpeed(Action.Character, WeaponId);
    Action.AttackSpeed = AttackSpeed;
    
    // 次回行動時間設定
    float ActionInterval = 1.0f / AttackSpeed;
    Action.NextActionTime = GetWorld()->GetTimeSeconds() + ActionInterval;
}

bool UActionSystemComponent::IsCharacterAlive(AC_IdleCharacter* Character) const
{
    if (!Character || !IsValid(Character))
    {
        return false;
    }

    // CharacterStatusComponentから実際のHPをチェック
    if (UCharacterStatusComponent* StatusComp = IIdleCharacterInterface::Execute_GetCharacterStatusComponent(Character))
    {
        return StatusComp->GetCurrentHealth() > 0.0f;
    }

    // StatusComponentがない場合はfalse
    return false;
}

void UActionSystemComponent::LogActionInfo(const FCharacterAction& Action, const FString& WeaponId) const
{
    if (!Action.Character || !Action.TargetCharacter)
    {
        return;
    }

    FString ActorName = IIdleCharacterInterface::Execute_GetCharacterName(Action.Character);
    FString TargetName = IIdleCharacterInterface::Execute_GetCharacterName(Action.TargetCharacter);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("%s attacks %s with %s (Speed: %.2f, Next: %.2fs)"), 
        *ActorName, *TargetName, *WeaponId, Action.AttackSpeed, Action.NextActionTime);
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