#include "CharacterBrain.h"
#include "TaskManagerComponent.h"
#include "TeamComponent.h"
#include "LocationMovementComponent.h"
#include "InventoryComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Types/ItemTypes.h"
#include "../Managers/LocationDataTableManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UCharacterBrain::UCharacterBrain()
{
    // デフォルト性格設定
    MyPersonality = ECharacterPersonality::Loyal;
    
    // 参照フラグ初期化
    bReferencesInitialized = false;
    
    // 参照初期化
    TaskManagerRef = nullptr;
    TeamComponentRef = nullptr;
    MovementComponentRef = nullptr;
    CharacterRef = nullptr;
    
    // 性格別優先度の初期化
    InitializePersonalityPreferences();
}

void UCharacterBrain::InitializeReferences(UTaskManagerComponent* TaskManager, UTeamComponent* TeamComp, ULocationMovementComponent* MovementComp)
{
    TaskManagerRef = TaskManager;
    TeamComponentRef = TeamComp;
    MovementComponentRef = MovementComp;
    
    bReferencesInitialized = AreReferencesValid();
    
    if (bReferencesInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("🧠 CharacterBrain references initialized successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ CharacterBrain reference initialization failed"));
    }
}

void UCharacterBrain::SetCharacterReference(AC_IdleCharacter* Character)
{
    CharacterRef = Character;
    UE_LOG(LogTemp, Log, TEXT("🧠 CharacterBrain character reference set"));
}

bool UCharacterBrain::AreReferencesValid() const
{
    return IsValid(TaskManagerRef) && IsValid(TeamComponentRef) && IsValid(MovementComponentRef);
}

FCharacterAction UCharacterBrain::DecideOptimalAction(const FCharacterSituation& Situation)
{
    // 参照チェック
    if (!bReferencesInitialized || !AreReferencesValid())
    {
        UE_LOG(LogTemp, Error, TEXT("🧠❌ CharacterBrain: References not initialized properly"));
        return DecideWaitAction(Situation, TEXT("References not initialized"));
    }

    FCharacterAction DecidedAction;
    
    // チーム割り当てタスクに基づく判断（既存ロジックの移植）
    switch (Situation.TeamAssignedTask)
    {
        case ETaskType::Gathering:
            DecidedAction = DecideGatheringAction(Situation);
            break;
            
        case ETaskType::Adventure:
            DecidedAction = DecideAdventureAction(Situation);
            break;
            
        case ETaskType::All:
            // 全てモード: TaskManagerから次の利用可能タスクを取得
            {
                FString TargetItem = GetTargetItemForTeam(Situation.MyTeamIndex, Situation.CurrentLocation);
                if (!TargetItem.IsEmpty())
                {
                    // 採集タスクとして処理
                    FCharacterSituation ModifiedSituation = Situation;
                    ModifiedSituation.TeamAssignedTask = ETaskType::Gathering;
                    DecidedAction = DecideGatheringAction(ModifiedSituation);
                }
                else
                {
                    DecidedAction = DecideWaitAction(Situation, TEXT("No available tasks in All mode"));
                }
            }
            break;
            
        case ETaskType::Idle:
        default:
            DecidedAction = DecideWaitAction(Situation, TEXT("Team task is Idle"));
            break;
    }
    
    // 性格に基づく修正を適用
    DecidedAction = ApplyPersonalityModifiers(DecidedAction, Situation);
    
    // 判断プロセスをログ出力
    LogDecisionProcess(Situation, DecidedAction);
    
    // 決定されたアクションを明確にログ出力
    FString CharName = CharacterRef ? CharacterRef->GetName() : TEXT("Unknown");
    UE_LOG(LogTemp, Warning, TEXT("🧠✅ %s: DECIDED ACTION = %d (%s) at %s targeting %s"), 
        *CharName, 
        (int32)DecidedAction.ActionType, 
        *DecidedAction.ActionReason,
        *DecidedAction.TargetLocation,
        *DecidedAction.TargetItem);
    
    return DecidedAction;
}

