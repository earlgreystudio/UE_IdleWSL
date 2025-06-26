#include "BaseComponent.h"
#include "UE_Idle/Managers/FacilityManager.h"
#include "UE_Idle/Managers/ItemDataTableManager.h"
#include "UE_Idle/Components/GlobalInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBaseComponent::UBaseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;  // 100msごと

    // 初期リソース
    BaseResources.Add("wood", 100);
    BaseResources.Add("stone", 50);
    BaseResources.Add("vegetables", 30);
}

void UBaseComponent::BeginPlay()
{
    Super::BeginPlay();

    // Get references to managers
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        FacilityManager = GameInstance->GetSubsystem<UFacilityManager>();
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
    }

    // Get GlobalInventoryComponent from PlayerController
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        GlobalInventory = PC->FindComponentByClass<UGlobalInventoryComponent>();
    }

    if (!FacilityManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent: FacilityManager not found!"));
    }
    if (!ItemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent: ItemDataTableManager not found!"));
    }

    // 初期設定：人口1人、上限0（住居がない状態）
    CurrentPopulation = 1;
    MaxPopulation = 0;
    AvailableWorkers = 1;
}

void UBaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 生産処理（5秒毎）
    ProductionTimer += DeltaTime;
    if (ProductionTimer >= 5.0f)  // 5秒間隔
    {
        ProcessProduction(ProductionTimer);
        ProductionTimer = 0.0f;
    }

    // メンテナンス処理
    MaintenanceTimer += DeltaTime;
    if (MaintenanceTimer >= MAINTENANCE_INTERVAL)
    {
        ProcessMaintenance(MaintenanceTimer);
        MaintenanceTimer = 0.0f;
    }

    // 建設進捗更新
    if (FacilityManager)
    {
        TArray<FFacilityInstance> ConstructingFacilities = FacilityManager->GetFacilitiesByState(EFacilityState::Constructing);
        for (const FFacilityInstance& Facility : ConstructingFacilities)
        {
            FacilityManager->UpdateConstructionProgress(Facility.InstanceId, DeltaTime, Facility.AssignedWorkers);
        }

        TArray<FFacilityInstance> UpgradingFacilities = FacilityManager->GetFacilitiesByState(EFacilityState::Upgrading);
        for (const FFacilityInstance& Facility : UpgradingFacilities)
        {
            FacilityManager->UpdateConstructionProgress(Facility.InstanceId, DeltaTime, Facility.AssignedWorkers);
        }
    }
}

bool UBaseComponent::AddResource(const FString& ResourceId, int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    int32& CurrentAmount = BaseResources.FindOrAdd(ResourceId, 0);
    CurrentAmount += Amount;

    OnResourceChanged.Broadcast(ResourceId, CurrentAmount);
    UE_LOG(LogTemp, Log, TEXT("BaseComponent: Added %d %s (Total: %d)"), Amount, *ResourceId, CurrentAmount);

    return true;
}

bool UBaseComponent::ConsumeResource(const FString& ResourceId, int32 Amount)
{
    if (Amount <= 0)
    {
        return false;
    }

    int32* CurrentAmount = BaseResources.Find(ResourceId);
    if (!CurrentAmount || *CurrentAmount < Amount)
    {
        return false;
    }

    *CurrentAmount -= Amount;
    OnResourceChanged.Broadcast(ResourceId, *CurrentAmount);

    UE_LOG(LogTemp, Log, TEXT("BaseComponent: Consumed %d %s (Remaining: %d)"), Amount, *ResourceId, *CurrentAmount);

    return true;
}

bool UBaseComponent::HasResource(const FString& ResourceId, int32 RequiredAmount) const
{
    const int32* CurrentAmount = BaseResources.Find(ResourceId);
    return CurrentAmount && *CurrentAmount >= RequiredAmount;
}

int32 UBaseComponent::GetResourceAmount(const FString& ResourceId) const
{
    const int32* Amount = BaseResources.Find(ResourceId);
    return Amount ? *Amount : 0;
}

FGuid UBaseComponent::PlanFacility(const FString& FacilityId, const FVector& Location)
{
    if (!FacilityManager)
    {
        return FGuid();
    }

    // 依存関係チェック
    if (!FacilityManager->CheckDependencies(FacilityId))
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Dependencies not met for %s"), *FacilityId);
        return FGuid();
    }

    FGuid InstanceId = FacilityManager->CreateFacility(FacilityId, Location);
    if (InstanceId.IsValid())
    {
        OnFacilityAdded.Broadcast(InstanceId, FacilityId);
    }

    return InstanceId;
}

