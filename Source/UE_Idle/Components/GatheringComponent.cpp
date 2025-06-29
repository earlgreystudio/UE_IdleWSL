#include "GatheringComponent.h"
#include "TeamComponent.h"
#include "InventoryComponent.h"
#include "TaskManagerComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Managers/ItemDataTableManager.h"
#include "../Managers/LocationDataTableManager.h"
#include "../Managers/CharacterPresetManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UGatheringComponent::UGatheringComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    GatheringUpdateInterval = 1.0f;
    BaseMovementSpeed = 30.0f;
    GatheringEfficiencyMultiplier = 40.0f;
    CarrierGatheringThreshold = 10.0f;
    CarrierCapacityThreshold = 50.0f;
}

void UGatheringComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Manager参照取得
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        if (!ItemManager)
        {
            UE_LOG(LogTemp, Error, TEXT("GatheringComponent: ItemDataTableManager not found!"));
        }

        LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>();
        if (!LocationManager)
        {
            UE_LOG(LogTemp, Error, TEXT("GatheringComponent: LocationDataTableManager not found!"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Initialized"));
}

void UGatheringComponent::BeginDestroy()
{
    // タイマークリア
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GatheringUpdateTimerHandle);
    }
    
    // 状態クリア
    TeamGatheringStates.Empty();
    TeamMovementProgress.Empty();
    TeamTargetLocations.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Destroyed"));
    
    Super::BeginDestroy();
}

// === 採集制御 ===

bool UGatheringComponent::StartGathering(int32 TeamIndex, const FString& LocationId)
{
    if (!IsValidTeam(TeamIndex))
    {
        LogGatheringError(FString::Printf(TEXT("StartGathering: Invalid team index %d"), TeamIndex));
        return false;
    }

    // 場所データ取得
    FLocationDataRow LocationData = GetLocationData(LocationId);
    if (LocationData.Name.IsEmpty())
    {
        LogGatheringError(FString::Printf(TEXT("StartGathering: Invalid location %s"), *LocationId));
        return false;
    }

    // 採集可能アイテムがあるかチェック
    if (!LocationData.HasGatherableItems())
    {
        LogGatheringError(FString::Printf(TEXT("StartGathering: No gatherable items at location %s"), *LocationId));
        return false;
    }

    // 状態初期化
    TeamTargetLocations.Add(TeamIndex, LocationId);
    SetMovementProgress(TeamIndex, 0.0f);
    
    // 拠点の場合は即座に採集開始、そうでなければ移動開始
    if (LocationData.Distance <= 0.0f)
    {
        SetGatheringState(TeamIndex, EGatheringState::Gathering);
    }
    else
    {
        SetGatheringState(TeamIndex, EGatheringState::MovingToSite);
    }

    // タイマー開始（まだ動いていない場合）
    if (!GetWorld()->GetTimerManager().IsTimerActive(GatheringUpdateTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            GatheringUpdateTimerHandle,
            this,
            &UGatheringComponent::UpdateGathering,
            GatheringUpdateInterval,
            true
        );
    }

    UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Started gathering for team %d at location %s"), TeamIndex, *LocationId);
    return true;
}

