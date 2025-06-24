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

    auto AddItemWeight = [&](const FItemInstance& Item)
    {
        if (!Item.ItemId.IsEmpty())
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(Item.ItemId, ItemData))
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

    auto AddArmorDefense = [&](const FItemInstance& Item)
    {
        if (!Item.ItemId.IsEmpty())
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(Item.ItemId, ItemData))
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

int32 FItemData::GetRealWorldValue() const
{
    float Multiplier = GetCategoryRealWorldMultiplier(TradeCategory);
    return FMath::RoundToInt(BaseValue * Multiplier);
}

int32 FItemData::GetOtherWorldValue() const
{
    float Multiplier = GetCategoryOtherWorldMultiplier(TradeCategory);
    return FMath::RoundToInt(BaseValue * Multiplier);
}

float FItemData::GetCategoryRealWorldMultiplier(ETradeCategory Category)
{
    switch (Category)
    {
    case ETradeCategory::MeleeWeapons:      return 0.1f;   // 現世では実用性なし
    case ETradeCategory::ModernWeapons:     return 1.0f;   // 需要あり
    case ETradeCategory::Gems:              return 1.0f;   // 装飾品価値
    case ETradeCategory::Antiques:          return 1.2f;   // コレクター需要
    case ETradeCategory::Electronics:       return 1.0f;   // 通常価格
    case ETradeCategory::ModernGoods:       return 0.8f;   // 中古品扱い
    case ETradeCategory::Food:              return 1.0f;   // 普通
    case ETradeCategory::MagicMaterials:    return 0.1f;   // オカルト扱い
    case ETradeCategory::MonsterMaterials:  return 2.0f;   // 研究価値
    case ETradeCategory::Medicine:          return 1.0f;   // 医療用
    case ETradeCategory::CommonMaterials:   return 0.9f;   // 一般的
    case ETradeCategory::Luxury:            return 1.1f;   // 贅沢品
    default:                                return 1.0f;
    }
}

float FItemData::GetCategoryOtherWorldMultiplier(ETradeCategory Category)
{
    switch (Category)
    {
    case ETradeCategory::MeleeWeapons:      return 1.0f;   // 実戦で使用
    case ETradeCategory::ModernWeapons:     return 0.1f;   // 魔法に劣る
    case ETradeCategory::Gems:              return 1.0f;   // 魔法触媒
    case ETradeCategory::Antiques:          return 0.2f;   // 文化的価値なし
    case ETradeCategory::Electronics:       return 0.05f;  // 電気なし
    case ETradeCategory::ModernGoods:       return 1.5f;   // 珍しい技術
    case ETradeCategory::Food:              return 2.0f;   // 未知の味
    case ETradeCategory::MagicMaterials:    return 1.5f;   // 魔法に必須
    case ETradeCategory::MonsterMaterials:  return 1.0f;   // 普通の素材
    case ETradeCategory::Medicine:          return 1.8f;   // 高度な治療
    case ETradeCategory::CommonMaterials:   return 1.0f;   // 標準価格
    case ETradeCategory::Luxury:            return 0.5f;   // 実用性重視
    default:                                return 1.0f;
    }
}

int32 FItemData::GetModifiedAttackPower() const
{
    return FMath::RoundToInt(AttackPower * GetQualityModifier(Quality));
}

int32 FItemData::GetModifiedDefense() const
{
    return FMath::RoundToInt(Defense * GetQualityModifier(Quality));
}

int32 FItemData::GetModifiedDurability() const
{
    return FMath::RoundToInt(MaxDurability * GetQualityModifier(Quality));
}

int32 FItemData::GetModifiedValue() const
{
    return FMath::RoundToInt(BaseValue * GetQualityModifier(Quality));
}

float FItemData::GetQualityModifier(EItemQuality Quality)
{
    switch (Quality)
    {
    case EItemQuality::Poor:        return 0.7f;   // 粗悪: -30%
    case EItemQuality::Common:      return 1.0f;   // 普通: 基準値
    case EItemQuality::Good:        return 1.3f;   // できの良い: +30%
    case EItemQuality::Masterwork:  return 1.6f;   // 名匠: +60%
    case EItemQuality::Legendary:   return 2.0f;   // マスターワーク: +100%
    default:                        return 1.0f;
    }
}