FCharacterAction UCharacterBrain::DecideGatheringAction(const FCharacterSituation& Situation)
{
    FString CharName = CharacterRef ? CharacterRef->GetName() : TEXT("Unknown");
    
    // 拠点にいる場合の完全処理
    if (Situation.CurrentLocation == TEXT("base"))
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠🏠 %s: At base, checking for unload"), *CharName);
        
        // インベントリにアイテムがある場合は荷下ろし
        if (HasItemsToUnload(Situation))
        {
            UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: Has items to unload, executing unload action"), *CharName);
            FCharacterAction UnloadAction;
            UnloadAction.ActionType = ECharacterActionType::UnloadItems;
            UnloadAction.TargetLocation = TEXT("base");
            UnloadAction.ExpectedDuration = 0.5f;
            UnloadAction.ActionReason = TEXT("Unloading items at base");
            return UnloadAction;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: No items to unload at base"), *CharName);
            
            // 拠点で実行可能なタスクがあるかチェック
            FString TargetItemAtBase = GetTargetItemForTeam(Situation.MyTeamIndex, TEXT("base"));
            if (!TargetItemAtBase.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("🧠🎯 %s: Found new task %s at base"), *CharName, *TargetItemAtBase);
                // 拠点でのタスクがある場合は、以降の採集ロジックで処理
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🧠🔍 %s: No tasks at base, checking other locations"), *CharName);
                
                // 他の場所でタスクがあるかチェック
                TArray<FString> LocationsToCheck = {TEXT("plains"), TEXT("forest"), TEXT("mountain"), TEXT("swamp")};
                for (const FString& LocationToCheck : LocationsToCheck)
                {
                    FString TargetItemAtLocation = GetTargetItemForTeam(Situation.MyTeamIndex, LocationToCheck);
                    if (!TargetItemAtLocation.IsEmpty())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("🧠🚶 %s: Found task %s at %s, moving there"), 
                            *CharName, *TargetItemAtLocation, *LocationToCheck);
                        
                        // 移動アクションを返す
                        FCharacterAction MoveAction;
                        MoveAction.ActionType = ECharacterActionType::MoveToLocation;
                        MoveAction.TargetLocation = LocationToCheck;
                        MoveAction.TargetItem = TargetItemAtLocation;
                        MoveAction.ExpectedDuration = 3.0f; // 移動時間
                        MoveAction.ActionReason = FString::Printf(TEXT("Moving to %s to gather %s"), 
                            *LocationToCheck, *TargetItemAtLocation);
                        return MoveAction;
                    }
                }
                
                UE_LOG(LogTemp, Warning, TEXT("🧠😴 %s: No tasks available anywhere, waiting at base"), *CharName);
                return DecideWaitAction(Situation, TEXT("No tasks available anywhere"));
            }
        }
    }
    
    // 拠点にいない場合のみ帰還判定を実行
    if (Situation.CurrentLocation != TEXT("base"))
    {
        // 拠点帰還判定（既存ロジックの移植）
        if (ShouldReturnToBase(Situation))
        {
            UE_LOG(LogTemp, Warning, TEXT("🧠🏠 %s: Should return to base from %s"), *CharName, *Situation.CurrentLocation);
            FCharacterAction ReturnAction;
            ReturnAction.ActionType = ECharacterActionType::ReturnToBase;
            ReturnAction.TargetLocation = TEXT("base");
            ReturnAction.ExpectedDuration = 2.0f; // 移動時間
            ReturnAction.ActionReason = TEXT("Should return to base for unloading");
            return ReturnAction;
        }
    }
    
    // 現在地でのターゲットアイテムチェック（直接アプローチ）
    FString TargetItem = GetTargetItemForTeam(Situation.MyTeamIndex, Situation.CurrentLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("🧠🎯 %s: At %s, found target item: %s"), 
        *CharName, *Situation.CurrentLocation, *TargetItem);
    
    if (!TargetItem.IsEmpty())
    {
        // 現在地でタスクがある場合は即座に採集実行
        UE_LOG(LogTemp, Warning, TEXT("🧠🌱 %s: Executing gathering for %s at current location %s"), 
            *CharName, *TargetItem, *Situation.CurrentLocation);
        
        FCharacterAction GatherAction;
        GatherAction.ActionType = ECharacterActionType::GatherResources;
        GatherAction.TargetLocation = Situation.CurrentLocation;
        GatherAction.TargetItem = TargetItem;
        GatherAction.ExpectedDuration = 1.0f;
        GatherAction.ActionReason = FString::Printf(TEXT("Gathering %s at %s"), *TargetItem, *Situation.CurrentLocation);
        return GatherAction;
    }
    
    // 現在地にタスクがない場合は、チームの採集場所を使用
    FString TeamGatheringLocation = GetTeamGatheringLocation(Situation.MyTeamIndex);
    if (TeamGatheringLocation.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ No gathering location set for team %d"), Situation.MyTeamIndex);
        return DecideWaitAction(Situation, TEXT("No gathering location set for team"));
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🌱 Using team gathering location: %s"), *TeamGatheringLocation);
    TargetItem = GetTargetItemForTeam(Situation.MyTeamIndex, TeamGatheringLocation);
    
    if (TargetItem.IsEmpty())
    {
        return DecideWaitAction(Situation, TEXT("No target item for gathering"));
    }
    
    // 適切な採集場所の検索
    FString GatheringLocation = FindGatheringLocation(TargetItem);
    
    if (GatheringLocation.IsEmpty())
    {
        return DecideWaitAction(Situation, FString::Printf(TEXT("No gathering location found for item: %s"), *TargetItem));
    }
    
    // 現在地と目標地の比較
    if (Situation.CurrentLocation != GatheringLocation)
    {
        // 移動が必要
        FCharacterAction MoveAction;
        MoveAction.ActionType = ECharacterActionType::MoveToLocation;
        MoveAction.TargetLocation = GatheringLocation;
        MoveAction.TargetItem = TargetItem;
        MoveAction.ExpectedDuration = 2.0f; // 移動時間（既存システムと同じ）
        MoveAction.ActionReason = FString::Printf(TEXT("Moving to %s to gather %s"), *GatheringLocation, *TargetItem);
        return MoveAction;
    }
    else
    {
        // 既に正しい場所にいるので採集実行
        FCharacterAction GatherAction;
        GatherAction.ActionType = ECharacterActionType::GatherResources;
        GatherAction.TargetLocation = GatheringLocation;
        GatherAction.TargetItem = TargetItem;
        GatherAction.ExpectedDuration = 1.0f; // 採集時間
        GatherAction.ActionReason = FString::Printf(TEXT("Gathering %s at %s"), *TargetItem, *GatheringLocation);
        return GatherAction;
    }
}