bool UGatheringComponent::StopGathering(int32 TeamIndex)
{
    if (!TeamGatheringStates.Contains(TeamIndex))
    {
        return false;
    }

    // 状態クリア
    TeamGatheringStates.Remove(TeamIndex);
    TeamMovementProgress.Remove(TeamIndex);
    TeamTargetLocations.Remove(TeamIndex);

    // チーム状態をIdleに戻す
    if (TeamComponent)
    {
        TeamComponent->SetTeamActionState(TeamIndex, ETeamActionState::Idle);
    }

    // 他にアクティブなチームがない場合はタイマー停止
    if (TeamGatheringStates.Num() == 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(GatheringUpdateTimerHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Stopped gathering for team %d"), TeamIndex);
    return true;
}

EGatheringState UGatheringComponent::GetGatheringState(int32 TeamIndex) const
{
    if (TeamGatheringStates.Contains(TeamIndex))
    {
        return TeamGatheringStates[TeamIndex];
    }
    return EGatheringState::Inactive;
}

float UGatheringComponent::GetMovementProgress(int32 TeamIndex) const
{
    if (TeamMovementProgress.Contains(TeamIndex))
    {
        return TeamMovementProgress[TeamIndex];
    }
    return 0.0f;
}

// === 採集処理 ===

void UGatheringComponent::UpdateGathering()
{
    // 再入防止
    if (bProcessingUpdate)
    {
        UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: UpdateGathering skipped - already processing"));
        return;
    }
    
    bProcessingUpdate = true;
    
    // 安全なイテレーションのためにキーのコピーを作成
    TArray<int32> ActiveTeams;
    TeamGatheringStates.GetKeys(ActiveTeams);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("GatheringComponent: UpdateGathering processing %d teams"), ActiveTeams.Num());
    
    // コピーしたキーを使用してイテレーション
    for (int32 TeamIndex : ActiveTeams)
    {
        // チームがまだアクティブかチェック
        if (TeamGatheringStates.Contains(TeamIndex))
        {
            ProcessTeamGathering(TeamIndex);
        }
    }
    
    bProcessingUpdate = false;
}

void UGatheringComponent::ProcessTeamGathering(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    EGatheringState CurrentState = GetGatheringState(TeamIndex);
    
    switch (CurrentState)
    {
        case EGatheringState::MovingToSite:
        case EGatheringState::MovingToBase:
            ProcessMovement(TeamIndex);
            break;
            
        case EGatheringState::Gathering:
            ProcessGatheringExecution(TeamIndex);
            break;
            
        case EGatheringState::Unloading:
            AutoUnloadResourceItems(TeamIndex);
            break;
            
        default:
            break;
    }
}

void UGatheringComponent::ProcessMovement(int32 TeamIndex)
{
    if (!TeamTargetLocations.Contains(TeamIndex))
    {
        return;
    }

    FString LocationId = TeamTargetLocations[TeamIndex];
    FLocationDataRow LocationData = GetLocationData(LocationId);
    
    if (LocationData.Distance <= 0.0f)
    {
        // 距離0なら即座に到着
        SetMovementProgress(TeamIndex, 1.0f);
    }
    else
    {
        // 移動速度に基づいて進捗更新
        float TeamSpeed = CalculateTeamMovementSpeed(TeamIndex);
        float DistancePerTick = TeamSpeed * GatheringUpdateInterval;
        float CurrentProgress = GetMovementProgress(TeamIndex);
        float NewProgress = CurrentProgress + (DistancePerTick / LocationData.Distance);
        
        SetMovementProgress(TeamIndex, FMath::Clamp(NewProgress, 0.0f, 1.0f));
    }

    // 移動完了チェック
    if (GetMovementProgress(TeamIndex) >= 1.0f)
    {
        EGatheringState CurrentState = GetGatheringState(TeamIndex);
        
        if (CurrentState == EGatheringState::MovingToSite)
        {
            // 採集地到着
            SetGatheringState(TeamIndex, EGatheringState::Gathering);
            UE_LOG(LogTemp, Log, TEXT("Team %d arrived at gathering site %s"), TeamIndex, *LocationId);
        }
        else if (CurrentState == EGatheringState::MovingToBase)
        {
            // 拠点到着
            SetGatheringState(TeamIndex, EGatheringState::Unloading);
            UE_LOG(LogTemp, Log, TEXT("Team %d returned to base"), TeamIndex);
        }
    }
}

void UGatheringComponent::ProcessGatheringExecution(int32 TeamIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Called for team %d"), TeamIndex);
    
    if (!TeamTargetLocations.Contains(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Team %d not in target locations"), TeamIndex);
        return;
    }

    FString LocationId = TeamTargetLocations[TeamIndex];
    FLocationDataRow LocationData = GetLocationData(LocationId);
    TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
    
    UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Team %d at %s, found %d gatherable items"), 
        TeamIndex, *LocationId, GatherableItems.Num());
    
    if (GatherableItems.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: No gatherable items at %s"), *LocationId);
        return;
    }

    float TeamGatheringPower = CalculateTeamGatheringPower(TeamIndex);
    UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Team %d has gathering power %.2f"), 
        TeamIndex, TeamGatheringPower);
    
    // チームの積載状況をチェック
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessGatheringExecution: Invalid team component"));
        return;
    }
    
    FTeam Team = TeamComponent->GetTeam(TeamIndex);
    for (int32 i = 0; i < Team.Members.Num(); i++)
    {
        AC_IdleCharacter* Member = Team.Members[i];
        if (IsValid(Member) && Member->GetStatusComponent())
        {
            // Try GetInventoryComponent first, then FindComponentByClass as fallback
            UInventoryComponent* MemberInv = Member->GetInventoryComponent();
            if (!MemberInv)
            {
                MemberInv = Member->FindComponentByClass<UInventoryComponent>();
            }
            
            if (MemberInv)
            {
                float MaxCapacity = Member->GetStatusComponent()->GetCarryingCapacity();
                float CurrentWeight = MemberInv->GetTotalWeight();
                UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Member %d (%s) - Capacity: %.2f, Current: %.2f, Available: %.2f"), 
                    i, *Member->GetName(), MaxCapacity, CurrentWeight, MaxCapacity - CurrentWeight);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Member %d (%s) has no inventory component"), 
                    i, *Member->GetName());
            }
        }
    }
    
    // 各アイテムを採集
    for (const FGatherableItemInfo& ItemInfo : GatherableItems)
    {
        // 採取量計算
        float BaseGatherRate = (TeamGatheringPower * ItemInfo.GatheringCoefficient) / GatheringEfficiencyMultiplier;
        
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Item %s - coefficient %.2f, base rate %.4f"), 
            *ItemInfo.ItemId, ItemInfo.GatheringCoefficient, BaseGatherRate);
        
        int32 GatheredAmount = 0;
        
        if (BaseGatherRate >= 1.0f)
        {
            // 1以上なら整数部分を確定獲得
            GatheredAmount = FMath::FloorToInt(BaseGatherRate);
            
            // 小数部分は確率で追加1個
            float ChanceForExtra = BaseGatherRate - GatheredAmount;
            if (FMath::FRand() < ChanceForExtra)
            {
                GatheredAmount++;
            }
        }
        else
        {
            // 1未満なら確率判定
            if (FMath::FRand() < BaseGatherRate)
            {
                GatheredAmount = 1;
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Item %s - calculated amount: %d"), 
            *ItemInfo.ItemId, GatheredAmount);

        // アイテム獲得処理
        if (GatheredAmount > 0)
        {
            if (DistributeItemToTeam(TeamIndex, ItemInfo.ItemId, GatheredAmount))
            {
                // 成功時のイベント通知
                FGatheringResult Result;
                Result.ItemId = ItemInfo.ItemId;
                Result.Quantity = GatheredAmount;
                Result.CharacterName = TEXT("Team"); // TODO: 実際に受け取ったキャラ名
                
                OnItemGathered.Broadcast(TeamIndex, Result);
                
                UE_LOG(LogTemp, Log, TEXT("Team %d gathered %d %s"), TeamIndex, GatheredAmount, *ItemInfo.ItemId);
            }
            else
            {
                // 積載量満杯で拠点へ帰還
                UE_LOG(LogTemp, Warning, TEXT("Team %d inventory full, returning to base"), TeamIndex);
                SetMovementProgress(TeamIndex, 0.0f);
                SetGatheringState(TeamIndex, EGatheringState::MovingToBase);
                OnInventoryFull.Broadcast(TeamIndex);
                break;
            }
        }
    }
}

