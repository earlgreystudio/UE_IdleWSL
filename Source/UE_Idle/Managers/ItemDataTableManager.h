#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "UE_Idle/Types/ItemDataTable.h"
#include "ItemDataTableManager.generated.h"

UCLASS(BlueprintType)
class UE_IDLE_API UItemDataTableManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // DataTable設定
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    void SetItemDataTable(UDataTable* InDataTable);

    // Core item data access
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool GetItemData(const FString& ItemId, FItemDataRow& OutItemData) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    FItemDataRow GetItemDataByRowName(const FName& RowName) const;

    // Item queries
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsValidItem(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool CanEquipToSlot(const FString& ItemId, EEquipmentSlotTable Slot) const;

    // Blueprint-friendly data retrieval (returns arrays, not pointers)
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FItemDataRow> GetAllItems() const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FItemDataRow> GetItemsByType(EItemTypeTable ItemType) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FItemDataRow> GetItemsByQuality(EItemQualityTable Quality) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    TArray<FName> GetAllItemRowNames() const;

    // Equipment restrictions
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool ItemBlocksShield(const FString& ItemId) const;

    // Pricing system
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetItemValue(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetModifiedItemValue(const FString& ItemId) const;

    // Item property checks (Blueprint wrappers)
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsItemEquippable(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsItemWeapon(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsItemArmor(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsItemConsumable(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool ItemHasEffect(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    bool IsItemInstant(const FString& ItemId) const;

    // Quality-modified values (Blueprint wrappers)
    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetItemModifiedAttackPower(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetItemModifiedDefense(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Item Manager")
    int32 GetItemModifiedDurability(const FString& ItemId) const;

protected:
    // DataTable reference - set this in Blueprint or assign programmatically
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    UDataTable* ItemDataTable;

private:
    // Helper function to find item by ItemId field (not row name)
    const FItemDataRow* FindItemByItemId(const FString& ItemId) const;
};