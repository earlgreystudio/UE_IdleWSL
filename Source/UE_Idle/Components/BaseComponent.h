#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE_Idle/Types/FacilityDataTable.h"
#include "BaseComponent.generated.h"

// Event Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, const FString&, ResourceId, int32, NewAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPopulationChanged, int32, NewPopulation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkerAssignmentChanged, int32, AvailableWorkers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFacilityAdded, const FGuid&, InstanceId, const FString&, FacilityId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFacilityRemoved, const FGuid&, InstanceId);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UBaseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBaseComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Event Dispatchers
    UPROPERTY(BlueprintAssignable, Category = "Base Events")
    FOnResourceChanged OnResourceChanged;

    UPROPERTY(BlueprintAssignable, Category = "Base Events")
    FOnPopulationChanged OnPopulationChanged;

    UPROPERTY(BlueprintAssignable, Category = "Base Events")
    FOnWorkerAssignmentChanged OnWorkerAssignmentChanged;

    UPROPERTY(BlueprintAssignable, Category = "Base Events")
    FOnFacilityAdded OnFacilityAdded;

    UPROPERTY(BlueprintAssignable, Category = "Base Events")
    FOnFacilityRemoved OnFacilityRemoved;

    // Base Resources
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Base Resources")
    TMap<FString, int32> BaseResources;

    // Population Management
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Population")
    int32 CurrentPopulation = 1;  // 初期人口

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Population")
    int32 MaxPopulation = 0;  // 住居がない状態

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Population")
    int32 AvailableWorkers = 1;

    // Resource Management
    UFUNCTION(BlueprintCallable, Category = "Base Resources")
    bool AddResource(const FString& ResourceId, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Base Resources")
    bool ConsumeResource(const FString& ResourceId, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Base Resources")
    bool HasResource(const FString& ResourceId, int32 RequiredAmount = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Base Resources")
    int32 GetResourceAmount(const FString& ResourceId) const;

    UFUNCTION(BlueprintCallable, Category = "Base Resources")
    TMap<FString, int32> GetAllResources() const { return BaseResources; }

    // Facility Construction
    UFUNCTION(BlueprintCallable, Category = "Facility Construction")
    FGuid PlanFacility(const FString& FacilityId, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Facility Construction")
    bool StartFacilityConstruction(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Facility Construction")
    bool UpgradeFacility(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Facility Construction")
    bool DemolishFacility(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Facility Construction")
    bool CanBuildFacility(const FString& FacilityId) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Construction")
    bool CanUpgradeFacility(const FGuid& InstanceId) const;

    // Worker Assignment
    UFUNCTION(BlueprintCallable, Category = "Worker Management")
    bool AssignWorkersToFacility(const FGuid& InstanceId, int32 WorkerCount);

    UFUNCTION(BlueprintCallable, Category = "Worker Management")
    int32 GetAssignedWorkers(const FGuid& InstanceId) const;

    UFUNCTION(BlueprintCallable, Category = "Worker Management")
    void RecalculateAvailableWorkers();

    // Population Management
    UFUNCTION(BlueprintCallable, Category = "Population")
    bool AddPopulation(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Population")
    bool RemovePopulation(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Population")
    void UpdateMaxPopulation();

    // Base Effects
    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    float GetTotalBaseEffect(EFacilityEffectType EffectType) const;

    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    TArray<FString> GetAllUnlockedRecipes() const;

    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    TArray<FString> GetAllUnlockedItems() const;

    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    int32 GetTotalStorageCapacity() const;

    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    float GetProductionSpeedMultiplier() const;

    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    float GetResearchSpeedMultiplier() const;

    UFUNCTION(BlueprintCallable, Category = "Base Effects")
    float GetMoraleBonus() const;

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Base Utility")
    TArray<FFacilityInstance> GetAllBaseFacilities() const;

    UFUNCTION(BlueprintCallable, Category = "Base Utility")
    TArray<FFacilityInstance> GetFacilitiesByType(EFacilityType Type) const;

    UFUNCTION(BlueprintCallable, Category = "Base Utility")
    TArray<FFacilityInstance> GetFacilitiesByState(EFacilityState State) const;

    UFUNCTION(BlueprintCallable, Category = "Base Utility")
    void ProcessProduction(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Base Utility")
    void ProcessMaintenance(float DeltaTime);

    // Save/Load Support
    UFUNCTION(BlueprintCallable, Category = "Save System")
    FString SerializeBaseData() const;

    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool DeserializeBaseData(const FString& SerializedData);

protected:
    // Helper functions
    bool ConsumeResourcesForCost(const TMap<FString, int32>& Costs);
    void RefundResources(const TMap<FString, int32>& Resources);
    void ApplyFacilityEffects();
    void ProcessAutoProduction(float DeltaTime);
    void ProcessResourceConversion(float DeltaTime);

private:
    // References to managers
    UPROPERTY()
    class UFacilityManager* FacilityManager;

    UPROPERTY()
    class UItemDataTableManager* ItemManager;

    UPROPERTY()
    class UGlobalInventoryComponent* GlobalInventory;

    // Timing
    float ProductionTimer = 0.0f;
    float MaintenanceTimer = 0.0f;

    // Constants
    const float PRODUCTION_INTERVAL = 5.0f;  // 5秒ごと
    const float MAINTENANCE_INTERVAL = 60.0f;  // 1分ごと
};