// === アイテム配分システム ===

bool UGatheringComponent::DistributeItemToTeam(int32 TeamIndex, const FString& ItemId, int32 Quantity)
{
    UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Attempting to distribute %d %s to team %d"), 
        Quantity, *ItemId, TeamIndex);
    
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("DistributeItemToTeam: Invalid team %d"), TeamIndex);
        return false;
    }

    FTeam Team = TeamComponent->GetTeam(TeamIndex);
    TArray<AC_IdleCharacter*> SortedMembers = Team.Members;
    
    UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Team %d has %d members"), 
        TeamIndex, SortedMembers.Num());
    
    // 運搬キャラ優先でソート
    SortedMembers.Sort([this](const AC_IdleCharacter& A, const AC_IdleCharacter& B) {
        bool AIsCarrier = IsCarrierCharacter(const_cast<AC_IdleCharacter*>(&A));
        bool BIsCarrier = IsCarrierCharacter(const_cast<AC_IdleCharacter*>(&B));
        
        if (AIsCarrier != BIsCarrier)
        {
            return AIsCarrier; // 運搬キャラを優先
        }
        
        // 両方同じタイプなら積載量の空きで判定
        float AAvailable = 0.0f;
        float BAvailable = 0.0f;
        
        if (A.GetStatusComponent())
        {
            UInventoryComponent* AInv = A.GetInventoryComponent();
            if (!AInv) AInv = A.FindComponentByClass<UInventoryComponent>();
            if (AInv)
            {
                AAvailable = A.GetStatusComponent()->GetCarryingCapacity() - AInv->GetTotalWeight();
            }
        }
        
        if (B.GetStatusComponent())
        {
            UInventoryComponent* BInv = B.GetInventoryComponent();
            if (!BInv) BInv = B.FindComponentByClass<UInventoryComponent>();
            if (BInv)
            {
                BAvailable = B.GetStatusComponent()->GetCarryingCapacity() - BInv->GetTotalWeight();
            }
        }
        
        return AAvailable > BAvailable;
    });

    // アイテム配分
    int32 RemainingQuantity = Quantity;
    
    UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Starting distribution of %d items"), RemainingQuantity);
    
    for (int32 MemberIndex = 0; MemberIndex < SortedMembers.Num(); MemberIndex++)
    {
        AC_IdleCharacter* Member = SortedMembers[MemberIndex];
        if (!IsValid(Member) || RemainingQuantity <= 0)
        {
            break;
        }

        UInventoryComponent* MemberInventory = Member->GetInventoryComponent();
        
        // Fallback: try to find component if GetInventoryComponent returns null
        if (!MemberInventory)
        {
            MemberInventory = Member->FindComponentByClass<UInventoryComponent>();
        }
        
        if (!MemberInventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Member %d (%s) has no inventory component"), 
                MemberIndex, *Member->GetName());
            UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Member class: %s"), 
                *Member->GetClass()->GetName());
            continue;
        }

        // 現在の積載状況をログ
        float MaxCapacity = Member->GetStatusComponent()->GetCarryingCapacity();
        float CurrentWeight = MemberInventory->GetTotalWeight();
        UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Member %d (%s) - Capacity: %.2f, Current: %.2f"), 
            MemberIndex, *Member->GetName(), MaxCapacity, CurrentWeight);

        // 追加可能な数量を計算
        int32 CanAdd = 0;
        for (int32 i = 1; i <= RemainingQuantity; i++)
        {
            bool CanAddResult = MemberInventory->CanAddItemByWeight(ItemId, i);
            UE_LOG(LogTemp, VeryVerbose, TEXT("DistributeItemToTeam: CanAddItemByWeight(%s, %d) = %s"), 
                *ItemId, i, CanAddResult ? TEXT("true") : TEXT("false"));
            
            if (CanAddResult)
            {
                CanAdd = i;
            }
            else
            {
                break;
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Member %d (%s) can add %d items"), 
            MemberIndex, *Member->GetName(), CanAdd);

        if (CanAdd > 0)
        {
            MemberInventory->AddItem(ItemId, CanAdd);
            RemainingQuantity -= CanAdd;
            UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Added %d items to member %d, remaining: %d"), 
                CanAdd, MemberIndex, RemainingQuantity);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("DistributeItemToTeam: Distribution complete, remaining: %d, success: %s"), 
        RemainingQuantity, RemainingQuantity == 0 ? TEXT("true") : TEXT("false"));

    // 全て配分できたかチェック
    return RemainingQuantity == 0;
}

