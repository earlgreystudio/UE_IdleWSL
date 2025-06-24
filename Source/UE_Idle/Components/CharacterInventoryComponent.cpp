#include "CharacterInventoryComponent.h"
#include "UE_Idle/Managers/ItemManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UCharacterInventoryComponent::UCharacterInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    Inventory.MaxSlots = 50;
    Inventory.MaxWeight = 100.0f;
}

void UCharacterInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        ItemManager = GameInstance->GetSubsystem<UItemManager>();
    }
}

bool UCharacterInventoryComponent::AddItem(const FString& ItemId, int32 Quantity)
{
    if (!ItemManager)
    {
        return false;
    }

    bool bSuccess = Inventory.AddItem(ItemId, Quantity, ItemManager);
    
    if (bSuccess)
    {
        OnItemAdded.Broadcast(ItemId, Quantity);
        OnInventoryChanged.Broadcast();
    }
    
    return bSuccess;
}

bool UCharacterInventoryComponent::RemoveItem(const FString& ItemId, int32 Quantity)
{
    bool bSuccess = Inventory.RemoveItem(ItemId, Quantity);
    
    if (bSuccess)
    {
        OnItemRemoved.Broadcast(ItemId, Quantity);
        OnInventoryChanged.Broadcast();
    }
    
    return bSuccess;
}

bool UCharacterInventoryComponent::HasItem(const FString& ItemId, int32 Quantity) const
{
    return Inventory.GetItemCount(ItemId) >= Quantity;
}

int32 UCharacterInventoryComponent::GetItemCount(const FString& ItemId) const
{
    return Inventory.GetItemCount(ItemId);
}

