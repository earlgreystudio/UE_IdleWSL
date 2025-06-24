#include "CharacterInventoryComponent.h"
#include "UE_Idle/Managers/ItemDataTableManager.h"
#include "UE_Idle/Components/GlobalInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

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
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
    }
}

bool UCharacterInventoryComponent::AddItem(const FString& ItemId, int32 Quantity)
{
    UE_LOG(LogTemp, Warning, TEXT("CharacterInventory AddItem: Start - ItemId: %s, Quantity: %d"), *ItemId, Quantity);
    
    if (!ItemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterInventory AddItem: ItemManager is NULL"));
        return false;
    }

    bool bSuccess = Inventory.AddItem(ItemId, Quantity, ItemManager);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("CharacterInventory AddItem: Successfully added %d %s"), Quantity, *ItemId);
        OnItemAdded.Broadcast(ItemId, Quantity);
        OnInventoryChanged.Broadcast();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterInventory AddItem: Failed to add %d %s"), Quantity, *ItemId);
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

TMap<FString, int32> UCharacterInventoryComponent::GetAllItems() const
{
    TMap<FString, int32> Result;
    for (const FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.Quantity > 0)
        {
            Result.Add(Slot.ItemId, Slot.Quantity);
        }
    }
    return Result;
}

TArray<FInventorySlot> UCharacterInventoryComponent::GetAllInventorySlots() const
{
    TArray<FInventorySlot> Result;
    for (const FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.Quantity > 0)
        {
            // Get item data to check if it's stackable
            if (ItemManager)
            {
                FItemDataRow ItemData;
                if (ItemManager->GetItemData(Slot.ItemId, ItemData) && ItemData.StackSize == 1)
                {
                    // Non-stackable items - separate by durability AND equipment status
                    TMap<FString, FInventorySlot> StatusSlots; // Key: "Durability_EquipStatus"
                    
                    for (const FItemInstance& Instance : Slot.ItemInstances)
                    {
                        FString StatusKey = FString::Printf(TEXT("%d_%s"), 
                            Instance.CurrentDurability, 
                            Instance.bIsEquipped ? TEXT("Equipped") : TEXT("Unequipped"));
                        
                        if (!StatusSlots.Contains(StatusKey))
                        {
                            FInventorySlot NewSlot;
                            NewSlot.ItemId = Slot.ItemId;
                            NewSlot.Quantity = 0;
                            StatusSlots.Add(StatusKey, NewSlot);
                        }
                        
                        StatusSlots[StatusKey].ItemInstances.Add(Instance);
                        StatusSlots[StatusKey].Quantity++;
                    }
                    
                    // Add all status-based slots to result
                    for (const auto& StatusPair : StatusSlots)
                    {
                        Result.Add(StatusPair.Value);
                    }
                }
                else
                {
                    // Stackable items - add as single slot (these cannot be equipped)
                    Result.Add(Slot);
                }
            }
            else
            {
                // Fallback if ItemManager is not available
                Result.Add(Slot);
            }
        }
    }
    return Result;
}