bool UGatheringComponent::IsCarrierCharacter(AC_IdleCharacter* Character) const
{
    if (!IsValid(Character))
    {
        return false;
    }

    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (!StatusComp)
    {
        return false;
    }

    float GatheringPower = StatusComp->GetGatheringPower();
    float CarryingCapacity = StatusComp->GetCarryingCapacity();
    
    // 採集能力が低く、運搬能力が高い = 運搬キャラ
    return (GatheringPower < CarrierGatheringThreshold && CarryingCapacity > CarrierCapacityThreshold);
}

float UGatheringComponent::GetTeamAvailableCapacity(int32 TeamIndex) const
{
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        return 0.0f;
    }

    FTeam Team = TeamComponent->GetTeam(TeamIndex);
    float TotalAvailable = 0.0f;
    
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (IsValid(Member))
        {
            UCharacterStatusComponent* StatusComp = Member->GetStatusComponent();
            UInventoryComponent* Inventory = Member->GetInventoryComponent();
            
            // Fallback search if GetInventoryComponent returns null
            if (!Inventory)
            {
                Inventory = Member->FindComponentByClass<UInventoryComponent>();
            }
            
            if (StatusComp && Inventory)
            {
                float MaxCapacity = StatusComp->GetCarryingCapacity();
                float CurrentWeight = Inventory->GetTotalWeight();
                TotalAvailable += FMath::Max(0.0f, MaxCapacity - CurrentWeight);
            }
        }
    }
    
    return TotalAvailable;
}

