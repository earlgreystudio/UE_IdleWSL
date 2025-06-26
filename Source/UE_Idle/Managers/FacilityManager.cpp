#include "FacilityManager.h"
#include "Engine/DataTable.h"

void UFacilityManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("FacilityManager initialized"));
}

void UFacilityManager::Deinitialize()
{
    FacilityInstances.Empty();
    Super::Deinitialize();
}

void UFacilityManager::SetFacilityDataTable(UDataTable* InDataTable)
{
    if (InDataTable && InDataTable->GetRowStruct()->IsChildOf(FFacilityDataRow::StaticStruct()))
    {
        FacilityDataTable = InDataTable;
        UE_LOG(LogTemp, Log, TEXT("FacilityManager: DataTable set successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FacilityManager: Invalid DataTable"));
    }
}

bool UFacilityManager::GetFacilityData(const FString& FacilityId, FFacilityDataRow& OutFacilityData) const
{
    if (!FacilityDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("FacilityManager: DataTable not set"));
        return false;
    }

    FName RowName(*FacilityId);
    FFacilityDataRow* Row = FacilityDataTable->FindRow<FFacilityDataRow>(RowName, TEXT("GetFacilityData"));
    
    if (Row)
    {
        OutFacilityData = *Row;
        return true;
    }

    return false;
}

FFacilityDataRow UFacilityManager::GetFacilityDataByRowName(const FName& RowName) const
{
    if (!FacilityDataTable)
    {
        return FFacilityDataRow();
    }

    FFacilityDataRow* Row = FacilityDataTable->FindRow<FFacilityDataRow>(RowName, TEXT("GetFacilityDataByRowName"));
    return Row ? *Row : FFacilityDataRow();
}

FGuid UFacilityManager::CreateFacility(const FString& FacilityId, const FVector& Location)
{
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        UE_LOG(LogTemp, Warning, TEXT("FacilityManager: Invalid FacilityId: %s"), *FacilityId);
        return FGuid();
    }

    FFacilityInstance NewInstance;
    NewInstance.FacilityId = FacilityId;
    NewInstance.Location = Location;
    NewInstance.Level = 1;
    NewInstance.State = EFacilityState::Planning;
    NewInstance.CurrentDurability = FacilityData.GetMaxDurabilityAtLevel(1);
    NewInstance.CompletionProgress = 0.0f;

    FacilityInstances.Add(NewInstance.InstanceId, NewInstance);
    
    UE_LOG(LogTemp, Log, TEXT("FacilityManager: Created facility %s at location %s"), 
        *FacilityId, *Location.ToString());

    return NewInstance.InstanceId;
}

bool UFacilityManager::DestroyFacility(const FGuid& InstanceId)
{
    if (FacilityInstances.Remove(InstanceId) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("FacilityManager: Destroyed facility %s"), *InstanceId.ToString());
        return true;
    }
    return false;
}

FFacilityInstance UFacilityManager::GetFacilityInstance(const FGuid& InstanceId)
{
    if (FFacilityInstance* Instance = FacilityInstances.Find(InstanceId))
    {
        return *Instance;
    }
    return FFacilityInstance(); // デフォルト値を返す
}

FFacilityInstance* UFacilityManager::GetFacilityInstancePtr(const FGuid& InstanceId)
{
    return FacilityInstances.Find(InstanceId);
}

FFacilityInstance* UFacilityManager::GetFacilityInstanceForComponent(const FGuid& InstanceId)
{
    return FacilityInstances.Find(InstanceId);
}

TArray<FFacilityInstance> UFacilityManager::GetAllFacilities() const
{
    TArray<FFacilityInstance> Result;
    for (const auto& Pair : FacilityInstances)
    {
        Result.Add(Pair.Value);
    }
    return Result;
}

TArray<FFacilityInstance> UFacilityManager::GetFacilitiesByType(EFacilityType Type) const
{
    TArray<FFacilityInstance> Result;
    
    for (const auto& Pair : FacilityInstances)
    {
        FFacilityDataRow FacilityData;
        if (GetFacilityData(Pair.Value.FacilityId, FacilityData))
        {
            if (FacilityData.FacilityType == Type)
            {
                Result.Add(Pair.Value);
            }
        }
    }
    
    return Result;
}

TArray<FFacilityInstance> UFacilityManager::GetFacilitiesByState(EFacilityState State) const
{
    TArray<FFacilityInstance> Result;
    
    for (const auto& Pair : FacilityInstances)
    {
        if (Pair.Value.State == State)
        {
            Result.Add(Pair.Value);
        }
    }
    
    return Result;
}

