#include "BaseComponent.h"
#include "UE_Idle/Managers/FacilityManager.h"
#include "UE_Idle/Managers/ItemDataTableManager.h"
#include "UE_Idle/Components/InventoryComponent.h"
#include "UE_Idle/C_GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UBaseComponent::UBaseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;  // 100msごと
}

void UBaseComponent::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("BaseComponent::BeginPlay - Auto initialization (no manual setup)"));
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

    // FacilityManager用のアイテム情報を取得（新Item-basedシステム）
    TMap<FString, int32> CurrentResources;
    if (GlobalInventory)
    {
        // 新採集システムでは全てアイテムとして管理
        TMap<FString, int32> AllItems = GlobalInventory->GetAllItems();
        CurrentResources = AllItems;
    }

    if (!FacilityManager->StartConstruction(InstanceId, CurrentResources))
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

    // FacilityManager用のアイテム情報を取得（新Item-basedシステム）
    TMap<FString, int32> CurrentResources;
    if (GlobalInventory)
    {
        // 新採集システムでは全てアイテムとして管理
        TMap<FString, int32> AllItems = GlobalInventory->GetAllItems();
        CurrentResources = AllItems;
    }

    if (!FacilityManager->StartUpgrade(InstanceId, CurrentResources))
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
    if (!FacilityManager || !GlobalInventory)
    {
        return false;
    }

    // PlayerControllerのInventoryからアイテム情報を取得（新Item-basedシステム）
    TMap<FString, int32> CurrentResources;
    TMap<FString, int32> AllItems = GlobalInventory->GetAllItems();
    CurrentResources = AllItems;

    return FacilityManager->CanBuildFacility(FacilityId, CurrentResources);
}

bool UBaseComponent::CanUpgradeFacility(const FGuid& InstanceId) const
{
    if (!FacilityManager || !GlobalInventory)
    {
        return false;
    }

    // PlayerControllerのInventoryからアイテム情報を取得（新Item-basedシステム）
    TMap<FString, int32> CurrentResources;
    TMap<FString, int32> AllItems = GlobalInventory->GetAllItems();
    CurrentResources = AllItems;

    return FacilityManager->CanUpgradeFacility(InstanceId, CurrentResources);
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
    if (!GlobalInventory)
    {
        return false;
    }

    // 新採集システム：全てアイテムとして管理
    // まず全てのコストが払えるか確認
    for (const auto& CostPair : Costs)
    {
        if (!GlobalInventory->HasItem(CostPair.Key, CostPair.Value))
        {
            UE_LOG(LogTemp, Warning, TEXT("ConsumeResourcesForCost: Insufficient %s (needed: %d, have: %d)"), 
                *CostPair.Key, CostPair.Value, GlobalInventory->GetItemCount(CostPair.Key));
            return false;
        }
    }

    // 実際に消費
    for (const auto& CostPair : Costs)
    {
        if (!GlobalInventory->RemoveItem(CostPair.Key, CostPair.Value))
        {
            UE_LOG(LogTemp, Error, TEXT("ConsumeResourcesForCost: Failed to remove %s x%d"), 
                *CostPair.Key, CostPair.Value);
            return false;
        }
    }

    return true;
}