bool UCharacterInventoryComponent::EquipWeapon(const FString& WeaponId)
{
    if (!ItemManager || !ItemManager->IsValidItem(WeaponId))
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(WeaponId, ItemData) || !ItemData.IsWeapon())
    {
        return false;
    }

    if (!HasItem(WeaponId) || !CanEquipWeapon(WeaponId))
    {
        return false;
    }

    // 現在の武器を装備解除
    if (!Equipment.Weapon.IsEmpty())
    {
        if (!UnequipWeapon())
        {
            return false;
        }
    }

    // 両手武器の場合、盾を自動的に外す
    if (ItemData.BlocksShield() && !Equipment.Shield.IsEmpty())
    {
        if (!UnequipShield())
        {
            return false;
        }
    }

    // インベントリ内で装備可能なアイテムインスタンスを検索
    FItemInstance* InstanceToEquip = nullptr;
    for (FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.ItemId == WeaponId)
        {
            for (FItemInstance& Instance : Slot.ItemInstances)
            {
                if (!Instance.bIsEquipped)
                {
                    InstanceToEquip = &Instance;
                    break;
                }
            }
            if (InstanceToEquip) break;
        }
    }

    if (!InstanceToEquip)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipWeapon: No unequipped instance found for %s"), *WeaponId);
        return false;
    }

    // 装備フラグを設定
    InstanceToEquip->bIsEquipped = true;
    InstanceToEquip->EquippedSlot = EEquipmentSlot::Weapon;
    
    // 装備スロットに参照を設定
    Equipment.Weapon = FEquipmentReference(WeaponId, InstanceToEquip->InstanceId);
    
    OnItemEquipped.Broadcast(WeaponId, EEquipmentSlot::Weapon);
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

    if (!Equipment.Shield.IsEmpty())
    {
        if (!UnequipShield())
        {
            return false;
        }
    }

    // インベントリ内で装備可能なアイテムインスタンスを検索
    FItemInstance* InstanceToEquip = nullptr;
    for (FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.ItemId == ShieldId)
        {
            for (FItemInstance& Instance : Slot.ItemInstances)
            {
                if (!Instance.bIsEquipped)
                {
                    InstanceToEquip = &Instance;
                    break;
                }
            }
            if (InstanceToEquip) break;
        }
    }

    if (!InstanceToEquip)
    {
        return false;
    }

    // 装備フラグを設定
    InstanceToEquip->bIsEquipped = true;
    InstanceToEquip->EquippedSlot = EEquipmentSlot::Shield;
    
    // 装備スロットに参照を設定
    Equipment.Shield = FEquipmentReference(ShieldId, InstanceToEquip->InstanceId);
    
    OnItemEquipped.Broadcast(ShieldId, EEquipmentSlot::Shield);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::EquipArmor(const FString& ArmorId)
{
    if (!ItemManager || !ItemManager->IsValidItem(ArmorId))
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ArmorId, ItemData) || !ItemData.IsArmor())
    {
        return false;
    }

    return EquipToSlot(ArmorId, (EEquipmentSlot)ItemData.EquipmentSlot);
}