bool UFacilityManager::StartConstruction(const FGuid& InstanceId, const TMap<FString, int32>& AvailableResources)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance || Instance->State != EFacilityState::Planning)
    {
        return false;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return false;
    }

    // コストチェック
    TMap<FString, int32> RequiredCosts = GetConstructionCost(Instance->FacilityId);
    for (const auto& CostPair : RequiredCosts)
    {
        const int32* Available = AvailableResources.Find(CostPair.Key);
        if (!Available || *Available < CostPair.Value)
        {
            UE_LOG(LogTemp, Warning, TEXT("FacilityManager: Insufficient resources for construction"));
            return false;
        }
    }

    // 依存関係チェック
    if (!CheckDependencies(Instance->FacilityId))
    {
        UE_LOG(LogTemp, Warning, TEXT("FacilityManager: Dependencies not met"));
        return false;
    }

    Instance->State = EFacilityState::Constructing;
    Instance->CompletionProgress = 0.0f;
    
    OnFacilityStateChanged.Broadcast(InstanceId, EFacilityState::Planning, EFacilityState::Constructing);
    
    return true;
}

bool UFacilityManager::StartUpgrade(const FGuid& InstanceId, const TMap<FString, int32>& AvailableResources)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance || Instance->State != EFacilityState::Active)
    {
        return false;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return false;
    }

    if (Instance->Level >= FacilityData.MaxLevel)
    {
        UE_LOG(LogTemp, Warning, TEXT("FacilityManager: Facility already at max level"));
        return false;
    }

    // コストチェック
    TMap<FString, int32> RequiredCosts = GetUpgradeCost(Instance->FacilityId, Instance->Level);
    for (const auto& CostPair : RequiredCosts)
    {
        const int32* Available = AvailableResources.Find(CostPair.Key);
        if (!Available || *Available < CostPair.Value)
        {
            UE_LOG(LogTemp, Warning, TEXT("FacilityManager: Insufficient resources for upgrade"));
            return false;
        }
    }

    Instance->State = EFacilityState::Upgrading;
    Instance->CompletionProgress = 0.0f;
    
    OnFacilityStateChanged.Broadcast(InstanceId, EFacilityState::Active, EFacilityState::Upgrading);
    
    return true;
}

void UFacilityManager::UpdateConstructionProgress(const FGuid& InstanceId, float DeltaTime, int32 WorkerCount)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance || (Instance->State != EFacilityState::Constructing && Instance->State != EFacilityState::Upgrading))
    {
        return;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return;
    }

    float RequiredTime = FacilityData.GetConstructionTimeAtLevel(Instance->Level);
    if (Instance->State == EFacilityState::Upgrading)
    {
        RequiredTime = FacilityData.GetConstructionTimeAtLevel(Instance->Level + 1);
    }

    // ワーカーによる加速
    float WorkerBonus = 1.0f + (WorkerCount - 1) * 0.2f;  // 追加ワーカーごとに20%加速
    float ProgressPerSecond = (100.0f / RequiredTime) * WorkerBonus;

    Instance->CompletionProgress += ProgressPerSecond * DeltaTime;
    Instance->CompletionProgress = FMath::Clamp(Instance->CompletionProgress, 0.0f, 100.0f);

    OnConstructionProgressChanged.Broadcast(InstanceId, Instance->CompletionProgress);

    // 完成チェック
    if (Instance->CompletionProgress >= 100.0f)
    {
        CompleteConstruction(InstanceId);
    }
}

bool UFacilityManager::CompleteConstruction(const FGuid& InstanceId)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance)
    {
        return false;
    }

    if (Instance->State == EFacilityState::Constructing)
    {
        Instance->State = EFacilityState::Active;
        Instance->CompletionProgress = 100.0f;
        OnFacilityStateChanged.Broadcast(InstanceId, EFacilityState::Constructing, EFacilityState::Active);
        
        UE_LOG(LogTemp, Log, TEXT("FacilityManager: Construction completed for %s"), *Instance->FacilityId);
    }
    else if (Instance->State == EFacilityState::Upgrading)
    {
        Instance->Level++;
        Instance->State = EFacilityState::Active;
        Instance->CompletionProgress = 100.0f;
        
        // 最大耐久値を更新
        FFacilityDataRow FacilityData;
        if (GetFacilityData(Instance->FacilityId, FacilityData))
        {
            Instance->CurrentDurability = FacilityData.GetMaxDurabilityAtLevel(Instance->Level);
        }
        
        OnFacilityStateChanged.Broadcast(InstanceId, EFacilityState::Upgrading, EFacilityState::Active);
        OnFacilityLevelChanged.Broadcast(InstanceId, Instance->Level);
        
        UE_LOG(LogTemp, Log, TEXT("FacilityManager: Upgrade completed for %s to level %d"), 
            *Instance->FacilityId, Instance->Level);
    }
    else
    {
        return false;
    }

    return true;
}

