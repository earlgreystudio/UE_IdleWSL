#include "GatheringComponent.h"
#include "TeamComponent.h"
#include "InventoryComponent.h"
#include "TaskManagerComponent.h"
#include "LocationMovementComponent.h"
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
    // 独立タイマー削除 - TimeManagerComponent統制下で動作
    
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
    
    // 拠点の場合は即座に採集開始、そうでなければ移動開始
    if (LocationData.Distance <= 0.0f)
    {
        SetGatheringState(TeamIndex, EGatheringState::Gathering);
        UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d starting immediate gathering at %s"), TeamIndex, *LocationId);
    }
    else
    {
        SetGatheringState(TeamIndex, EGatheringState::MovingToSite);
        UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d starting movement to %s"), TeamIndex, *LocationId);
        
        // MovementComponentに移動を委譲
        // 注意: MovementComponentの参照は持っていないため、TimeManagerを通じて移動が管理される
        SetMovementProgress(TeamIndex, 0.0f);
    }

    // 独立タイマー削除 - TimeManagerComponent統制下で動作

    UE_LOG(LogTemp, Log, TEXT("GatheringComponent: Started gathering for team %d at location %s"), TeamIndex, *LocationId);
    return true;
}

bool UGatheringComponent::StopGathering(int32 TeamIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("StopGathering: Called for team %d"), TeamIndex);
    
    if (!TeamGatheringStates.Contains(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d not in gathering states, checking if return needed"), TeamIndex);
        
        // 採集状態にないが、チームが拠点以外にいる可能性があるので距離をチェック
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (ULocationMovementComponent* MovementComp = PC->FindComponentByClass<ULocationMovementComponent>())
                {
                    float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
                    UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d distance from base: %.1fm"), TeamIndex, CurrentDistance);
                    
                    if (CurrentDistance > 0.1f) // 拠点にいない場合
                    {
                        // 帰還移動を開始
                        SetGatheringState(TeamIndex, EGatheringState::MovingToBase);
                        bool bMovementStarted = MovementComp->StartReturnToBase(TeamIndex, TEXT("unknown_location"));
                        if (bMovementStarted)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d starting return from distance %.1fm"), TeamIndex, CurrentDistance);
                            if (TeamComponent)
                            {
                                TeamComponent->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
                            }
                            return true;
                        }
                    }
                }
            }
        }
        
        return false;
    }

    // チームが拠点以外にいる場合は拠点に戻る処理を開始
    EGatheringState CurrentState = GetGatheringState(TeamIndex);
    bool bShouldReturnToBase = false;
    
    UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d current state: %d (%s)"), TeamIndex, (int32)CurrentState,
        CurrentState == EGatheringState::Gathering ? TEXT("Gathering") :
        CurrentState == EGatheringState::MovingToSite ? TEXT("MovingToSite") :
        CurrentState == EGatheringState::MovingToBase ? TEXT("MovingToBase") :
        CurrentState == EGatheringState::Unloading ? TEXT("Unloading") :
        CurrentState == EGatheringState::Inactive ? TEXT("Inactive") : TEXT("Unknown"));
    
    if (CurrentState == EGatheringState::Gathering || CurrentState == EGatheringState::MovingToSite)
    {
        // 採集中または採集地への移動中の場合、拠点に戻る
        bShouldReturnToBase = true;
        UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d needs to return to base (current state: %s)"), 
            TeamIndex, 
            CurrentState == EGatheringState::Gathering ? TEXT("Gathering") : TEXT("MovingToSite"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d does not need to return to base (current state: %s)"), TeamIndex,
            CurrentState == EGatheringState::MovingToBase ? TEXT("MovingToBase") :
            CurrentState == EGatheringState::Unloading ? TEXT("Unloading") :
            CurrentState == EGatheringState::Inactive ? TEXT("Inactive") : TEXT("Unknown"));
    }

    if (bShouldReturnToBase)
    {
        // 拠点への帰還を開始
        SetGatheringState(TeamIndex, EGatheringState::MovingToBase);
        
        // MovementComponentを通じて拠点への移動を開始
        // PlayerControllerのMovementComponentにアクセス
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (ULocationMovementComponent* MovementComp = PC->FindComponentByClass<ULocationMovementComponent>())
                {
                    // 現在地を取得
                    FString CurrentLocation = TeamTargetLocations.Contains(TeamIndex) ? TeamTargetLocations[TeamIndex] : TEXT("base");
                    float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
                    
                    // 現在地が拠点以外の場合のみ移動を開始
                    if (CurrentLocation != TEXT("base"))
                    {
                        bool bMovementStarted = MovementComp->StartReturnToBase(TeamIndex, CurrentLocation);
                        if (bMovementStarted)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d started returning to base from %s (distance: %.1fm)"), TeamIndex, *CurrentLocation, CurrentDistance);
                            
                            // チーム状態を移動中に設定
                            if (TeamComponent)
                            {
                                TeamComponent->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
                            }
                            
                            // ターゲット場所は保持（移動完了まで）
                            return true;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("StopGathering: Failed to start return movement for team %d"), TeamIndex);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("StopGathering: Team %d is already at base (distance: %.1fm), no movement needed"), TeamIndex, CurrentDistance);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("StopGathering: MovementComponent not found on PlayerController"));
                }
            }
        }
    }

    // 状態クリア（拠点への移動が不要な場合、または移動開始に失敗した場合）
    TeamGatheringStates.Remove(TeamIndex);
    TeamMovementProgress.Remove(TeamIndex);
    TeamTargetLocations.Remove(TeamIndex);

    // チーム状態をIdleに戻す
    if (TeamComponent)
    {
        TeamComponent->SetTeamActionState(TeamIndex, ETeamActionState::Idle);
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

void UGatheringComponent::ProcessTeamGatheringWithTarget(int32 TeamIndex, const FString& TargetItemId)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessTeamGatheringWithTarget: Team %d targeting %s"), TeamIndex, *TargetItemId);
    
    if (!IsValidTeam(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessTeamGatheringWithTarget: Invalid team %d"), TeamIndex);
        return;
    }
    
    // ターゲットアイテムが指定されていない場合は処理しない
    if (TargetItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessTeamGatheringWithTarget: No target item for team %d"), TeamIndex);
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
            ProcessGatheringExecutionWithTarget(TeamIndex, TargetItemId);
            break;
            
        case EGatheringState::Unloading:
            AutoUnloadResourceItems(TeamIndex);
            break;
            
        default:
            // Inactive状態の場合は何もしない
            break;
    }
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
                
                // 個数指定タイプのタスクの目標量を減らす
                UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecution: Calling ReduceSpecifiedTaskQuantity for %s x%d"), 
                    *ItemInfo.ItemId, GatheredAmount);
                ReduceSpecifiedTaskQuantity(ItemInfo.ItemId, GatheredAmount);
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

void UGatheringComponent::ProcessGatheringExecutionWithTarget(int32 TeamIndex, const FString& TargetItemId)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringExecutionWithTarget: Team %d targeting %s"), TeamIndex, *TargetItemId);
    
    if (!TeamTargetLocations.Contains(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecutionWithTarget: Team %d not in target locations"), TeamIndex);
        return;
    }

    FString LocationId = TeamTargetLocations[TeamIndex];
    FLocationDataRow LocationData = GetLocationData(LocationId);
    TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringExecutionWithTarget: Team %d at %s, found %d gatherable items"), 
        TeamIndex, *LocationId, GatherableItems.Num());
    
    if (GatherableItems.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecutionWithTarget: No gatherable items at %s"), *LocationId);
        return;
    }

    float TeamGatheringPower = CalculateTeamGatheringPower(TeamIndex);
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringExecutionWithTarget: Team %d has gathering power %.2f"), 
        TeamIndex, TeamGatheringPower);
    
    // チームの積載状況をチェック
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessGatheringExecutionWithTarget: Invalid team component"));
        return;
    }
    
    // 指定されたアイテムのみを採集（目的外アイテムは無視）
    bool bFoundTargetItem = false;
    for (const FGatherableItemInfo& ItemInfo : GatherableItems)
    {
        if (ItemInfo.ItemId != TargetItemId)
        {
            continue; // 目的アイテム以外は無視
        }
        
        bFoundTargetItem = true;
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringExecutionWithTarget: Processing target item %s"), *TargetItemId);
        
        // 採取量計算
        float BaseGatherRate = (TeamGatheringPower * ItemInfo.GatheringCoefficient) / GatheringEfficiencyMultiplier;
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringExecutionWithTarget: Item %s - coefficient %.2f, base rate %.4f"), 
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

        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringExecutionWithTarget: Item %s - calculated amount: %d"), 
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
                
                UE_LOG(LogTemp, Log, TEXT("Team %d gathered %d %s (target)"), TeamIndex, GatheredAmount, *ItemInfo.ItemId);
                
                // 個数指定タイプのタスクの目標量を減らす
                UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecutionWithTarget: Calling ReduceSpecifiedTaskQuantity for %s x%d"), 
                    *ItemInfo.ItemId, GatheredAmount);
                ReduceSpecifiedTaskQuantity(ItemInfo.ItemId, GatheredAmount);
            }
            else
            {
                // 積載量満杯で拠点へ帰還
                UE_LOG(LogTemp, Warning, TEXT("Team %d inventory full, returning to base"), TeamIndex);
                SetMovementProgress(TeamIndex, 0.0f);
                SetGatheringState(TeamIndex, EGatheringState::MovingToBase);
                OnInventoryFull.Broadcast(TeamIndex);
            }
        }
        
        break; // 目的アイテムを処理したら終了
    }
    
    if (!bFoundTargetItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringExecutionWithTarget: Target item %s not available at %s"), 
            *TargetItemId, *LocationId);
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
        UE_LOG(LogTemp, Warning, TEXT("AutoUnloadResourceItems: Checking %d gatherable items for continuation"), GatherableItems.Num());
        for (const FGatherableItemInfo& ItemInfo : GatherableItems)
        {
            UE_LOG(LogTemp, Warning, TEXT("AutoUnloadResourceItems: Checking if should continue gathering %s"), *ItemInfo.ItemId);
            bool bItemShouldContinue = TaskManager->ShouldContinueGathering(TeamIndex, ItemInfo.ItemId);
            UE_LOG(LogTemp, Warning, TEXT("AutoUnloadResourceItems: ShouldContinueGathering for %s returned: %s"), 
                *ItemInfo.ItemId, bItemShouldContinue ? TEXT("Yes") : TEXT("No"));
                
            if (bItemShouldContinue)
            {
                bShouldContinue = true;
                UE_LOG(LogTemp, Warning, TEXT("AutoUnloadResourceItems: Team %d should continue gathering %s"), TeamIndex, *ItemInfo.ItemId);
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

void UGatheringComponent::OnMovementCompleted(int32 TeamIndex, const FString& ArrivedLocation)
{
    UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d completed movement to %s"), TeamIndex, *ArrivedLocation);
    
    EGatheringState CurrentState = GetGatheringState(TeamIndex);
    
    if (CurrentState == EGatheringState::MovingToSite)
    {
        // 採集地に到着 → 採集開始
        SetGatheringState(TeamIndex, EGatheringState::Gathering);
        UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d started gathering at %s"), TeamIndex, *ArrivedLocation);
    }
    else if (CurrentState == EGatheringState::MovingToBase)
    {
        // 拠点に到着 - タスクがまだ有効かチェック
        bool bHasValidTask = false;
        if (TeamTargetLocations.Contains(TeamIndex) && IsValid(TaskManager))
        {
            FString LocationId = TeamTargetLocations[TeamIndex];
            FLocationDataRow LocationData = GetLocationData(LocationId);
            TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
            
            // 採集可能なアイテムでアクティブなタスクがあるかチェック
            for (const FGatherableItemInfo& ItemInfo : GatherableItems)
            {
                if (TaskManager->ShouldContinueGathering(TeamIndex, ItemInfo.ItemId))
                {
                    bHasValidTask = true;
                    break;
                }
            }
        }
        
        if (bHasValidTask)
        {
            // 通常の拠点帰還（荷下ろし）
            SetGatheringState(TeamIndex, EGatheringState::Unloading);
            UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d started unloading at base"), TeamIndex);
        }
        else
        {
            // タスク削除による強制帰還 - 即座にIdle状態にする
            UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d returned to base due to task cancellation, setting to idle"), TeamIndex);
            
            // 状態クリア
            TeamGatheringStates.Remove(TeamIndex);
            TeamMovementProgress.Remove(TeamIndex);
            TeamTargetLocations.Remove(TeamIndex);
            
            // チーム状態をIdleに戻す
            if (TeamComponent)
            {
                TeamComponent->SetTeamActionState(TeamIndex, ETeamActionState::Idle);
                TeamComponent->SetTeamTask(TeamIndex, ETaskType::Idle);
                UE_LOG(LogTemp, Warning, TEXT("GatheringComponent: Team %d task and action state set to Idle after return"), TeamIndex);
            }
        }
    }
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

void UGatheringComponent::ReduceSpecifiedTaskQuantity(const FString& ItemId, int32 ReduceAmount)
{
    UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Called for %s x%d"), *ItemId, ReduceAmount);
    
    if (!IsValid(TaskManager))
    {
        UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: TaskManager is null"));
        return;
    }
    
    // 該当アイテムの「個数指定」タイプのタスクを検索
    TArray<FGlobalTask> AllTasks = TaskManager->GetGlobalTasks();
    UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Found %d total tasks"), AllTasks.Num());
    
    for (int32 TaskIndex = 0; TaskIndex < AllTasks.Num(); TaskIndex++)
    {
        const FGlobalTask& Task = AllTasks[TaskIndex];
        
        UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Task %d - Type: %d, ItemId: %s, GatheringType: %d, Completed: %s"), 
            TaskIndex, (int32)Task.TaskType, *Task.TargetItemId, (int32)Task.GatheringQuantityType, 
            Task.bIsCompleted ? TEXT("Yes") : TEXT("No"));
        
        // 個数指定タイプの採集タスクのみ対象
        if (Task.TaskType == ETaskType::Gathering && 
            Task.TargetItemId == ItemId && 
            Task.GatheringQuantityType == EGatheringQuantityType::Specified &&
            !Task.bIsCompleted)
        {
            int32 NewTargetQuantity = FMath::Max(0, Task.TargetQuantity - ReduceAmount);
            
            UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Item %s - Task %s reduced from %d to %d"), 
                *ItemId, *Task.TaskId, Task.TargetQuantity, NewTargetQuantity);
            
            // TaskManagerを通じてタスクの目標量を更新
            TaskManager->UpdateTaskTargetQuantity(TaskIndex, NewTargetQuantity);
            
            // 目標量が0になったらタスクを完了・削除
            if (NewTargetQuantity <= 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Task %s reached 0 quantity, completing and removing"), *Task.TaskId);
                
                // 先にチームを停止（タスク削除前に）
                UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Stopping teams gathering %s before task removal"), *ItemId);
                StopGatheringForItem(ItemId);
                
                // その後タスクを完了・削除
                TaskManager->CompleteTask(TaskIndex);
                TaskManager->RemoveGlobalTask(TaskIndex);
                UE_LOG(LogTemp, Warning, TEXT("ReduceSpecifiedTaskQuantity: Task %s successfully removed"), *Task.TaskId);
            }
            
            // 複数のタスクがある場合は最初の1つだけ処理
            break;
        }
    }
}

void UGatheringComponent::StopGatheringForItem(const FString& ItemId)
{
    UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Called for item %s"), *ItemId);
    
    if (!IsValid(TeamComponent) || !IsValid(LocationManager))
    {
        UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Missing components"));
        return;
    }
    
    // 全てのアクティブなチームをチェック
    TArray<int32> TeamsToStop;
    for (const auto& TeamStatePair : TeamGatheringStates)
    {
        int32 TeamIndex = TeamStatePair.Key;
        
        // そのチームが対象アイテムを採集中かチェック
        if (TeamTargetLocations.Contains(TeamIndex))
        {
            FString LocationId = TeamTargetLocations[TeamIndex];
            FLocationDataRow LocationData = GetLocationData(LocationId);
            TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
            
            // この場所で対象アイテムが採集可能かチェック
            for (const FGatherableItemInfo& ItemInfo : GatherableItems)
            {
                if (ItemInfo.ItemId == ItemId)
                {
                    TeamsToStop.Add(TeamIndex);
                    UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Team %d will be stopped (gathering %s at %s)"), 
                        TeamIndex, *ItemId, *LocationId);
                    break;
                }
            }
        }
    }
    
    // 対象チームの採集を停止
    for (int32 TeamIndex : TeamsToStop)
    {
        StopGathering(TeamIndex);
        
        // そのチームが他の採集可能アイテムのアクティブタスクを持っているかチェック
        bool bHasOtherGatheringTasks = false;
        if (TeamTargetLocations.Contains(TeamIndex))
        {
            FString LocationId = TeamTargetLocations[TeamIndex];
            FLocationDataRow LocationData = GetLocationData(LocationId);
            TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
            
            for (const FGatherableItemInfo& ItemInfo : GatherableItems)
            {
                if (ItemInfo.ItemId != ItemId && IsValid(TaskManager))
                {
                    FGlobalTask ActiveTask = TaskManager->FindActiveGatheringTask(ItemInfo.ItemId);
                    if (!ActiveTask.TaskId.IsEmpty())
                    {
                        bHasOtherGatheringTasks = true;
                        UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Team %d has other gathering task for %s"), TeamIndex, *ItemInfo.ItemId);
                        break;
                    }
                }
            }
        }
        
        // 他に採集タスクがない場合でも、帰還中なら Gathering を維持
        if (!bHasOtherGatheringTasks && IsValid(TeamComponent))
        {
            EGatheringState CurrentGatheringState = GetGatheringState(TeamIndex);
            if (CurrentGatheringState == EGatheringState::MovingToBase)
            {
                // 帰還中は Gathering タスクを維持（帰還完了後に Idle に設定）
                UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Team %d returning to base, keeping Gathering task until return completes"), TeamIndex);
            }
            else
            {
                // 既に拠点にいる場合のみ Idle に設定
                TeamComponent->SetTeamTask(TeamIndex, ETaskType::Idle);
                UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Cleared team %d task assignment (no more gathering tasks)"), TeamIndex);
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Stopped team %d from gathering %s"), TeamIndex, *ItemId);
    }
    
    if (TeamsToStop.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: Stopped %d teams from gathering %s"), TeamsToStop.Num(), *ItemId);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StopGatheringForItem: No teams were gathering %s"), *ItemId);
    }
}