FCharacterAction UCharacterBrain::DecideMovementAction(const FCharacterSituation& Situation)
{
    // 基本的な移動判断（将来の拡張用）
    FCharacterAction MoveAction;
    MoveAction.ActionType = ECharacterActionType::MoveToLocation;
    MoveAction.TargetLocation = TEXT("base");
    MoveAction.ExpectedDuration = 2.0f;
    MoveAction.ActionReason = TEXT("Basic movement decision");
    return MoveAction;
}

FCharacterAction UCharacterBrain::DecideAdventureAction(const FCharacterSituation& Situation)
{
    // 冒険タスクの判断（既存ロジックの移植）
    if (!TeamComponentRef)
    {
        return DecideWaitAction(Situation, TEXT("TeamComponent reference not available for adventure"));
    }
    
    // チームの冒険先を取得
    FTeam Team = TeamComponentRef->GetTeam(Situation.MyTeamIndex);
    FString AdventureLocation = Team.AdventureLocationId;
    
    if (AdventureLocation.IsEmpty())
    {
        return DecideWaitAction(Situation, TEXT("No adventure location assigned"));
    }
    
    // 現在地チェック
    if (Situation.CurrentLocation != AdventureLocation)
    {
        // 冒険地への移動
        FCharacterAction MoveAction;
        MoveAction.ActionType = ECharacterActionType::MoveToLocation;
        MoveAction.TargetLocation = AdventureLocation;
        MoveAction.ExpectedDuration = 2.0f;
        MoveAction.ActionReason = FString::Printf(TEXT("Moving to adventure location: %s"), *AdventureLocation);
        return MoveAction;
    }
    else
    {
        // 戦闘開始
        FCharacterAction CombatAction;
        CombatAction.ActionType = ECharacterActionType::AttackEnemy;
        CombatAction.TargetLocation = AdventureLocation;
        CombatAction.ExpectedDuration = 5.0f; // 戦闘時間
        CombatAction.ActionReason = FString::Printf(TEXT("Starting combat at %s"), *AdventureLocation);
        return CombatAction;
    }
}

FCharacterAction UCharacterBrain::DecideWaitAction(const FCharacterSituation& Situation, const FString& Reason)
{
    FCharacterAction WaitAction;
    WaitAction.ActionType = ECharacterActionType::Wait;
    WaitAction.TargetLocation = Situation.CurrentLocation;
    WaitAction.ExpectedDuration = 1.0f;
    WaitAction.ActionReason = Reason;
    return WaitAction;
}