TMap<FString, int32> UFacilityManager::GetConstructionCost(const FString& FacilityId) const
{
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        return TMap<FString, int32>();
    }

    return CalculateCostsAtLevel(FacilityData.ConstructionCosts, 1);
}

TMap<FString, int32> UFacilityManager::GetUpgradeCost(const FString& FacilityId, int32 CurrentLevel) const
{
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        return TMap<FString, int32>();
    }

    return CalculateCostsAtLevel(FacilityData.UpgradeCosts, CurrentLevel + 1);
}

TMap<FString, int32> UFacilityManager::GetMaintenanceCost(const FString& FacilityId, int32 Level) const
{
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        return TMap<FString, int32>();
    }

    return CalculateCostsAtLevel(FacilityData.MaintenanceCosts, Level);
}

bool UFacilityManager::CheckDependencies(const FString& FacilityId) const
{
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        return false;
    }

    for (const FFacilityDependency& Dep : FacilityData.Dependencies)
    {
        bool bFound = false;
        
        for (const auto& Pair : FacilityInstances)
        {
            if (Pair.Value.FacilityId == Dep.RequiredFacilityId && 
                Pair.Value.Level >= Dep.RequiredLevel &&
                Pair.Value.State == EFacilityState::Active)
            {
                bFound = true;
                break;
            }
        }
        
        if (!bFound)
        {
            return false;
        }
    }

    return true;
}

TArray<FFacilityDependency> UFacilityManager::GetUnmetDependencies(const FString& FacilityId) const
{
    TArray<FFacilityDependency> UnmetDeps;
    
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        return UnmetDeps;
    }

    for (const FFacilityDependency& Dep : FacilityData.Dependencies)
    {
        bool bMet = false;
        
        for (const auto& Pair : FacilityInstances)
        {
            if (Pair.Value.FacilityId == Dep.RequiredFacilityId && 
                Pair.Value.Level >= Dep.RequiredLevel &&
                Pair.Value.State == EFacilityState::Active)
            {
                bMet = true;
                break;
            }
        }
        
        if (!bMet)
        {
            UnmetDeps.Add(Dep);
        }
    }

    return UnmetDeps;
}

TArray<FFacilityEffect> UFacilityManager::GetActiveEffects(const FGuid& InstanceId) const
{
    TArray<FFacilityEffect> ActiveEffects;
    
    const FFacilityInstance* Instance = FacilityInstances.Find(InstanceId);
    if (!Instance || Instance->State != EFacilityState::Active)
    {
        return ActiveEffects;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return ActiveEffects;
    }

    for (const FFacilityEffect& Effect : FacilityData.Effects)
    {
        if (Instance->Level >= Effect.RequiredLevel)
        {
            ActiveEffects.Add(Effect);
        }
    }

    return ActiveEffects;
}

float UFacilityManager::GetEffectValue(const FGuid& InstanceId, EFacilityEffectType EffectType) const
{
    const FFacilityInstance* Instance = FacilityInstances.Find(InstanceId);
    if (!Instance || Instance->State != EFacilityState::Active)
    {
        return 0.0f;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return 0.0f;
    }

    float TotalValue = 0.0f;
    for (const FFacilityEffect& Effect : FacilityData.Effects)
    {
        if (Effect.EffectType == EffectType && Instance->Level >= Effect.RequiredLevel)
        {
            TotalValue += FacilityData.GetEffectValueAtLevel(Effect, Instance->Level);
        }
    }

    return TotalValue;
}

float UFacilityManager::GetTotalEffectValue(EFacilityEffectType EffectType) const
{
    float TotalValue = 0.0f;
    
    for (const auto& Pair : FacilityInstances)
    {
        TotalValue += GetEffectValue(Pair.Key, EffectType);
    }
    
    return TotalValue;
}