// === 自動荷下ろしシステム ===

void UGatheringComponent::AutoUnloadResourceItems(int32 TeamIndex)
{
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        return;
    }

    UInventoryComponent* BaseStorage = GetBaseStorage();
    if (!BaseStorage)
    {
        LogGatheringError(TEXT("AutoUnloadResourceItems: Base storage not found"));
        return;
    }

    FTeam Team = TeamComponent->GetTeam(TeamIndex);
    
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (!IsValid(Member))
        {
            continue;
        }

        UInventoryComponent* MemberInventory = Member->GetInventoryComponent();
        if (!MemberInventory)
        {
            MemberInventory = Member->FindComponentByClass<UInventoryComponent>();
        }
        if (!MemberInventory)
        {
            continue;
        }

        TArray<FInventorySlot> AllSlots = MemberInventory->GetAllSlots();
        
        for (const FInventorySlot& Slot : AllSlots)
        {
            if (IsResourceItem(Slot.ItemId))
            {
                int32 Quantity = Slot.Quantity;
                if (MemberInventory->RemoveItem(Slot.ItemId, Quantity))
                {
                    BaseStorage->AddItem(Slot.ItemId, Quantity);
                    UE_LOG(LogTemp, Log, TEXT("Auto unloaded: %s x%d from %s to base storage"), 
                           *Slot.ItemId, Quantity, *Member->GetName());
                }
            }
        }
    }

    // **修正**: 荷下ろし完了後の継続判定処理
    bool bShouldContinue = false;
    
    if (IsValid(TaskManager) && TeamTargetLocations.Contains(TeamIndex))
    {
        FString LocationId = TeamTargetLocations[TeamIndex];
        FLocationDataRow LocationData = GetLocationData(LocationId);
        TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
        
        // 各採集可能アイテムについて継続判定
        for (const FGatherableItemInfo& ItemInfo : GatherableItems)
        {
            if (TaskManager->ShouldContinueGathering(TeamIndex, ItemInfo.ItemId))
            {
                bShouldContinue = true;
                UE_LOG(LogTemp, Log, TEXT("Team %d should continue gathering %s"), TeamIndex, *ItemInfo.ItemId);
                break;
            }
        }
    }
    
    if (bShouldContinue)
    {
        // 採集継続
        SetGatheringState(TeamIndex, EGatheringState::Gathering);
        SetMovementProgress(TeamIndex, 0.0f);
        UE_LOG(LogTemp, Log, TEXT("Team %d continuing gathering after unload"), TeamIndex);
    }
    else
    {
        // 採集停止
        StopGathering(TeamIndex);
        UE_LOG(LogTemp, Log, TEXT("Team %d completed gathering task after unload"), TeamIndex);
    }
    
    OnAutoUnloadCompleted.Broadcast(TeamIndex);
}

