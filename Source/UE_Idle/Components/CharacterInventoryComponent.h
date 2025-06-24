#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UE_Idle/Types/ItemTypes.h"
#include "CharacterInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, const FString&, ItemId, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, const FString&, ItemId, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, const FString&, ItemId, EEquipmentSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, const FString&, ItemId, EEquipmentSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UCharacterInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCharacterInventoryComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FInventory Inventory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FEquipmentSlots Equipment;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnItemAdded OnItemAdded;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnItemRemoved OnItemRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnItemEquipped OnItemEquipped;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnItemUnequipped OnItemUnequipped;

    UPROPERTY(BlueprintAssignable, Category = "Inventory Events")
    FOnInventoryChanged OnInventoryChanged;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool HasItem(const FString& ItemId, int32 Quantity = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 GetItemCount(const FString& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool EquipWeapon(const FString& WeaponId);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool EquipShield(const FString& ShieldId);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool EquipArmor(const FString& ArmorId);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UnequipWeapon();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UnequipShield();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UnequipArmor(EEquipmentSlot Slot);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseConsumable(const FString& ConsumableId);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    float GetTotalEquipmentWeight() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    float GetTotalInventoryWeight() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 GetTotalDefense() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    FWeaponData GetEquippedWeaponData() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TMap<FString, int32> GetEquipmentStatBonuses() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TransferItemToInventory(UCharacterInventoryComponent* TargetInventory, const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropItem(const FString& ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool EquipItemInstance(const FGuid& InstanceId);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    FItemInstance GetItemInstance(const FGuid& InstanceId) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool CanEquipShield() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool CanEquipWeapon(const FString& WeaponId) const;

protected:
    UPROPERTY()
    class UItemManager* ItemManager;

    bool EquipToSlot(const FString& ItemId, EEquipmentSlot Slot);
    bool UnequipFromSlot(EEquipmentSlot Slot);
    FItemInstance* GetEquipmentSlot(EEquipmentSlot Slot);
};