bool UBaseComponent::StartFacilityConstruction(const FGuid& InstanceId)
{
    if (!FacilityManager)
    {
        return false;
    }

    FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(InstanceId);
    if (!Instance)
    {
        return false;
    }

    // コスト取得と消費
    TMap<FString, int32> Costs = FacilityManager->GetConstructionCost(Instance->FacilityId);
    if (!ConsumeResourcesForCost(Costs))
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Insufficient resources for construction"));
        return false;
    }

    if (!FacilityManager->StartConstruction(InstanceId, BaseResources))
    {
        // 失敗したらリソースを返却
        RefundResources(Costs);
        return false;
    }

    return true;
}

bool UBaseComponent::UpgradeFacility(const FGuid& InstanceId)
{
    if (!FacilityManager)
    {
        return false;
    }

    FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(InstanceId);
    if (!Instance)
    {
        return false;
    }

    // コスト取得と消費
    TMap<FString, int32> Costs = FacilityManager->GetUpgradeCost(Instance->FacilityId, Instance->Level);
    if (!ConsumeResourcesForCost(Costs))
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Insufficient resources for upgrade"));
        return false;
    }

    if (!FacilityManager->StartUpgrade(InstanceId, BaseResources))
    {
        // 失敗したらリソースを返却
        RefundResources(Costs);
        return false;
    }

    return true;
}

bool UBaseComponent::DemolishFacility(const FGuid& InstanceId)
{
    if (!FacilityManager)
    {
        return false;
    }

    FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(InstanceId);
    if (!Instance)
    {
        return false;
    }

    // 割り当てワーカーを解放
    if (Instance->AssignedWorkers > 0)
    {
        AvailableWorkers += Instance->AssignedWorkers;
        OnWorkerAssignmentChanged.Broadcast(AvailableWorkers);
    }

    // 建材の一部を返却（50%）
    FFacilityDataRow FacilityData;
    if (FacilityManager->GetFacilityData(Instance->FacilityId, FacilityData))
    {
        TMap<FString, int32> RefundAmount;
        for (const FFacilityResourceCost& Cost : FacilityData.ConstructionCosts)
        {
            int32 Amount = FacilityData.GetResourceCostAtLevel(Cost, Instance->Level) / 2;
            RefundAmount.Add(Cost.ItemId, Amount);
        }
        RefundResources(RefundAmount);
    }

    if (FacilityManager->DestroyFacility(InstanceId))
    {
        OnFacilityRemoved.Broadcast(InstanceId);
        UpdateMaxPopulation();
        return true;
    }

    return false;
}

bool UBaseComponent::CanBuildFacility(const FString& FacilityId) const
{
    if (!FacilityManager)
    {
        return false;
    }

    return FacilityManager->CanBuildFacility(FacilityId, BaseResources);
}

bool UBaseComponent::CanUpgradeFacility(const FGuid& InstanceId) const
{
    if (!FacilityManager)
    {
        return false;
    }

    return FacilityManager->CanUpgradeFacility(InstanceId, BaseResources);
}

bool UBaseComponent::AssignWorkersToFacility(const FGuid& InstanceId, int32 WorkerCount)
{
    if (!FacilityManager)
    {
        return false;
    }

    FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(InstanceId);
    if (!Instance)
    {
        return false;
    }

    // 現在の割り当てを考慮
    int32 CurrentAssigned = Instance->AssignedWorkers;
    int32 Difference = WorkerCount - CurrentAssigned;

    if (Difference > 0 && Difference > AvailableWorkers)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Not enough available workers"));
        return false;
    }

    if (FacilityManager->AssignWorkers(InstanceId, WorkerCount))
    {
        AvailableWorkers -= Difference;
        OnWorkerAssignmentChanged.Broadcast(AvailableWorkers);
        return true;
    }

    return false;
}

int32 UBaseComponent::GetAssignedWorkers(const FGuid& InstanceId) const
{
    if (!FacilityManager)
    {
        return 0;
    }

    const FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(InstanceId);
    return Instance ? Instance->AssignedWorkers : 0;
}