TArray<FString> UFacilityManager::GetUnlockedRecipes() const
{
    TArray<FString> UnlockedRecipes;
    
    for (const auto& Pair : FacilityInstances)
    {
        if (Pair.Value.State != EFacilityState::Active)
        {
            continue;
        }

        FFacilityDataRow FacilityData;
        if (!GetFacilityData(Pair.Value.FacilityId, FacilityData))
        {
            continue;
        }

        for (const FFacilityEffect& Effect : FacilityData.Effects)
        {
            if (Effect.EffectType == EFacilityEffectType::UnlockRecipe && 
                Pair.Value.Level >= Effect.RequiredLevel)
            {
                UnlockedRecipes.AddUnique(Effect.TargetId);
            }
        }
    }
    
    return UnlockedRecipes;
}

TArray<FString> UFacilityManager::GetUnlockedItems() const
{
    TArray<FString> UnlockedItems;
    
    for (const auto& Pair : FacilityInstances)
    {
        if (Pair.Value.State != EFacilityState::Active)
        {
            continue;
        }

        FFacilityDataRow FacilityData;
        if (!GetFacilityData(Pair.Value.FacilityId, FacilityData))
        {
            continue;
        }

        for (const FFacilityEffect& Effect : FacilityData.Effects)
        {
            if (Effect.EffectType == EFacilityEffectType::UnlockItem && 
                Pair.Value.Level >= Effect.RequiredLevel)
            {
                UnlockedItems.AddUnique(Effect.TargetId);
            }
        }
    }
    
    return UnlockedItems;
}

TArray<FString> UFacilityManager::GetUnlockedFacilities() const
{
    TArray<FString> UnlockedFacilities;
    
    for (const auto& Pair : FacilityInstances)
    {
        if (Pair.Value.State != EFacilityState::Active)
        {
            continue;
        }

        FFacilityDataRow FacilityData;
        if (!GetFacilityData(Pair.Value.FacilityId, FacilityData))
        {
            continue;
        }

        for (const FFacilityEffect& Effect : FacilityData.Effects)
        {
            if (Effect.EffectType == EFacilityEffectType::UnlockFacility && 
                Pair.Value.Level >= Effect.RequiredLevel)
            {
                UnlockedFacilities.AddUnique(Effect.TargetId);
            }
        }
    }
    
    return UnlockedFacilities;
}

bool UFacilityManager::SetFacilityState(const FGuid& InstanceId, EFacilityState NewState)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance)
    {
        return false;
    }

    EFacilityState OldState = Instance->State;
    Instance->State = NewState;
    
    OnFacilityStateChanged.Broadcast(InstanceId, OldState, NewState);
    
    return true;
}

bool UFacilityManager::DamageFacility(const FGuid& InstanceId, int32 Damage)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance)
    {
        return false;
    }

    Instance->CurrentDurability = FMath::Max(0, Instance->CurrentDurability - Damage);
    
    CheckAndApplyStateTransitions(*Instance);
    
    return true;
}

bool UFacilityManager::RepairFacility(const FGuid& InstanceId, int32 RepairAmount)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance)
    {
        return false;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return false;
    }

    int32 MaxDurability = FacilityData.GetMaxDurabilityAtLevel(Instance->Level);
    Instance->CurrentDurability = FMath::Min(MaxDurability, Instance->CurrentDurability + RepairAmount);
    
    CheckAndApplyStateTransitions(*Instance);
    
    return true;
}

bool UFacilityManager::AssignWorkers(const FGuid& InstanceId, int32 WorkerCount)
{
    FFacilityInstance* Instance = GetFacilityInstancePtr(InstanceId);
    if (!Instance)
    {
        return false;
    }

    Instance->AssignedWorkers = FMath::Max(0, WorkerCount);
    return true;
}

int32 UFacilityManager::GetTotalAssignedWorkers() const
{
    int32 Total = 0;
    
    for (const auto& Pair : FacilityInstances)
    {
        Total += Pair.Value.AssignedWorkers;
    }
    
    return Total;
}

bool UFacilityManager::CanBuildFacility(const FString& FacilityId, const TMap<FString, int32>& AvailableResources) const
{
    // データ存在チェック
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(FacilityId, FacilityData))
    {
        return false;
    }

    // コストチェック
    TMap<FString, int32> RequiredCosts = GetConstructionCost(FacilityId);
    for (const auto& CostPair : RequiredCosts)
    {
        const int32* Available = AvailableResources.Find(CostPair.Key);
        if (!Available || *Available < CostPair.Value)
        {
            return false;
        }
    }

    // 依存関係チェック
    return CheckDependencies(FacilityId);
}