FString UCharacterBrain::FindGatheringLocation(const FString& TargetItem)
{
    // 既存のLocationDataTableManagerを使用した場所検索（TimeManagerロジックの移植）
    if (!IsValid(GetWorld()))
    {
        return TEXT("");
    }
    
    ULocationDataTableManager* LocationManager = GetWorld()->GetGameInstance()->GetSubsystem<ULocationDataTableManager>();
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠 CharacterBrain: LocationDataTableManager not found"));
        return TEXT("");
    }
    
    // 平野での採集を優先（既存ロジックと同じ）
    TArray<FString> GatherableItems;
    FLocationDataRow PlainsData;
    if (LocationManager->GetLocationData(TEXT("plains"), PlainsData))
    {
        PlainsData.GetGatherableItemIds(GatherableItems);
        if (GatherableItems.Contains(TargetItem))
        {
            return TEXT("plains");
        }
    }
    
    // 拠点での採集もチェック
    FLocationDataRow BaseData;
    if (LocationManager->GetLocationData(TEXT("base"), BaseData))
    {
        GatherableItems.Empty();
        BaseData.GetGatherableItemIds(GatherableItems);
        if (GatherableItems.Contains(TargetItem))
        {
            return TEXT("base");
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🧠 CharacterBrain: No gathering location found for item: %s"), *TargetItem);
    return TEXT("");
}

bool UCharacterBrain::ShouldReturnToBase(const FCharacterSituation& Situation)
{
    // 既にベースにいる場合は帰還不要
    if (Situation.CurrentLocation == TEXT("base"))
    {
        return false;
    }
    
    // 1. タスク完了時の帰還判定（最重要）
    FString TargetItem = GetTargetItemForTeam(Situation.MyTeamIndex, Situation.CurrentLocation);
    if (TargetItem.IsEmpty())
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🏠 %s: No target item at current location, should return to base"), 
            CharacterRef ? *CharacterRef->GetName() : TEXT("Unknown"));
        return true;
    }
    
    // 2. インベントリ容量チェック
    if (CharacterRef && CharacterRef->GetInventoryComponent())
    {
        TMap<FString, int32> AllItems = CharacterRef->GetInventoryComponent()->GetAllItems();
        int32 TotalItems = 0;
        for (const auto& Item : AllItems)
        {
            TotalItems += Item.Value;
        }
        
        // 20個以上持っている場合は帰還
        if (TotalItems >= 20)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🏠 %s: Inventory full (%d items), should return to base"), 
                CharacterRef ? *CharacterRef->GetName() : TEXT("Unknown"), TotalItems);
            return true;
        }
    }
    
    // 3. 体力が低い場合は帰還（簡易判定）
    if (Situation.CurrentHealth < 50.0f)
    {
        return true;
    }
    
    // 4. スタミナが低い場合は帰還
    if (Situation.CurrentStamina < 30.0f)
    {
        return true;
    }
    
    return false;
}

bool UCharacterBrain::HasItemsToUnload(const FCharacterSituation& Situation)
{
    // このキャラクター自身のインベントリのみを確認
    // (チーム全体ではなく個々のキャラクターが自分のアイテムを荷下ろしする)
    
    // CharacterRefから現在のキャラクターを取得
    if (!CharacterRef)
    {
        return false;
    }
    
    UInventoryComponent* Inventory = CharacterRef->GetInventoryComponent();
    if (!Inventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠📦 Character %s: No inventory component"), 
            CharacterRef ? *CharacterRef->GetName() : TEXT("Unknown"));
        return false;
    }
    
    TMap<FString, int32> AllItems = Inventory->GetAllItems();
    FString CharacterName = CharacterRef->GetName();
    
    if (AllItems.Num() > 0)
    {
        int32 TotalItems = 0;
        for (const auto& Item : AllItems)
        {
            TotalItems += Item.Value;
            UE_LOG(LogTemp, Warning, TEXT("🧠📦 Character %s has item: %s x%d"), 
                *CharacterName, *Item.Key, Item.Value);
        }
        UE_LOG(LogTemp, Warning, TEXT("🧠📦 Character %s has %d total items to unload"), 
            *CharacterName, TotalItems);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠📦 Character %s has no items to unload"), *CharacterName);
        return false;
    }
}

FString UCharacterBrain::GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId)
{
    // TaskManagerの既存ロジックを使用
    if (!TaskManagerRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠 CharacterBrain: TaskManager reference not available"));
        return TEXT("");
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🎯 CharacterBrain: Requesting target item for Team %d at location %s"), TeamIndex, *LocationId);
    FString TargetItem = TaskManagerRef->GetTargetItemForTeam(TeamIndex, LocationId);
    UE_LOG(LogTemp, VeryVerbose, TEXT("🧠📦 CharacterBrain: TaskManager returned target item: '%s'"), *TargetItem);
    
    return TargetItem;
}

FString UCharacterBrain::GetTeamGatheringLocation(int32 TeamIndex)
{
    if (!TeamComponentRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ CharacterBrain: TeamComponent reference not available"));
        return TEXT("");
    }
    
    FTeam Team = TeamComponentRef->GetTeam(TeamIndex);
    FString GatheringLocation = Team.GatheringLocationId;
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🌱 GetTeamGatheringLocation: Team %d gathering location: %s"), 
        TeamIndex, *GatheringLocation);
    
    return GatheringLocation;
}