bool UCharacterInventoryComponent::EquipToSlot(const FString& ItemId, EEquipmentSlot Slot)
{
    if (!HasItem(ItemId))
    {
        return false;
    }

    // アクセサリスロットの判定
    FEquipmentReference* TargetSlot = Equipment.GetSlot(Slot);
    if (!TargetSlot)
    {
        if (Slot == EEquipmentSlot::Accessory)
        {
            if (Equipment.Accessory1.IsEmpty())
            {
                TargetSlot = &Equipment.Accessory1;
            }
            else if (Equipment.Accessory2.IsEmpty())
            {
                TargetSlot = &Equipment.Accessory2;
            }
            else
            {
                return false; // 両方のアクセサリスロットが埋まっている
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (!TargetSlot->IsEmpty())
        {
            if (!UnequipFromSlot(Slot))
            {
                return false;
            }
        }
    }

    // インベントリ内で装備可能なアイテムインスタンスを検索
    FItemInstance* InstanceToEquip = nullptr;
    for (FInventorySlot& InventorySlot : Inventory.Slots)
    {
        if (InventorySlot.ItemId == ItemId)
        {
            for (FItemInstance& Instance : InventorySlot.ItemInstances)
            {
                if (!Instance.bIsEquipped)
                {
                    InstanceToEquip = &Instance;
                    break;
                }
            }
            if (InstanceToEquip) break;
        }
    }

    if (!InstanceToEquip)
    {
        return false;
    }

    // 装備フラグを設定
    InstanceToEquip->bIsEquipped = true;
    InstanceToEquip->EquippedSlot = Slot;
    
    // 装備スロットに参照を設定
    *TargetSlot = FEquipmentReference(ItemId, InstanceToEquip->InstanceId);

    OnItemEquipped.Broadcast(ItemId, Slot);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UnequipWeapon()
{
    if (Equipment.Weapon.IsEmpty())
    {
        return false;
    }

    FString WeaponId = Equipment.Weapon.ItemId;
    
    // インベントリ内の装備アイテムインスタンスを検索してフラグをリセット
    FItemInstance* EquippedInstance = Inventory.FindInstance(Equipment.Weapon.InstanceId);
    if (EquippedInstance)
    {
        EquippedInstance->bIsEquipped = false;
        EquippedInstance->EquippedSlot = EEquipmentSlot::None;
    }

    Equipment.Weapon.Clear();
    OnItemUnequipped.Broadcast(WeaponId, EEquipmentSlot::Weapon);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UnequipShield()
{
    if (Equipment.Shield.IsEmpty())
    {
        return false;
    }

    FString ShieldId = Equipment.Shield.ItemId;
    
    // インベントリ内の装備アイテムインスタンスを検索してフラグをリセット
    FItemInstance* EquippedInstance = Inventory.FindInstance(Equipment.Shield.InstanceId);
    if (EquippedInstance)
    {
        EquippedInstance->bIsEquipped = false;
        EquippedInstance->EquippedSlot = EEquipmentSlot::None;
    }

    Equipment.Shield.Clear();
    OnItemUnequipped.Broadcast(ShieldId, EEquipmentSlot::Shield);
    OnInventoryChanged.Broadcast();
    
    return true;
}

bool UCharacterInventoryComponent::UnequipArmor(EEquipmentSlot Slot)
{
    return UnequipFromSlot(Slot);
}

bool UCharacterInventoryComponent::UnequipFromSlot(EEquipmentSlot Slot)
{
    FEquipmentReference* SlotRef = Equipment.GetSlot(Slot);
    if (!SlotRef || SlotRef->IsEmpty())
    {
        return false;
    }

    FString ItemId = SlotRef->ItemId;
    
    // インベントリ内の装備アイテムインスタンスを検索してフラグをリセット
    FItemInstance* EquippedInstance = Inventory.FindInstance(SlotRef->InstanceId);
    if (EquippedInstance)
    {
        EquippedInstance->bIsEquipped = false;
        EquippedInstance->EquippedSlot = EEquipmentSlot::None;
    }

    SlotRef->Clear();
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

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ConsumableId, ItemData) || !ItemData.IsConsumable())
    {
        return false;
    }

    // TODO: Apply consumable effects to character using ItemData
    
    RemoveItem(ConsumableId, 1);
    return true;
}

bool UCharacterInventoryComponent::SellItemInstance(const FGuid& InstanceId)
{
    FItemInstance* Instance = Inventory.FindInstance(InstanceId);
    if (!Instance)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Item instance not found for selling: %s"), *InstanceId.ToString());
        return false;
    }

    int32 SellValue = GetItemInstanceSellPrice(InstanceId);
    
    // Remove instance from inventory
    FItemInstance RemovedInstance;
    if (Inventory.RemoveItemInstance(InstanceId, RemovedInstance))
    {
        // Add money to global storage via PlayerController
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                if (UGlobalInventoryComponent* GlobalInventory = PC->FindComponentByClass<UGlobalInventoryComponent>())
                {
                    GlobalInventory->AddMoney(SellValue);
                }
            }
        }
        
        OnInventoryChanged.Broadcast();
        UE_LOG(LogTemp, Log, TEXT("CharacterInventory: Sold item instance %s for %d money"), *Instance->ItemId, SellValue);
        return true;
    }
    
    return false;
}

int32 UCharacterInventoryComponent::GetItemInstanceSellPrice(const FGuid& InstanceId) const
{
    const FItemInstance* Instance = Inventory.FindInstance(InstanceId);
    if (!Instance || !ItemManager)
    {
        return 0;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(Instance->ItemId, ItemData))
    {
        return 0;
    }

    int32 BaseValue = ItemManager->GetItemValue(Instance->ItemId);
    
    // Check if item has durability
    if (ItemData.MaxDurability > 0)
    {
        float DurabilityRatio = (float)Instance->CurrentDurability / (float)ItemData.MaxDurability;
        
        // Durability-based pricing:
        // 100% durability = 80% of base value (new item)
        // 99% and below = starts at 40% and decreases linearly to 10% at 1 durability
        float SellRatio;
        if (DurabilityRatio >= 1.0f)
        {
            // Perfect condition
            SellRatio = 0.8f;
        }
        else
        {
            // Damaged: 40% base, then linear decrease
            // At 99% = 40%, At 1% = 10%
            float DamagedRatio = FMath::Clamp((DurabilityRatio * ItemData.MaxDurability - 1.0f) / (ItemData.MaxDurability - 1.0f), 0.0f, 1.0f);
            SellRatio = 0.1f + (DamagedRatio * 0.3f); // 10% to 40%
        }
        
        return FMath::RoundToInt(BaseValue * SellRatio);
    }
    else
    {
        // No durability (consumables, etc.) - standard 80%
        return FMath::RoundToInt(BaseValue * 0.8f);
    }
}

