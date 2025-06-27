#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/ItemDataTable.h"
#include "../Types/CharacterTypes.h"
#include "../Types/ItemTypes.h"
#include "InventoryComponent.generated.h"

class UItemDataTableManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemChanged, const FString&, ItemId, int32, NewQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemEquipped, const FString&, ItemId, EEquipmentSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemUnequipped, const FString&, ItemId, EEquipmentSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryMoneyChanged, int32, NewAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryResourceChanged, EResourceType, ResourceType, int32, NewAmount);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

protected:
    virtual void BeginPlay() override;

    // Core inventory data
    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    FInventory Inventory;

    // Manager reference
    UPROPERTY()
    UItemDataTableManager* ItemManager;

    // Optional features (use as needed)
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    FEquipmentSlots Equipment;

    UPROPERTY(BlueprintReadOnly, Category = "Money")
    int32 Money = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    TMap<EResourceType, int32> Resources;

public:
    // Owner identification
    UPROPERTY(BlueprintReadWrite, Category = "Identity")
    FString OwnerId;

    // ========== Core Inventory Operations ==========
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool HasItem(const FString& ItemId, int32 Quantity = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 GetItemCount(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TransferTo(UInventoryComponent* TargetInventory, const FString& ItemId, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<FInventorySlot> GetAllSlots() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TMap<FString, int32> GetAllItems() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    float GetTotalWeight() const;

    // ========== Carrying Capacity Functions ==========

    // 最大積載量取得（所有者に応じて自動計算）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Carrying Capacity")
    float GetMaxCarryingCapacity() const;

    // 現在の積載率取得（0.0-1.0）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Carrying Capacity")
    float GetLoadRatio() const;

    // 積載量オーバーかどうか
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Carrying Capacity")
    bool IsOverweight() const;

    // アイテム追加可能かどうか（重量チェック）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Carrying Capacity")
    bool CanAddItemByWeight(const FString& ItemId, int32 Quantity = 1) const;

    // ========== Equipment Functions (for Characters) ==========
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipItem(const FString& ItemId);
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipWeapon(const FString& WeaponId);
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipShield(const FString& ShieldId);
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipArmor(const FString& ArmorId);
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UnequipWeapon();
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UnequipShield();
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UnequipArmor(EEquipmentSlot Slot);
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool CanEquipItem(const FString& ItemId) const;
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    float GetTotalEquipmentWeight() const;
    
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    int32 GetTotalDefense() const;

    // ========== Money Functions (for Storage) ==========
    
    UFUNCTION(BlueprintCallable, Category = "Money")
    bool AddMoney(int32 Amount);
    
    UFUNCTION(BlueprintCallable, Category = "Money")
    bool SpendMoney(int32 Amount);
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Money")
    int32 GetMoney() const { return Money; }
    
    UFUNCTION(BlueprintCallable, Category = "Trading")
    bool BuyItem(const FString& ItemId, int32 Quantity = 1);
    
    UFUNCTION(BlueprintCallable, Category = "Trading")
    bool SellItem(const FString& ItemId, int32 Quantity = 1);

    // ========== Resource Functions (for Teams/Locations) ==========
    
    UFUNCTION(BlueprintCallable, Category = "Resources")
    bool AddResource(EResourceType ResourceType, int32 Amount);
    
    UFUNCTION(BlueprintCallable, Category = "Resources")
    bool SpendResource(EResourceType ResourceType, int32 Amount);
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    int32 GetResource(EResourceType ResourceType) const;
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    TMap<EResourceType, int32> GetAllResources() const { return Resources; }

    // ========== Equipment Access Functions ==========
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
    FString GetEquippedWeaponId() const { return Equipment.Weapon.ItemId; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
    bool HasEquippedWeapon() const { return !Equipment.Weapon.IsEmpty(); }

    // ========== Events ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnInventoryItemChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "Equipment Events")
    FOnInventoryItemEquipped OnItemEquipped;

    UPROPERTY(BlueprintAssignable, Category = "Equipment Events")
    FOnInventoryItemUnequipped OnItemUnequipped;

    UPROPERTY(BlueprintAssignable, Category = "Money Events")
    FOnInventoryMoneyChanged OnMoneyChanged;

    UPROPERTY(BlueprintAssignable, Category = "Resource Events")
    FOnInventoryResourceChanged OnResourceChanged;

    // ========== Backward Compatibility ==========
    
    // For old CharacterInventoryComponent users
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<FInventorySlot> GetAllInventorySlots() const { return GetAllSlots(); }

    // For old GlobalInventoryComponent users  
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItemToStorage(const FString& ItemId, int32 Quantity = 1) { return AddItem(ItemId, Quantity); }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool HasItemInStorage(const FString& ItemId, int32 Quantity = 1) const { return HasItem(ItemId, Quantity); }

protected:
    // Helper methods
    bool EquipToSlot(const FString& ItemId, EEquipmentSlot Slot);
    bool UnequipFromSlot(EEquipmentSlot Slot);
};