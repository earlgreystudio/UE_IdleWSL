#include "ItemTypes.h"
#include "UE_Idle/Managers/ItemDataTableManager.h"
#include "UE_Idle/Types/ItemDataTable.h"

float FEquipmentSlots::GetTotalWeight(UItemDataTableManager* ItemManager) const
{
    if (!ItemManager)
    {
        return 0.0f;
    }

    float TotalWeight = 0.0f;

    auto AddItemWeight = [&](const FEquipmentReference& EquipRef)
    {
        if (!EquipRef.IsEmpty())
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(EquipRef.ItemId, ItemData))
            {
                TotalWeight += ItemData.Weight;
            }
        }
    };

    AddItemWeight(Weapon);
    AddItemWeight(Shield);
    AddItemWeight(Head);
    AddItemWeight(Body);
    AddItemWeight(Legs);
    AddItemWeight(Hands);
    AddItemWeight(Feet);
    AddItemWeight(Accessory1);
    AddItemWeight(Accessory2);

    return TotalWeight;
}

int32 FEquipmentSlots::GetTotalDefense(UItemDataTableManager* ItemManager) const
{
    if (!ItemManager)
    {
        return 0;
    }

    int32 TotalDefense = 0;

    auto AddArmorDefense = [&](const FEquipmentReference& EquipRef)
    {
        if (!EquipRef.IsEmpty())
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(EquipRef.ItemId, ItemData))
            {
                TotalDefense += ItemData.Defense;
            }
        }
    };

    AddArmorDefense(Shield);
    AddArmorDefense(Head);
    AddArmorDefense(Body);
    AddArmorDefense(Legs);
    AddArmorDefense(Hands);
    AddArmorDefense(Feet);

    return TotalDefense;
}

bool FInventory::AddItem(const FString& ItemId, int32 Quantity, UItemDataTableManager* ItemManager)
{
    if (!ItemManager || !ItemManager->IsValidItem(ItemId) || Quantity <= 0)
    {
        return false;
    }

    // Get item data first
    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        return false;
    }
    
    int32 StackSize = ItemData.StackSize;
    EItemTypeTable ItemType = ItemData.ItemType;
    float ItemWeight = ItemData.Weight;
    float NewTotalWeight = GetTotalWeight(ItemManager) + (ItemWeight * Quantity);
    if (NewTotalWeight > MaxWeight)
    {
        return false;
    }
    
    // Stackable items (consumables, materials, etc.)
    if (StackSize > 1)
    {
        FInventorySlot* ExistingSlot = FindSlot(ItemId);
        if (ExistingSlot)
        {
            ExistingSlot->AddStackableItem(Quantity);
            return true;
        }
        else
        {
            if (GetUsedSlots() >= MaxSlots)
            {
                return false;
            }
            FInventorySlot NewSlot(ItemId);
            NewSlot.AddStackableItem(Quantity);
            Slots.Add(NewSlot);
            return true;
        }
    }
    // Non-stackable items (weapons, armor)
    else
    {
        if (GetUsedSlots() + Quantity > MaxSlots)
        {
            return false;
        }
        
        for (int32 i = 0; i < Quantity; i++)
        {
            FItemInstance Instance(ItemId);
            AddItemInstance(Instance, ItemManager);
        }
        return true;
    }
}

bool FInventory::AddItemInstance(const FItemInstance& Instance, UItemDataTableManager* ItemManager)
{
    if (!ItemManager || Instance.ItemId.IsEmpty())
    {
        return false;
    }
    
    FInventorySlot* Slot = FindSlot(Instance.ItemId);
    if (!Slot)
    {
        if (GetUsedSlots() >= MaxSlots)
        {
            return false;
        }
        FInventorySlot NewSlot(Instance.ItemId);
        NewSlot.AddInstance(Instance);
        Slots.Add(NewSlot);
    }
    else
    {
        Slot->AddInstance(Instance);
    }
    return true;
}

bool FInventory::RemoveItem(const FString& ItemId, int32 Quantity)
{
    if (Quantity <= 0)
    {
        return false;
    }

    FInventorySlot* Slot = FindSlot(ItemId);
    if (!Slot || Slot->Quantity < Quantity)
    {
        return false;
    }

    // Stackable items
    if (Slot->ItemInstances.Num() == 0)
    {
        bool bSuccess = Slot->RemoveStackableItem(Quantity);
        if (bSuccess && Slot->Quantity == 0)
        {
            Slots.RemoveAll([&ItemId](const FInventorySlot& InSlot)
            {
                return InSlot.ItemId == ItemId;
            });
        }
        return bSuccess;
    }
    // Non-stackable items
    else
    {
        int32 RemovedCount = 0;
        while (RemovedCount < Quantity && Slot->ItemInstances.Num() > 0)
        {
            Slot->ItemInstances.RemoveAt(0);
            RemovedCount++;
        }
        Slot->Quantity = Slot->ItemInstances.Num();
        
        if (Slot->Quantity == 0)
        {
            Slots.RemoveAll([&ItemId](const FInventorySlot& InSlot)
            {
                return InSlot.ItemId == ItemId;
            });
        }
        
        return RemovedCount == Quantity;
    }
}

