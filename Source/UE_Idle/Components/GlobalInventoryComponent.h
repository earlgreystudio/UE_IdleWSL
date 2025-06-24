#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE_Idle/Types/ItemTypes.h"
#include "GlobalInventoryComponent.generated.h"

// Event Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStorageChanged, const FString&, ItemId, int32, NewQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, NewAmount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UGlobalInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGlobalInventoryComponent();

protected:
    virtual void BeginPlay() override;

public:
    // Event Dispatchers
    UPROPERTY(BlueprintAssignable, Category = "Storage Events")
    FOnStorageChanged OnStorageChanged;

    UPROPERTY(BlueprintAssignable, Category = "Storage Events")
    FOnMoneyChanged OnMoneyChanged;
    // Global storage - shared across all characters
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Storage")
    TMap<FString, FInventorySlot> GlobalStorage;

    // Banking/trading functions
    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    bool AddItemToStorage(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    bool RemoveItemFromStorage(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    bool HasItemInStorage(const FString& ItemId, int32 RequiredQuantity = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    int32 GetItemQuantityInStorage(const FString& ItemId) const;

    // Transfer between global storage and character inventory
    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    bool TransferToCharacter(class UCharacterInventoryComponent* CharacterInventory, const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    bool TransferFromCharacter(class UCharacterInventoryComponent* CharacterInventory, const FString& ItemId, int32 Quantity = 1);

    // Bulk operations
    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    TArray<FString> GetAllStoredItemIds() const;

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    TMap<FString, int32> GetAllStoredItems() const;

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    TArray<FInventorySlot> GetAllStorageSlots() const;

    UFUNCTION(BlueprintCallable, Category = "Global Inventory")
    void ClearStorage();

    // Money/currency management
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Currency")
    int32 GlobalMoney = 0;

    UFUNCTION(BlueprintCallable, Category = "Currency")
    bool AddMoney(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Currency")
    bool SpendMoney(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Currency")
    int32 GetMoney() const { return GlobalMoney; }

    // Trading functions
    UFUNCTION(BlueprintCallable, Category = "Trading")
    bool BuyItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Trading")
    bool SellItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Trading")
    bool SellItemInstance(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Trading")
    int32 GetItemBuyPrice(const FString& ItemId, int32 Quantity = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Trading")
    int32 GetItemSellPrice(const FString& ItemId, int32 Quantity = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Trading")
    int32 GetItemInstanceSellPrice(const FGuid& InstanceId) const;

protected:
    // Helper functions
    FInventorySlot* GetOrCreateStorageSlot(const FString& ItemId);
    FItemInstance* FindItemInstance(const FGuid& InstanceId);
    const FItemInstance* FindItemInstance(const FGuid& InstanceId) const;
    bool RemoveItemInstance(FInventorySlot& Slot, int32 Quantity);

private:
    // Reference to ItemDataTableManager for item data
    UPROPERTY()
    class UItemDataTableManager* ItemManager;
};