bool UFacilityManager::CanUpgradeFacility(const FGuid& InstanceId, const TMap<FString, int32>& AvailableResources) const
{
    const FFacilityInstance* Instance = FacilityInstances.Find(InstanceId);
    if (!Instance || Instance->State != EFacilityState::Active)
    {
        return false;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return false;
    }

    // 最大レベルチェック
    if (Instance->Level >= FacilityData.MaxLevel)
    {
        return false;
    }

    // コストチェック
    TMap<FString, int32> RequiredCosts = GetUpgradeCost(Instance->FacilityId, Instance->Level);
    for (const auto& CostPair : RequiredCosts)
    {
        const int32* Available = AvailableResources.Find(CostPair.Key);
        if (!Available || *Available < CostPair.Value)
        {
            return false;
        }
    }

    return true;
}

float UFacilityManager::GetConstructionTimeRemaining(const FGuid& InstanceId) const
{
    const FFacilityInstance* Instance = FacilityInstances.Find(InstanceId);
    if (!Instance || (Instance->State != EFacilityState::Constructing && Instance->State != EFacilityState::Upgrading))
    {
        return 0.0f;
    }

    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance->FacilityId, FacilityData))
    {
        return 0.0f;
    }

    float TotalTime = FacilityData.GetConstructionTimeAtLevel(Instance->Level);
    if (Instance->State == EFacilityState::Upgrading)
    {
        TotalTime = FacilityData.GetConstructionTimeAtLevel(Instance->Level + 1);
    }

    float RemainingProgress = 100.0f - Instance->CompletionProgress;
    return (RemainingProgress / 100.0f) * TotalTime;
}

void UFacilityManager::ProcessMaintenance(float DeltaTime)
{
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    for (auto& Pair : FacilityInstances)
    {
        FFacilityInstance& Instance = Pair.Value;
        
        if (Instance.State != EFacilityState::Active)
        {
            continue;
        }

        // 耐久値減少
        float TimeSinceMaintenance = CurrentTime - Instance.LastMaintenanceTime;
        if (TimeSinceMaintenance >= MaintenanceInterval)
        {
            int32 DurabilityLoss = FMath::RoundToInt(DurabilityLossPerHour * (TimeSinceMaintenance / 3600.0f));
            DamageFacility(Instance.InstanceId, DurabilityLoss);
            Instance.LastMaintenanceTime = CurrentTime;
        }
    }
}

const FFacilityDataRow* UFacilityManager::FindFacilityByFacilityId(const FString& FacilityId) const
{
    if (!FacilityDataTable)
    {
        return nullptr;
    }

    return FacilityDataTable->FindRow<FFacilityDataRow>(FName(*FacilityId), TEXT("FindFacilityByFacilityId"));
}

TMap<FString, int32> UFacilityManager::CalculateCostsAtLevel(const TArray<FFacilityResourceCost>& BaseCosts, int32 Level) const
{
    TMap<FString, int32> CalculatedCosts;
    
    for (const FFacilityResourceCost& Cost : BaseCosts)
    {
        int32 Amount = FMath::RoundToInt(Cost.BaseAmount * FMath::Pow(Level, Cost.LevelExponent));
        CalculatedCosts.Add(Cost.ItemId, Amount);
    }
    
    return CalculatedCosts;
}

void UFacilityManager::CheckAndApplyStateTransitions(FFacilityInstance& Instance)
{
    FFacilityDataRow FacilityData;
    if (!GetFacilityData(Instance.FacilityId, FacilityData))
    {
        return;
    }

    int32 MaxDurability = FacilityData.GetMaxDurabilityAtLevel(Instance.Level);
    float DurabilityPercent = (float)Instance.CurrentDurability / (float)MaxDurability;

    EFacilityState OldState = Instance.State;
    EFacilityState NewState = OldState;

    if (Instance.CurrentDurability <= 0)
    {
        NewState = EFacilityState::Broken;
    }
    else if (DurabilityPercent <= 0.5f && Instance.State == EFacilityState::Active)
    {
        NewState = EFacilityState::Damaged;
    }
    else if (DurabilityPercent > 0.5f && Instance.State == EFacilityState::Damaged)
    {
        NewState = EFacilityState::Active;
    }

    if (NewState != OldState)
    {
        Instance.State = NewState;
        OnFacilityStateChanged.Broadcast(Instance.InstanceId, OldState, NewState);
    }
}