void UBaseComponent::RecalculateAvailableWorkers()
{
    if (!FacilityManager)
    {
        return;
    }

    int32 TotalAssigned = FacilityManager->GetTotalAssignedWorkers();
    AvailableWorkers = CurrentPopulation - TotalAssigned;

    OnWorkerAssignmentChanged.Broadcast(AvailableWorkers);
}

bool UBaseComponent::AddPopulation(int32 Count)
{
    if (Count <= 0)
    {
        return false;
    }

    if (CurrentPopulation + Count > MaxPopulation)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Population would exceed max capacity"));
        return false;
    }

    CurrentPopulation += Count;
    AvailableWorkers += Count;

    OnPopulationChanged.Broadcast(CurrentPopulation);
    OnWorkerAssignmentChanged.Broadcast(AvailableWorkers);

    return true;
}

bool UBaseComponent::RemovePopulation(int32 Count)
{
    if (Count <= 0 || Count > CurrentPopulation)
    {
        return false;
    }

    // 利用可能ワーカーから優先的に減らす
    int32 WorkerReduction = FMath::Min(Count, AvailableWorkers);
    AvailableWorkers -= WorkerReduction;
    CurrentPopulation -= Count;

    // 必要なら割り当てワーカーも調整
    if (WorkerReduction < Count)
    {
        // TODO: 施設からワーカーを自動解除する処理
    }

    OnPopulationChanged.Broadcast(CurrentPopulation);
    OnWorkerAssignmentChanged.Broadcast(AvailableWorkers);

    return true;
}

void UBaseComponent::UpdateMaxPopulation()
{
    if (!FacilityManager)
    {
        return;
    }

    // 基本人口上限は0（住居がない状態）
    int32 BaseMax = 0;

    // 住居施設による追加
    float HousingBonus = FacilityManager->GetTotalEffectValue(EFacilityEffectType::PopulationCap);

    MaxPopulation = BaseMax + FMath::RoundToInt(HousingBonus);

    UE_LOG(LogTemp, Log, TEXT("BaseComponent: Max population updated to %d"), MaxPopulation);
}

float UBaseComponent::GetTotalBaseEffect(EFacilityEffectType EffectType) const
{
    if (!FacilityManager)
    {
        return 0.0f;
    }

    return FacilityManager->GetTotalEffectValue(EffectType);
}

TArray<FString> UBaseComponent::GetAllUnlockedRecipes() const
{
    if (!FacilityManager)
    {
        return TArray<FString>();
    }

    return FacilityManager->GetUnlockedRecipes();
}

TArray<FString> UBaseComponent::GetAllUnlockedItems() const
{
    if (!FacilityManager)
    {
        return TArray<FString>();
    }

    return FacilityManager->GetUnlockedItems();
}

int32 UBaseComponent::GetTotalStorageCapacity() const
{
    // 基本容量
    int32 BaseCapacity = 1000;

    // 倉庫による追加
    float StorageBonus = GetTotalBaseEffect(EFacilityEffectType::StorageCapacity);

    return BaseCapacity + FMath::RoundToInt(StorageBonus);
}

float UBaseComponent::GetProductionSpeedMultiplier() const
{
    float BaseSpeed = 1.0f;
    float Bonus = GetTotalBaseEffect(EFacilityEffectType::ProductionSpeed);
    return BaseSpeed + (Bonus / 100.0f);  // ボーナスはパーセンテージとして扱う
}

float UBaseComponent::GetResearchSpeedMultiplier() const
{
    float BaseSpeed = 1.0f;
    float Bonus = GetTotalBaseEffect(EFacilityEffectType::ResearchSpeed);
    return BaseSpeed + (Bonus / 100.0f);
}

float UBaseComponent::GetMoraleBonus() const
{
    return GetTotalBaseEffect(EFacilityEffectType::MoraleBonus);
}

TArray<FFacilityInstance> UBaseComponent::GetAllBaseFacilities() const
{
    if (!FacilityManager)
    {
        return TArray<FFacilityInstance>();
    }

    return FacilityManager->GetAllFacilities();
}

TArray<FFacilityInstance> UBaseComponent::GetFacilitiesByType(EFacilityType Type) const
{
    if (!FacilityManager)
    {
        return TArray<FFacilityInstance>();
    }

    return FacilityManager->GetFacilitiesByType(Type);
}

