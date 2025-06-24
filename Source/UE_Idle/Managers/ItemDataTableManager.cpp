#include "ItemDataTableManager.h"
#include "Engine/DataTable.h"

void UItemDataTableManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // DataTable will be set in Blueprint or loaded programmatically
    UE_LOG(LogTemp, Log, TEXT("ItemDataTableManager initialized"));
}

bool UItemDataTableManager::GetItemData(const FString& ItemId, FItemDataRow& OutItemData) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    if (ItemData)
    {
        OutItemData = *ItemData;
        return true;
    }
    
    return false;
}

FItemDataRow UItemDataTableManager::GetItemDataByRowName(const FName& RowName) const
{
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemDataTable is not set"));
        return FItemDataRow();
    }

    FItemDataRow* ItemData = ItemDataTable->FindRow<FItemDataRow>(RowName, TEXT(""));
    if (ItemData)
    {
        return *ItemData;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Item not found with row name: %s"), *RowName.ToString());
    return FItemDataRow();
}

bool UItemDataTableManager::IsValidItem(const FString& ItemId) const
{
    return FindItemByItemId(ItemId) != nullptr;
}

bool UItemDataTableManager::CanEquipToSlot(const FString& ItemId, EEquipmentSlotTable Slot) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    if (!ItemData)
    {
        return false;
    }
    
    return ItemData->EquipmentSlot == Slot;
}

TArray<FItemDataRow> UItemDataTableManager::GetAllItems() const
{
    TArray<FItemDataRow> AllItems;
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemDataTable is not set"));
        return AllItems;
    }
    
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FItemDataRow* ItemData = ItemDataTable->FindRow<FItemDataRow>(RowName, TEXT(""));
        if (ItemData)
        {
            AllItems.Add(*ItemData);
        }
    }
    
    return AllItems;
}

TArray<FItemDataRow> UItemDataTableManager::GetItemsByType(EItemTypeTable ItemType) const
{
    TArray<FItemDataRow> FilteredItems;
    
    if (!ItemDataTable)
    {
        return FilteredItems;
    }
    
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FItemDataRow* ItemData = ItemDataTable->FindRow<FItemDataRow>(RowName, TEXT(""));
        if (ItemData && ItemData->ItemType == ItemType)
        {
            FilteredItems.Add(*ItemData);
        }
    }
    
    return FilteredItems;
}

TArray<FItemDataRow> UItemDataTableManager::GetItemsByQuality(EItemQualityTable Quality) const
{
    TArray<FItemDataRow> FilteredItems;
    
    if (!ItemDataTable)
    {
        return FilteredItems;
    }
    
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FItemDataRow* ItemData = ItemDataTable->FindRow<FItemDataRow>(RowName, TEXT(""));
        if (ItemData && ItemData->Quality == Quality)
        {
            FilteredItems.Add(*ItemData);
        }
    }
    
    return FilteredItems;
}

TArray<FName> UItemDataTableManager::GetAllItemRowNames() const
{
    if (!ItemDataTable)
    {
        return TArray<FName>();
    }
    
    return ItemDataTable->GetRowNames();
}

bool UItemDataTableManager::ItemBlocksShield(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    if (!ItemData)
    {
        return false;
    }
    
    return ItemData->BlocksShield();
}

int32 UItemDataTableManager::GetItemValue(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    if (!ItemData)
    {
        return 0;
    }
    
    return ItemData->BaseValue;
}

int32 UItemDataTableManager::GetModifiedItemValue(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    if (!ItemData)
    {
        return 0;
    }
    
    return ItemData->GetModifiedValue();
}

const FItemDataRow* UItemDataTableManager::FindItemByItemId(const FString& ItemId) const
{
    if (!ItemDataTable)
    {
        return nullptr;
    }
    
    // ItemIdをRowNameとして直接検索
    FItemDataRow* ItemData = ItemDataTable->FindRow<FItemDataRow>(FName(*ItemId), TEXT(""));
    return ItemData;
}

bool UItemDataTableManager::IsItemEquippable(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->IsEquippable() : false;
}

bool UItemDataTableManager::IsItemWeapon(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->IsWeapon() : false;
}

bool UItemDataTableManager::IsItemArmor(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->IsArmor() : false;
}

bool UItemDataTableManager::IsItemConsumable(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->IsConsumable() : false;
}

bool UItemDataTableManager::ItemHasEffect(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->HasEffect() : false;
}

bool UItemDataTableManager::IsItemInstant(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->IsInstant() : false;
}

int32 UItemDataTableManager::GetItemModifiedAttackPower(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->GetModifiedAttackPower() : 0;
}

int32 UItemDataTableManager::GetItemModifiedDefense(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->GetModifiedDefense() : 0;
}

int32 UItemDataTableManager::GetItemModifiedDurability(const FString& ItemId) const
{
    const FItemDataRow* ItemData = FindItemByItemId(ItemId);
    return ItemData ? ItemData->GetModifiedDurability() : 0;
}