bool UCharacterInventoryComponent::EquipWeapon(const FString& WeaponId)
{
    if (!ItemManager || !ItemManager->IsValidItem(WeaponId))
    {
        return false;
    }

    FWeaponData WeaponData;
    if (!ItemManager->GetWeaponData(WeaponId, WeaponData))
    {
        return false;
    }

    if (!HasItem(WeaponId) || !CanEquipWeapon(WeaponId))
    {
        return false;
    }

    if (!Equipment.Weapon.ItemId.IsEmpty())
    {
        if (!UnequipWeapon())
        {
            return false;
        }
    }

    // 両手武器の場合、盾を自動的に外す
    FItemData ItemData;
    if (ItemManager->GetItemData(WeaponId, ItemData) && ItemData.BlocksShield() && !Equipment.Shield.ItemId.IsEmpty())
    {
        if (!UnequipShield())
        {
            return false;
        }
    }

    // Find the weapon instance in inventory
    FInventorySlot* Slot = Inventory.FindSlot(WeaponId);
    if (!Slot || Slot->Quantity == 0)
    {
        return false;
    }

    // Take the first instance if non-stackable, or create new instance if stackable
    if (Slot->ItemInstances.Num() > 0)
    {
        Equipment.Weapon = Slot->ItemInstances[0];
        Slot->RemoveInstance(Equipment.Weapon.InstanceId);
    }
    else
    {
        Equipment.Weapon = FItemInstance(WeaponId);
        Slot->RemoveStackableItem(1);
    }
    
    // Clean up empty slots
    if (Slot->Quantity == 0)
    {
        Inventory.Slots.RemoveAll([&WeaponId](const FInventorySlot& InSlot)
        {
            return InSlot.ItemId == WeaponId && InSlot.Quantity == 0;
        });
    }
    
    OnItemEquipped.Broadcast(WeaponId, EEquipmentSlot::Head);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::EquipShield(const FString& ShieldId)
{
    if (!ItemManager || !ItemManager->IsValidItem(ShieldId))
    {
        return false;
    }

    if (!HasItem(ShieldId) || !CanEquipShield())
    {
        return false;
    }

    if (!Equipment.Shield.ItemId.IsEmpty())
    {
        if (!UnequipShield())
        {
            return false;
        }
    }

    // Find the shield instance in inventory
    FInventorySlot* Slot = Inventory.FindSlot(ShieldId);
    if (!Slot || Slot->Quantity == 0)
    {
        return false;
    }

    // Take the first instance if non-stackable, or create new instance if stackable
    if (Slot->ItemInstances.Num() > 0)
    {
        Equipment.Shield = Slot->ItemInstances[0];
        Slot->RemoveInstance(Equipment.Shield.InstanceId);
    }
    else
    {
        Equipment.Shield = FItemInstance(ShieldId);
        Slot->RemoveStackableItem(1);
    }
    
    // Clean up empty slots
    if (Slot->Quantity == 0)
    {
        Inventory.Slots.RemoveAll([&ShieldId](const FInventorySlot& InSlot)
        {
            return InSlot.ItemId == ShieldId && InSlot.Quantity == 0;
        });
    }
    
    OnItemEquipped.Broadcast(ShieldId, EEquipmentSlot::Head);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::EquipArmor(const FString& ArmorId)
{
    if (!ItemManager || !ItemManager->IsValidItem(ArmorId))
    {
        return false;
    }

    FArmorData ArmorData;
    if (!ItemManager->GetArmorData(ArmorId, ArmorData))
    {
        return false;
    }

    return EquipToSlot(ArmorId, ArmorData.EquipmentSlot);
}

bool UCharacterInventoryComponent::EquipToSlot(const FString& ItemId, EEquipmentSlot Slot)
{
    if (!HasItem(ItemId))
    {
        return false;
    }

    // Find the item instance in inventory
    FInventorySlot* InventorySlot = Inventory.FindSlot(ItemId);
    if (!InventorySlot || InventorySlot->Quantity == 0)
    {
        return false;
    }

    FItemInstance* SlotItem = Equipment.GetSlot(Slot);
    if (!SlotItem)
    {
        if (Slot == EEquipmentSlot::Accessory)
        {
            if (Equipment.Accessory1.ItemId.IsEmpty())
            {
                SlotItem = &Equipment.Accessory1;
            }
            else if (Equipment.Accessory2.ItemId.IsEmpty())
            {
                SlotItem = &Equipment.Accessory2;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (!SlotItem->ItemId.IsEmpty())
        {
            if (!UnequipFromSlot(Slot))
            {
                return false;
            }
        }
    }

    // Take the first instance if non-stackable, or create new instance if stackable
    if (InventorySlot->ItemInstances.Num() > 0)
    {
        *SlotItem = InventorySlot->ItemInstances[0];
        InventorySlot->RemoveInstance(SlotItem->InstanceId);
    }
    else
    {
        *SlotItem = FItemInstance(ItemId);
        InventorySlot->RemoveStackableItem(1);
    }
    
    // Clean up empty slots
    if (InventorySlot->Quantity == 0)
    {
        Inventory.Slots.RemoveAll([&ItemId](const FInventorySlot& InSlot)
        {
            return InSlot.ItemId == ItemId && InSlot.Quantity == 0;
        });
    }

    OnItemEquipped.Broadcast(ItemId, Slot);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UnequipWeapon()
{
    if (Equipment.Weapon.ItemId.IsEmpty())
    {
        return false;
    }

    FString WeaponId = Equipment.Weapon.ItemId;
    
    // Add the weapon instance back to inventory
    if (!Inventory.AddItemInstance(Equipment.Weapon, ItemManager))
    {
        return false;
    }

    Equipment.Weapon = FItemInstance();
    OnItemUnequipped.Broadcast(WeaponId, EEquipmentSlot::Weapon);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UnequipShield()
{
    if (Equipment.Shield.ItemId.IsEmpty())
    {
        return false;
    }

    FString ShieldId = Equipment.Shield.ItemId;
    
    // Add the shield instance back to inventory
    if (!Inventory.AddItemInstance(Equipment.Shield, ItemManager))
    {
        return false;
    }

    Equipment.Shield = FItemInstance();
    OnItemUnequipped.Broadcast(ShieldId, EEquipmentSlot::Head);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UnequipArmor(EEquipmentSlot Slot)
{
    return UnequipFromSlot(Slot);
}

bool UCharacterInventoryComponent::UnequipFromSlot(EEquipmentSlot Slot)
{
    FItemInstance* SlotItem = Equipment.GetSlot(Slot);
    if (!SlotItem || SlotItem->ItemId.IsEmpty())
    {
        return false;
    }

    FString ItemId = SlotItem->ItemId;
    
    // Add the item instance back to inventory
    if (!Inventory.AddItemInstance(*SlotItem, ItemManager))
    {
        return false;
    }

    *SlotItem = FItemInstance();
    OnItemUnequipped.Broadcast(ItemId, Slot);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UseConsumable(const FString& ConsumableId)
{
    if (!ItemManager || !HasItem(ConsumableId))
    {
        return false;
    }

    FConsumableData ConsumableData;
    if (!ItemManager->GetConsumableData(ConsumableId, ConsumableData))
    {
        return false;
    }

    // TODO: Apply consumable effects to character
    
    RemoveItem(ConsumableId, 1);
    return true;
}

float UCharacterInventoryComponent::GetTotalEquipmentWeight() const
{
    if (!ItemManager)
    {
        return 0.0f;
    }
    
    return Equipment.GetTotalWeight(ItemManager);
}

float UCharacterInventoryComponent::GetTotalInventoryWeight() const
{
    if (!ItemManager)
    {
        return 0.0f;
    }
    
    return Inventory.GetTotalWeight(ItemManager) + GetTotalEquipmentWeight();
}

int32 UCharacterInventoryComponent::GetTotalDefense() const
{
    if (!ItemManager)
    {
        return 0;
    }
    
    return Equipment.GetTotalDefense(ItemManager);
}

FWeaponData UCharacterInventoryComponent::GetEquippedWeaponData() const
{
    FWeaponData WeaponData;
    if (!ItemManager || Equipment.Weapon.ItemId.IsEmpty())
    {
        return WeaponData; // Return empty weapon data
    }
    
    ItemManager->GetWeaponData(Equipment.Weapon.ItemId, WeaponData);
    return WeaponData;
}

bool UCharacterInventoryComponent::EquipItemInstance(const FGuid& InstanceId)
{
    FItemInstance* Instance = Inventory.FindInstance(InstanceId);
    if (!Instance || !ItemManager)
    {
        return false;
    }

    EItemType ItemType = ItemManager->GetItemType(Instance->ItemId);
    
    if (ItemType == EItemType::Weapon)
    {
        return EquipWeapon(Instance->ItemId);
    }
    else if (ItemType == EItemType::Armor)
    {
        return EquipArmor(Instance->ItemId);
    }
    
    return false;
}

FItemInstance UCharacterInventoryComponent::GetItemInstance(const FGuid& InstanceId) const
{
    FItemInstance* Instance = const_cast<FInventory&>(Inventory).FindInstance(InstanceId);
    if (Instance)
    {
        return *Instance;
    }
    return FItemInstance(); // Return empty instance if not found
}

bool UCharacterInventoryComponent::CanEquipShield() const
{
    if (!ItemManager || Equipment.Weapon.ItemId.IsEmpty())
    {
        return true; // 武器がないなら盾装備可能
    }
    
    FItemData WeaponData;
    if (ItemManager->GetItemData(Equipment.Weapon.ItemId, WeaponData))
    {
        return !WeaponData.BlocksShield();
    }
    return true;
}

bool UCharacterInventoryComponent::CanEquipWeapon(const FString& WeaponId) const
{
    if (!ItemManager)
    {
        return false;
    }
    
    FItemData WeaponData;
    if (!ItemManager->GetItemData(WeaponId, WeaponData) || !WeaponData.IsWeapon())
    {
        return false;
    }
    
    // 両手武器を装備しようとして、盾が装備されている場合は不可
    if (WeaponData.BlocksShield() && !Equipment.Shield.ItemId.IsEmpty())
    {
        return false;
    }
    
    return true;
}

TMap<FString, int32> UCharacterInventoryComponent::GetEquipmentStatBonuses() const
{
    TMap<FString, int32> TotalBonuses;
    
    if (!ItemManager)
    {
        return TotalBonuses;
    }

    auto AddBonuses = [&](const FItemInstance& Item)
    {
        if (!Item.ItemId.IsEmpty())
        {
            FArmorData Armor;
            if (ItemManager->GetArmorData(Item.ItemId, Armor))
            {
                for (const auto& Bonus : Armor.StatBonuses)
                {
                    if (TotalBonuses.Contains(Bonus.Key))
                    {
                        TotalBonuses[Bonus.Key] += Bonus.Value;
                    }
                    else
                    {
                        TotalBonuses.Add(Bonus.Key, Bonus.Value);
                    }
                }
            }
        }
    };

    AddBonuses(Equipment.Head);
    AddBonuses(Equipment.Body);
    AddBonuses(Equipment.Legs);
    AddBonuses(Equipment.Hands);
    AddBonuses(Equipment.Feet);
    AddBonuses(Equipment.Accessory1);
    AddBonuses(Equipment.Accessory2);

    return TotalBonuses;
}

bool UCharacterInventoryComponent::TransferItemToInventory(UCharacterInventoryComponent* TargetInventory, const FString& ItemId, int32 Quantity)
{
    if (!TargetInventory || !HasItem(ItemId, Quantity))
    {
        return false;
    }

    if (TargetInventory->AddItem(ItemId, Quantity))
    {
        RemoveItem(ItemId, Quantity);
        return true;
    }

    return false;
}

void UCharacterInventoryComponent::DropItem(const FString& ItemId, int32 Quantity)
{
    if (!HasItem(ItemId, Quantity))
    {
        return;
    }

    // TODO: Spawn dropped item actor in world
    
    RemoveItem(ItemId, Quantity);
}

FItemInstance* UCharacterInventoryComponent::GetEquipmentSlot(EEquipmentSlot Slot)
{
    return Equipment.GetSlot(Slot);
}