FCharacterAction UCharacterBrain::ApplyPersonalityModifiers(const FCharacterAction& BaseAction, const FCharacterSituation& Situation)
{
    FCharacterAction ModifiedAction = BaseAction;
    
    // 性格による修正を適用
    switch (MyPersonality)
    {
        case ECharacterPersonality::Aggressive:
            // 積極的: 戦闘行動の持続時間を短縮
            if (BaseAction.ActionType == ECharacterActionType::AttackEnemy)
            {
                ModifiedAction.ExpectedDuration *= 0.9f;
                ModifiedAction.ActionReason += TEXT(" (Aggressive: faster combat)");
            }
            break;
            
        case ECharacterPersonality::Cautious:
            // 慎重: 危険地域での行動時間を延長
            if (Situation.bDangerousArea)
            {
                ModifiedAction.ExpectedDuration *= 1.2f;
                ModifiedAction.ActionReason += TEXT(" (Cautious: careful in dangerous area)");
            }
            break;
            
        case ECharacterPersonality::Efficient:
            // 効率重視: 作業時間を短縮
            if (BaseAction.ActionType == ECharacterActionType::GatherResources)
            {
                ModifiedAction.ExpectedDuration *= 0.85f;
                ModifiedAction.ActionReason += TEXT(" (Efficient: faster gathering)");
            }
            break;
            
        case ECharacterPersonality::Loyal:
        case ECharacterPersonality::Creative:
        case ECharacterPersonality::Defensive:
        default:
            // デフォルト: 修正なし
            break;
    }
    
    return ModifiedAction;
}

void UCharacterBrain::SetPersonality(ECharacterPersonality NewPersonality)
{
    MyPersonality = NewPersonality;
    UE_LOG(LogTemp, Log, TEXT("🧠 CharacterBrain: Personality set to %d"), (int32)NewPersonality);
}

void UCharacterBrain::LogDecisionProcess(const FCharacterSituation& Situation, const FCharacterAction& Decision)
{
    if (UE_LOG_ACTIVE(LogTemp, VeryVerbose))
    {
        UE_LOG(LogTemp, VeryVerbose, 
            TEXT("🧠 CharacterBrain Decision: Team=%d, Location=%s, Task=%d -> Action=%d (%s)"),
            Situation.MyTeamIndex,
            *Situation.CurrentLocation,
            (int32)Situation.TeamAssignedTask,
            (int32)Decision.ActionType,
            *Decision.ActionReason
        );
    }
}

void UCharacterBrain::InitializePersonalityPreferences()
{
    // 現在の性格に基づいて行動優先度を初期化
    MyActionPreferences.Empty();
    
    switch (MyPersonality)
    {
        case ECharacterPersonality::Aggressive:
        {
            // 積極的: 戦闘・冒険を優先
            FActionPreference CombatPref;
            CombatPref.ActionType = ECharacterActionType::AttackEnemy;
            CombatPref.PreferenceWeight = 8.0f;
            MyActionPreferences.Add(CombatPref);
            break;
        }
        
        case ECharacterPersonality::Cautious:
        {
            // 慎重: 安全行動を優先
            FActionPreference DefendPref;
            DefendPref.ActionType = ECharacterActionType::DefendAlly;
            DefendPref.PreferenceWeight = 7.0f;
            MyActionPreferences.Add(DefendPref);
            break;
        }
        
        case ECharacterPersonality::Efficient:
        {
            // 効率重視: 作業行動を優先
            FActionPreference GatherPref;
            GatherPref.ActionType = ECharacterActionType::GatherResources;
            GatherPref.PreferenceWeight = 8.0f;
            MyActionPreferences.Add(GatherPref);
            break;
        }
        
        case ECharacterPersonality::Loyal:
        case ECharacterPersonality::Creative:
        case ECharacterPersonality::Defensive:
        default:
        {
            // デフォルト: バランス重視
            FActionPreference DefaultPref;
            DefaultPref.ActionType = ECharacterActionType::GatherResources;
            DefaultPref.PreferenceWeight = 5.0f;
            MyActionPreferences.Add(DefaultPref);
            break;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("🧠 CharacterBrain: Action preferences initialized for personality %d"), (int32)MyPersonality);
}