bool UCharacterInventoryComponent::EquipItem(const FString& ItemId)
{
    UE_LOG(LogTemp, Warning, TEXT("EquipItem: %s"), *ItemId);
    
    if (!ItemManager || !HasItem(ItemId))
    {
        UE_LOG(LogTemp, Error, TEXT("EquipItem: Failed - No ItemManager or Item not owned"));
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        UE_LOG(LogTemp, Error, TEXT("EquipItem: Failed - No item data"));
        return false;
    }

    // Check if item is equippable
    if (!ItemData.IsEquippable())
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipItem: Failed - Not equippable"));
        return false;
    }

    // Handle equipment conflicts before equipping
    if (ItemData.IsWeapon())
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipItem: Weapon - %s"), *ItemId);
        if (ItemData.BlocksShield() && !Equipment.Shield.IsEmpty())
        {
            UnequipShield();
        }
        return EquipWeapon(ItemId);
    }
    else if (ItemData.IsArmor())
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipItem: Armor - %s"), *ItemId);
        if (ItemData.EquipmentSlot == EEquipmentSlotTable::Shield && !Equipment.Weapon.IsEmpty())
        {
            FItemDataRow WeaponData;
            if (ItemManager->GetItemData(Equipment.Weapon.ItemId, WeaponData) && WeaponData.BlocksShield())
            {
                UnequipWeapon();
            }
        }
        bool bResult = EquipArmor(ItemId);
        UE_LOG(LogTemp, Warning, TEXT("EquipItem: Armor result - %s"), bResult ? TEXT("Success") : TEXT("Failed"));
        return bResult;
    }

    UE_LOG(LogTemp, Error, TEXT("EquipItem: Failed - Unknown type"));
    return false;
}

bool UCharacterInventoryComponent::EquipItemInstance(const FGuid& InstanceId)
{
    FItemInstance* Instance = Inventory.FindInstance(InstanceId);
    if (!Instance)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipItemInstance: Instance not found - %s"), *InstanceId.ToString());
        return false;
    }

    return EquipItem(Instance->ItemId);
}

bool UCharacterInventoryComponent::CanEquipItem(const FString& ItemId) const
{
    if (!ItemManager)
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        return false;
    }

    // Simply check if item is equippable (weapon or armor)
    return ItemData.IsEquippable();
}

bool UCharacterInventoryComponent::TransferToCharacter(UCharacterInventoryComponent* TargetCharacter, const FString& ItemId, int32 Quantity)
{
    if (!TargetCharacter || !HasItem(ItemId, Quantity))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Cannot transfer - target invalid or insufficient items"));
        return false;
    }

    // Remove from this character
    if (!RemoveItem(ItemId, Quantity))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Failed to remove items for transfer"));
        return false;
    }

    // Add to target character
    if (!TargetCharacter->AddItem(ItemId, Quantity))
    {
        // Rollback: add back to this character
        AddItem(ItemId, Quantity);
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Failed to add items to target, rolled back"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("CharacterInventory: Transferred %d %s to another character"), Quantity, *ItemId);
    return true;
}

bool UCharacterInventoryComponent::TransferItemInstanceToCharacter(UCharacterInventoryComponent* TargetCharacter, const FGuid& InstanceId)
{
    if (!TargetCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Cannot transfer instance - target character is null"));
        return false;
    }

    FItemInstance* Instance = Inventory.FindInstance(InstanceId);
    if (!Instance)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Item instance not found for transfer: %s"), *InstanceId.ToString());
        return false;
    }

    FString ItemId = Instance->ItemId;

    // Remove instance from this character
    FItemInstance RemovedInstance;
    if (!Inventory.RemoveItemInstance(InstanceId, RemovedInstance))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Failed to remove instance for transfer"));
        return false;
    }

    // Add instance to target character
    if (!TargetCharacter->Inventory.AddItemInstance(RemovedInstance, ItemManager))
    {
        // Rollback: add instance back to this character
        Inventory.AddItemInstance(RemovedInstance, ItemManager);
        UE_LOG(LogTemp, Warning, TEXT("CharacterInventory: Failed to add instance to target, rolled back"));
        return false;
    }

    OnInventoryChanged.Broadcast();
    TargetCharacter->OnInventoryChanged.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("CharacterInventory: Transferred item instance %s (%s) to another character"), *ItemId, *InstanceId.ToString());
    return true;
}