bool UGatheringComponent::IsResourceItem(const FString& ItemId) const
{
    if (!ItemManager)
    {
        return false;
    }

    FItemDataRow ItemData;
    if (ItemManager->GetItemData(ItemId, ItemData))
    {
        return ItemData.ItemType == EItemTypeTable::Resource;
    }
    
    return false;
}

// === ヘルパー関数 ===

FLocationDataRow UGatheringComponent::GetLocationData(const FString& LocationId) const
{
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GatheringComponent::GetLocationData - LocationManager is null"));
        return FLocationDataRow();
    }

    FLocationDataRow LocationData;
    if (LocationManager->GetLocationData(LocationId, LocationData))
    {
        return LocationData;
    }

    UE_LOG(LogTemp, Warning, TEXT("GatheringComponent::GetLocationData - Location not found: %s"), *LocationId);
    return FLocationDataRow();
}

float UGatheringComponent::CalculateTeamMovementSpeed(int32 TeamIndex) const
{
    // 基本移動速度を返す（将来的にはチームメンバーの最低速度を計算）
    return BaseMovementSpeed;
}

float UGatheringComponent::CalculateTeamGatheringPower(int32 TeamIndex) const
{
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        return 0.0f;
    }

    FTeam Team = TeamComponent->GetTeam(TeamIndex);
    float TotalGatheringPower = 0.0f;
    
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (IsValid(Member))
        {
            UCharacterStatusComponent* StatusComp = Member->GetStatusComponent();
            if (StatusComp)
            {
                TotalGatheringPower += StatusComp->GetGatheringPower();
            }
        }
    }
    
    return TotalGatheringPower;
}

// === コンポーネント登録 ===

void UGatheringComponent::RegisterTeamComponent(UTeamComponent* InTeamComponent)
{
    TeamComponent = InTeamComponent;
    UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Registered TeamComponent"));
}

void UGatheringComponent::RegisterTaskManagerComponent(UTaskManagerComponent* InTaskManagerComponent)
{
    if (IsValid(InTaskManagerComponent))
    {
        TaskManager = InTaskManagerComponent;
        UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Registered TaskManagerComponent"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GatheringComponent: Invalid TaskManagerComponent"));
    }
}

// === 内部ヘルパー ===

void UGatheringComponent::SetGatheringState(int32 TeamIndex, EGatheringState NewState)
{
    EGatheringState OldState = GetGatheringState(TeamIndex);
    TeamGatheringStates.Add(TeamIndex, NewState);
    
    if (OldState != NewState)
    {
        OnGatheringStateChanged.Broadcast(TeamIndex, NewState);
        
        // TeamComponentの状態も更新
        if (TeamComponent)
        {
            ETeamActionState TeamState = ETeamActionState::Idle;
            
            switch (NewState)
            {
                case EGatheringState::MovingToSite:
                case EGatheringState::MovingToBase:
                    TeamState = ETeamActionState::Moving;
                    break;
                case EGatheringState::Gathering:
                case EGatheringState::Unloading:
                    TeamState = ETeamActionState::Working;
                    break;
                default:
                    TeamState = ETeamActionState::Idle;
                    break;
            }
            
            TeamComponent->SetTeamActionState(TeamIndex, TeamState);
        }
    }
}

void UGatheringComponent::SetMovementProgress(int32 TeamIndex, float Progress)
{
    TeamMovementProgress.Add(TeamIndex, FMath::Clamp(Progress, 0.0f, 1.0f));
    OnMovementProgress.Broadcast(TeamIndex, Progress);
}

void UGatheringComponent::LogGatheringError(const FString& ErrorMessage) const
{
    UE_LOG(LogTemp, Error, TEXT("GatheringComponent: %s"), *ErrorMessage);
}

bool UGatheringComponent::IsValidTeam(int32 TeamIndex) const
{
    return TeamComponent && TeamComponent->GetAllTeams().IsValidIndex(TeamIndex);
}

UInventoryComponent* UGatheringComponent::GetBaseStorage() const
{
    // PlayerControllerのInventoryComponentを取得
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->FindComponentByClass<UInventoryComponent>();
        }
    }
    return nullptr;
}