TArray<FFacilityInstance> UBaseComponent::GetFacilitiesByState(EFacilityState State) const
{
    if (!FacilityManager)
    {
        return TArray<FFacilityInstance>();
    }

    return FacilityManager->GetFacilitiesByState(State);
}

void UBaseComponent::ProcessProduction(float DeltaTime)
{
    ProcessAutoProduction(DeltaTime);
    ProcessResourceConversion(DeltaTime);
}

void UBaseComponent::ProcessMaintenance(float DeltaTime)
{
    if (!FacilityManager)
    {
        return;
    }

    // 施設の耐久値処理
    FacilityManager->ProcessMaintenance(DeltaTime);

    // メンテナンスコストの徴収
    TArray<FFacilityInstance> ActiveFacilities = GetFacilitiesByState(EFacilityState::Active);
    for (const FFacilityInstance& Facility : ActiveFacilities)
    {
        TMap<FString, int32> MaintenanceCost = FacilityManager->GetMaintenanceCost(Facility.FacilityId, Facility.Level);
        
        // コストが払えない場合は施設を停止
        if (!ConsumeResourcesForCost(MaintenanceCost))
        {
            FacilityManager->SetFacilityState(Facility.InstanceId, EFacilityState::Damaged);
            UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Facility %s stopped due to lack of maintenance resources"), 
                *Facility.FacilityId);
        }
    }
}

FString UBaseComponent::SerializeBaseData() const
{
    // TODO: JSON形式でデータをシリアライズ
    return TEXT("");
}

bool UBaseComponent::DeserializeBaseData(const FString& SerializedData)
{
    // TODO: JSONからデータを復元
    return false;
}

bool UBaseComponent::ConsumeResourcesForCost(const TMap<FString, int32>& Costs)
{
    // まず全てのコストが払えるか確認
    for (const auto& CostPair : Costs)
    {
        if (!HasResource(CostPair.Key, CostPair.Value))
        {
            return false;
        }
    }

    // 実際に消費
    for (const auto& CostPair : Costs)
    {
        ConsumeResource(CostPair.Key, CostPair.Value);
    }

    return true;
}

void UBaseComponent::RefundResources(const TMap<FString, int32>& Resources)
{
    for (const auto& ResourcePair : Resources)
    {
        AddResource(ResourcePair.Key, ResourcePair.Value);
    }
}

void UBaseComponent::ApplyFacilityEffects()
{
    // 各種効果の適用（必要に応じて実装）
}

void UBaseComponent::ProcessAutoProduction(float DeltaTime)
{
    if (!FacilityManager)
    {
        return;
    }

    // 自動生産施設の処理
    TArray<FFacilityInstance> ProductionFacilities = GetFacilitiesByType(EFacilityType::Production);
    
    for (const FFacilityInstance& Facility : ProductionFacilities)
    {
        if (Facility.State != EFacilityState::Active)
        {
            continue;
        }

        TArray<FFacilityEffect> Effects = FacilityManager->GetActiveEffects(Facility.InstanceId);
        
        for (const FFacilityEffect& Effect : Effects)
        {
            if (Effect.EffectType == EFacilityEffectType::AutoProduction)
            {
                // 生産速度を考慮した生産量
                float ProductionRate = FacilityManager->GetEffectValue(Facility.InstanceId, EFacilityEffectType::AutoProduction);
                ProductionRate *= GetProductionSpeedMultiplier();
                
                int32 ProducedAmount = FMath::RoundToInt(ProductionRate * DeltaTime);
                if (ProducedAmount > 0)
                {
                    AddResource(Effect.TargetId, ProducedAmount);
                }
            }
        }
    }
}

void UBaseComponent::ProcessResourceConversion(float DeltaTime)
{
    if (!FacilityManager)
    {
        return;
    }

    // 資源変換施設の処理
    TArray<FFacilityInstance> UtilityFacilities = GetFacilitiesByType(EFacilityType::Utility);
    
    for (const FFacilityInstance& Facility : UtilityFacilities)
    {
        if (Facility.State != EFacilityState::Active)
        {
            continue;
        }

        TArray<FFacilityEffect> Effects = FacilityManager->GetActiveEffects(Facility.InstanceId);
        
        for (const FFacilityEffect& Effect : Effects)
        {
            if (Effect.EffectType == EFacilityEffectType::ResourceConversion)
            {
                // TODO: 資源変換ロジックの実装
                // 入力資源と出力資源の定義が必要
            }
        }
    }
}