bool FInventory::RemoveItemInstance(const FGuid& InstanceId, FItemInstance& OutInstance)
{
    for (FInventorySlot& Slot : Slots)
    {
        if (FItemInstance* Instance = Slot.GetInstance(InstanceId))
        {
            OutInstance = *Instance;
            Slot.RemoveInstance(InstanceId);
            
            if (Slot.Quantity == 0)
            {
                Slots.RemoveAll([&Slot](const FInventorySlot& InSlot)
                {
                    return InSlot.ItemId == Slot.ItemId;
                });
            }
            return true;
        }
    }
    return false;
}

FInventorySlot* FInventory::FindSlot(const FString& ItemId)
{
    for (FInventorySlot& Slot : Slots)
    {
        if (Slot.ItemId == ItemId)
        {
            return &Slot;
        }
    }
    return nullptr;
}

FItemInstance* FInventory::FindInstance(const FGuid& InstanceId)
{
    for (FInventorySlot& Slot : Slots)
    {
        if (FItemInstance* Instance = Slot.GetInstance(InstanceId))
        {
            return Instance;
        }
    }
    return nullptr;
}

const FItemInstance* FInventory::FindInstance(const FGuid& InstanceId) const
{
    for (const FInventorySlot& Slot : Slots)
    {
        if (const FItemInstance* Instance = Slot.GetInstance(InstanceId))
        {
            return Instance;
        }
    }
    return nullptr;
}

int32 FInventory::GetItemCount(const FString& ItemId) const
{
    for (const FInventorySlot& Slot : Slots)
    {
        if (Slot.ItemId == ItemId)
        {
            return Slot.Quantity;
        }
    }
    return 0;
}

float FInventory::GetTotalWeight(UItemDataTableManager* ItemManager) const
{
    if (!ItemManager)
    {
        return 0.0f;
    }

    float TotalWeight = 0.0f;
    for (const FInventorySlot& Slot : Slots)
    {
        FItemDataRow ItemData;
        float ItemWeight = 0.0f;
        if (ItemManager->GetItemData(Slot.ItemId, ItemData))
        {
            ItemWeight = ItemData.Weight;
        }
        TotalWeight += ItemWeight * Slot.Quantity;
    }
    return TotalWeight;
}

int32 FInventory::GetUsedSlots() const
{
    int32 UsedSlots = 0;
    for (const FInventorySlot& Slot : Slots)
    {
        // Stackable items take 1 slot
        if (Slot.ItemInstances.Num() == 0)
        {
            UsedSlots += 1;
        }
        // Non-stackable items take 1 slot per instance
        else
        {
            UsedSlots += Slot.ItemInstances.Num();
        }
    }
    return UsedSlots;
}

bool FInventory::HasSpace(const FString& ItemId, int32 Quantity, UItemDataTableManager* ItemManager) const
{
    if (!ItemManager || !ItemManager->IsValidItem(ItemId) || Quantity <= 0)
    {
        return false;
    }

    // Check weight
    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        return false;
    }
    float ItemWeight = ItemData.Weight;
    float NewTotalWeight = GetTotalWeight(ItemManager) + (ItemWeight * Quantity);
    if (NewTotalWeight > MaxWeight)
    {
        return false;
    }

    FItemDataRow CheckItemData;
    if (!ItemManager->GetItemData(ItemId, CheckItemData))
    {
        return false;
    }
    int32 StackSize = CheckItemData.StackSize;
    
    // Stackable items
    if (StackSize > 1)
    {
        const FInventorySlot* ExistingSlot = const_cast<FInventory*>(this)->FindSlot(ItemId);
        if (ExistingSlot)
        {
            return true; // Can add to existing stack
        }
        return GetUsedSlots() < MaxSlots; // Need new slot
    }
    // Non-stackable items
    else
    {
        return GetUsedSlots() + Quantity <= MaxSlots;
    }
}

// FItemDataRowの実装メソッドはItemDataTable.cppに移動されました