float UCharacterInventoryComponent::GetTotalEquipmentWeight() const
{
    if (!ItemManager)
    {
        return 0.0f;
    }
    
    float TotalWeight = 0.0f;
    
    auto AddWeightFromSlot = [&](const FEquipmentReference& SlotRef)
    {
        if (!SlotRef.IsEmpty())
        {
            FItemInstance* Instance = const_cast<FInventory&>(Inventory).FindInstance(SlotRef.InstanceId);
            if (Instance)
            {
                FItemDataRow ItemData;
                if (ItemManager->GetItemData(Instance->ItemId, ItemData))
                {
                    TotalWeight += ItemData.Weight;
                }
            }
        }
    };
    
    AddWeightFromSlot(Equipment.Weapon);
    AddWeightFromSlot(Equipment.Shield);
    AddWeightFromSlot(Equipment.Head);
    AddWeightFromSlot(Equipment.Body);
    AddWeightFromSlot(Equipment.Legs);
    AddWeightFromSlot(Equipment.Hands);
    AddWeightFromSlot(Equipment.Feet);
    AddWeightFromSlot(Equipment.Accessory1);
    AddWeightFromSlot(Equipment.Accessory2);
    
    return TotalWeight;
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
    
    int32 TotalDefense = 0;
    
    auto AddDefenseFromSlot = [&](const FEquipmentReference& SlotRef)
    {
        if (!SlotRef.IsEmpty())
        {
            FItemInstance* Instance = const_cast<FInventory&>(Inventory).FindInstance(SlotRef.InstanceId);
            if (Instance)
            {
                FItemDataRow ItemData;
                if (ItemManager->GetItemData(Instance->ItemId, ItemData))
                {
                    TotalDefense += ItemData.Defense;
                }
            }
        }
    };
    
    AddDefenseFromSlot(Equipment.Shield);
    AddDefenseFromSlot(Equipment.Head);
    AddDefenseFromSlot(Equipment.Body);
    AddDefenseFromSlot(Equipment.Legs);
    AddDefenseFromSlot(Equipment.Hands);
    AddDefenseFromSlot(Equipment.Feet);
    
    return TotalDefense;
}

FItemDataRow UCharacterInventoryComponent::GetEquippedWeaponData() const
{
    FItemDataRow ItemData;
    if (!ItemManager || Equipment.Weapon.IsEmpty())
    {
        return ItemData; // Return empty item data
    }
    
    ItemManager->GetItemData(Equipment.Weapon.ItemId, ItemData);
    return ItemData;
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
    if (!ItemManager || Equipment.Weapon.IsEmpty())
    {
        return true; // 武器がないなら盾装備可能
    }
    
    FItemDataRow WeaponData;
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
    
    FItemDataRow WeaponData;
    if (!ItemManager->GetItemData(WeaponId, WeaponData) || !WeaponData.IsWeapon())
    {
        return false;
    }
    
    // 両手武器を装備しようとして、盾が装備されている場合は不可
    if (WeaponData.BlocksShield() && !Equipment.Shield.IsEmpty())
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

    auto AddBonuses = [&](const FEquipmentReference& SlotRef)
    {
        if (!SlotRef.IsEmpty())
        {
            FItemInstance* Instance = const_cast<FInventory&>(Inventory).FindInstance(SlotRef.InstanceId);
            if (Instance)
            {
                FItemDataRow ItemData;
                if (ItemManager->GetItemData(Instance->ItemId, ItemData) && ItemData.IsArmor())
                {
                    // TODO: StatBonusesは現在FItemDataRowに存在しないため、将来的に追加する必要がある
                    // 今のところは単純なボーナスシステムのみ
                    // Defense値をStrengthボーナスとして使用
                    if (ItemData.Defense > 0)
                    {
                        if (TotalBonuses.Contains(TEXT("Defense")))
                        {
                            TotalBonuses[TEXT("Defense")] += ItemData.Defense;
                        }
                        else
                        {
                            TotalBonuses.Add(TEXT("Defense"), ItemData.Defense);
                        }
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
    FEquipmentReference* SlotRef = Equipment.GetSlot(Slot);
    if (!SlotRef || SlotRef->IsEmpty())
    {
        return nullptr;
    }
    
    // インベントリ内から装備アイテムインスタンスを検索
    return Inventory.FindInstance(SlotRef->InstanceId);
}