void UBaseComponent::RefundResources(const TMap<FString, int32>& Resources)
{
    if (!GlobalInventory)
    {
        return;
    }

    // 新採集システム：全てアイテムとして追加
    for (const auto& ResourcePair : Resources)
    {
        if (!GlobalInventory->AddItem(ResourcePair.Key, ResourcePair.Value))
        {
            UE_LOG(LogTemp, Error, TEXT("RefundResources: Failed to add %s x%d"), 
                *ResourcePair.Key, ResourcePair.Value);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("RefundResources: Refunded %s x%d"), 
                *ResourcePair.Key, ResourcePair.Value);
        }
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
                if (ProducedAmount > 0 && GlobalInventory)
                {
                    // 新採集システム：全てアイテムとして追加
                    if (!GlobalInventory->AddItem(Effect.TargetId, ProducedAmount))
                    {
                        UE_LOG(LogTemp, Error, TEXT("ProcessAutoProduction: Failed to add %s x%d"), 
                            *Effect.TargetId, ProducedAmount);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Log, TEXT("ProcessAutoProduction: Produced %s x%d"), 
                            *Effect.TargetId, ProducedAmount);
                    }
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

void UBaseComponent::AddTestFacilities()
{
    if (!FacilityManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent::AddTestFacilities - FacilityManager is null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Adding test facilities for display"));

    // DataTableが設定されているかテスト
    FFacilityDataRow TestData;
    // GameInstanceのDataTable設定を強制確認
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UC_GameInstance* CustomGameInstance = Cast<UC_GameInstance>(GameInstance))
        {
            UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Found C_GameInstance, checking DataTable..."));
            // カスタムGameInstanceの場合、DataTableを直接設定を試行
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("BaseComponent::AddTestFacilities - Using default GameInstance instead of C_GameInstance!"));
        }
    }
    
    if (!FacilityManager->GetFacilityData(TEXT("small_house"), TestData))
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent::AddTestFacilities - FacilityDataTable not properly set! Cannot find 'small_house' in DataTable."));
        UE_LOG(LogTemp, Error, TEXT("BaseComponent::AddTestFacilities - This usually means:"));
        UE_LOG(LogTemp, Error, TEXT("  1. FacilityData.csv was not imported as a DataTable asset in Unreal Editor"));
        UE_LOG(LogTemp, Error, TEXT("  2. The DataTable asset path in C_GameInstance.cpp is incorrect"));
        UE_LOG(LogTemp, Error, TEXT("  3. GameInstance initialization failed or hasn't completed yet"));
        UE_LOG(LogTemp, Error, TEXT("BaseComponent::AddTestFacilities - Cannot create facilities without DataTable. Please import FacilityData.csv as DataTable asset."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - DataTable found, creating real facilities..."));

    // 実在の設備を追加（それぞれ異なる状態・レベルで）
    
    // 1. 小屋 - 稼働中レベル1
    FGuid HouseInstance = FacilityManager->CreateFacility(TEXT("small_house"), FVector(100, 0, 0));
    if (HouseInstance.IsValid())
    {
        FacilityManager->SetFacilityState(HouseInstance, EFacilityState::Active);
        OnFacilityAdded.Broadcast(HouseInstance, TEXT("small_house"));
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Added small_house (Active, Level 1)"));
    }

    // 2. 作業台 - 建設中
    FGuid WorkshopInstance = FacilityManager->CreateFacility(TEXT("workshop"), FVector(200, 0, 0));
    if (WorkshopInstance.IsValid())
    {
        FacilityManager->SetFacilityState(WorkshopInstance, EFacilityState::Constructing);
        // 建設進捗を50%に設定
        if (FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(WorkshopInstance))
        {
            Instance->CompletionProgress = 50.0f;
        }
        OnFacilityAdded.Broadcast(WorkshopInstance, TEXT("workshop"));
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Added workshop (Constructing, 50%%)"));
    }

    // 3. 鍛冶場 - 稼働中レベル2
    FGuid ForgeInstance = FacilityManager->CreateFacility(TEXT("forge"), FVector(300, 0, 0));
    if (ForgeInstance.IsValid())
    {
        FacilityManager->SetFacilityState(ForgeInstance, EFacilityState::Active);
        if (FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(ForgeInstance))
        {
            Instance->Level = 2;
        }
        OnFacilityAdded.Broadcast(ForgeInstance, TEXT("forge"));
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Added forge (Active, Level 2)"));
    }

    // 4. 倉庫 - 損傷状態
    FGuid WarehouseInstance = FacilityManager->CreateFacility(TEXT("warehouse"), FVector(400, 0, 0));
    if (WarehouseInstance.IsValid())
    {
        FacilityManager->SetFacilityState(WarehouseInstance, EFacilityState::Damaged);
        if (FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(WarehouseInstance))
        {
            Instance->CurrentDurability = 30; // 低い耐久度
        }
        OnFacilityAdded.Broadcast(WarehouseInstance, TEXT("warehouse"));
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Added warehouse (Damaged)"));
    }

    // 5. 畑 - アップグレード中
    FGuid FieldInstance = FacilityManager->CreateFacility(TEXT("field"), FVector(500, 0, 0));
    if (FieldInstance.IsValid())
    {
        FacilityManager->SetFacilityState(FieldInstance, EFacilityState::Upgrading);
        if (FFacilityInstance* Instance = FacilityManager->GetFacilityInstanceForComponent(FieldInstance))
        {
            Instance->Level = 1;
            Instance->CompletionProgress = 75.0f; // アップグレード進捗75%
        }
        OnFacilityAdded.Broadcast(FieldInstance, TEXT("field"));
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Added field (Upgrading, 75%%)"));
    }

    UE_LOG(LogTemp, Warning, TEXT("BaseComponent::AddTestFacilities - Test facilities added successfully"));
}

void UBaseComponent::InitializeBaseComponent()
{
    UE_LOG(LogTemp, Warning, TEXT("BaseComponent::InitializeBaseComponent - Manual initialization called"));

    // Get references to managers
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::InitializeBaseComponent - GameInstance found: %s"), 
            *GameInstance->GetClass()->GetName());
        
        FacilityManager = GameInstance->GetSubsystem<UFacilityManager>();
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent::InitializeBaseComponent - FacilityManager: %s"), 
            FacilityManager ? TEXT("Found") : TEXT("NULL"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent::InitializeBaseComponent - GameInstance is NULL!"));
    }

    // Get GlobalInventoryComponent from PlayerController
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        GlobalInventory = PC->FindComponentByClass<UInventoryComponent>();
    }

    if (!FacilityManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent: FacilityManager not found!"));
    }
    else
    {
        // DataTableが設定されていない場合、手動で設定を試行
        UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Attempting manual DataTable setup..."));
        if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            if (UC_GameInstance* CGameInstance = Cast<UC_GameInstance>(GameInstance))
            {
                UE_LOG(LogTemp, Warning, TEXT("BaseComponent: Found C_GameInstance, calling InitializeDataTables manually"));
                CGameInstance->InitializeDataTables();
            }
        }
    }
    if (!ItemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent: ItemDataTableManager not found!"));
    }

    // 初期設定：人口1人、上限0（住居がない状態）
    CurrentPopulation = 1;
    MaxPopulation = 0;
    AvailableWorkers = 1;

    // 既存の設備データをクリアしてから新しいテスト設備を追加
    if (FacilityManager)
    {
        FacilityManager->ClearAllFacilities();
    }
    
    // 表示テスト用のデフォルト設備を追加
    AddTestFacilities();
}

void UBaseComponent::ForceSetupTestFacilities()
{
    UE_LOG(LogTemp, Warning, TEXT("BaseComponent::ForceSetupTestFacilities - Force setup called"));
    
    // 管理者への参照がない場合は取得
    if (!FacilityManager)
    {
        if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            UE_LOG(LogTemp, Error, TEXT("DEBUG: GameInstance class is: %s"), *GameInstance->GetClass()->GetName());
            FacilityManager = GameInstance->GetSubsystem<UFacilityManager>();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("DEBUG: GameInstance is NULL!"));
        }
    }
    
    if (!FacilityManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BaseComponent::ForceSetupTestFacilities - FacilityManager still not found!"));
        return;
    }
    
    // データをクリアして新しい設備を追加
    FacilityManager->ClearAllFacilities();
    AddTestFacilities();
}

