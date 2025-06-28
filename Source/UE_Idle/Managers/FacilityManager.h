#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "UE_Idle/Types/FacilityDataTable.h"
#include "FacilityManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFacilityStateChanged, const FGuid&, InstanceId, EFacilityState, OldState, EFacilityState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFacilityLevelChanged, const FGuid&, InstanceId, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConstructionProgressChanged, const FGuid&, InstanceId, float, Progress);

UCLASS(BlueprintType)
class UE_IDLE_API UFacilityManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // DataTable設定
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    void SetFacilityDataTable(UDataTable* InDataTable);

    // 基本データアクセス
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool GetFacilityData(const FString& FacilityId, FFacilityDataRow& OutFacilityData) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    FFacilityDataRow GetFacilityDataByRowName(const FName& RowName) const;

    // デバッグ・開発用
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    void ClearAllFacilities();

    // 施設の作成・管理
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    FGuid CreateFacility(const FString& FacilityId, const FVector& Location);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool DestroyFacility(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    FFacilityInstance GetFacilityInstance(const FGuid& InstanceId);

    // 内部コンポーネント用：ポインターアクセス（Blueprint非公開）
    FFacilityInstance* GetFacilityInstanceForComponent(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FFacilityInstance> GetAllFacilities() const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FFacilityInstance> GetFacilitiesByType(EFacilityType Type) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FFacilityInstance> GetFacilitiesByState(EFacilityState State) const;

    // 建設・アップグレード
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool StartConstruction(const FGuid& InstanceId, const TMap<FString, int32>& AvailableResources);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool StartUpgrade(const FGuid& InstanceId, const TMap<FString, int32>& AvailableResources);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    void UpdateConstructionProgress(const FGuid& InstanceId, float DeltaTime, int32 WorkerCount);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool CompleteConstruction(const FGuid& InstanceId);

    // コスト計算
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TMap<FString, int32> GetConstructionCost(const FString& FacilityId) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TMap<FString, int32> GetUpgradeCost(const FString& FacilityId, int32 CurrentLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TMap<FString, int32> GetMaintenanceCost(const FString& FacilityId, int32 Level) const;

    // 依存関係チェック
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool CheckDependencies(const FString& FacilityId) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FFacilityDependency> GetUnmetDependencies(const FString& FacilityId) const;

    // 効果計算
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FFacilityEffect> GetActiveEffects(const FGuid& InstanceId) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    float GetEffectValue(const FGuid& InstanceId, EFacilityEffectType EffectType) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    float GetTotalEffectValue(EFacilityEffectType EffectType) const;

    // アンロック確認
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FString> GetUnlockedRecipes() const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FString> GetUnlockedItems() const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    TArray<FString> GetUnlockedFacilities() const;

    // 状態管理
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool SetFacilityState(const FGuid& InstanceId, EFacilityState NewState);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool DamageFacility(const FGuid& InstanceId, int32 Damage);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool RepairFacility(const FGuid& InstanceId, int32 RepairAmount);

    // ワーカー管理
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool AssignWorkers(const FGuid& InstanceId, int32 WorkerCount);

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    int32 GetTotalAssignedWorkers() const;

    // ユーティリティ
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool CanBuildFacility(const FString& FacilityId, const TMap<FString, int32>& AvailableResources) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    bool CanUpgradeFacility(const FGuid& InstanceId, const TMap<FString, int32>& AvailableResources) const;

    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    float GetConstructionTimeRemaining(const FGuid& InstanceId) const;

    // 定期処理
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    void ProcessMaintenance(float DeltaTime);

    // Debug/Test function for BaseComponent
    UFUNCTION(BlueprintCallable, Category = "Facility Manager")
    void AddTestFacilityInstance(const FFacilityInstance& Instance);

    // イベント
    UPROPERTY(BlueprintAssignable, Category = "Facility Manager")
    FOnFacilityStateChanged OnFacilityStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Facility Manager")
    FOnFacilityLevelChanged OnFacilityLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "Facility Manager")
    FOnConstructionProgressChanged OnConstructionProgressChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    UDataTable* FacilityDataTable;

    UPROPERTY()
    TMap<FGuid, FFacilityInstance> FacilityInstances;

    // メンテナンス設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MaintenanceInterval = 3600.0f;  // 1時間

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float DurabilityLossPerHour = 10.0f;

private:
    const FFacilityDataRow* FindFacilityByFacilityId(const FString& FacilityId) const;
    TMap<FString, int32> CalculateCostsAtLevel(const TArray<FFacilityResourceCost>& BaseCosts, int32 Level) const;
    void CheckAndApplyStateTransitions(FFacilityInstance& Instance);
    
    // 内部使用：ポインターを返すヘルパー関数
    FFacilityInstance* GetFacilityInstancePtr(const FGuid& InstanceId);
};