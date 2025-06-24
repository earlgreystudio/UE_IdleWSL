#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UE_Idle/Types/ItemTypes.h"
#include "ItemManager.generated.h"

UCLASS()
class UE_IDLE_API UItemManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    void LoadItemsFromJSON();

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool GetItemData(const FString& ItemId, FItemData& OutItemData);

    // Non-blueprint version that returns pointer
    FItemData* GetItemDataPtr(const FString& ItemId);

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool CanEquipToSlot(const FString& ItemId, EEquipmentSlot Slot) const;

    // Backward compatibility methods
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool GetWeaponData(const FString& ItemId, FWeaponData& OutWeaponData);

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool GetArmorData(const FString& ItemId, FArmorData& OutArmorData);

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool GetConsumableData(const FString& ItemId, FConsumableData& OutConsumableData);

    // Non-blueprint versions that return pointers
    FWeaponData* GetWeaponDataPtr(const FString& ItemId);
    FArmorData* GetArmorDataPtr(const FString& ItemId);
    FConsumableData* GetConsumableDataPtr(const FString& ItemId);
    
    // Legacy item data access
    FItemData* GetLegacyItemData(const FString& ItemId);

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    EItemType GetItemType(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    float GetItemWeight(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetItemStackSize(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    FText GetItemName(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsValidItem(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetRealWorldPrice(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetOtherWorldPrice(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FString> GetAllItemIds() const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FItemData> GetAllItems() const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FString> GetItemsByType(EItemType ItemType) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FString> GetItemsByQuality(EItemQuality Quality) const;

protected:
    void LoadUnifiedItemsFromJSON(const FString& FilePath);
    
    // Legacy loading methods for backward compatibility
    void LoadWeaponsFromJSON(const FString& FilePath);
    void LoadArmorsFromJSON(const FString& FilePath);
    void LoadConsumablesFromJSON(const FString& FilePath);

    UPROPERTY()
    TMap<FString, FItemData> UnifiedItemDatabase;

    // Legacy databases for backward compatibility
    UPROPERTY()
    TMap<FString, FWeaponData> WeaponDatabase;

    UPROPERTY()
    TMap<FString, FArmorData> ArmorDatabase;

    UPROPERTY()
    TMap<FString, FConsumableData> ConsumableDatabase;

    UPROPERTY()
    TMap<FString, FItemData> ItemDatabase;
};