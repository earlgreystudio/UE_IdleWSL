#include "GlobalInventoryComponent.h"
#include "UE_Idle/Managers/ItemDataTableManager.h"
#include "UE_Idle/Components/CharacterInventoryComponent.h"
#include "Engine/Engine.h"

UGlobalInventoryComponent::UGlobalInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGlobalInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("GlobalInventoryComponent: BeginPlay started"));
    
    // Get reference to ItemDataTableManager
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        if (ItemManager)
        {
            UE_LOG(LogTemp, Warning, TEXT("GlobalInventoryComponent: ItemDataTableManager found successfully"));
        }
    }
    
    if (!ItemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GlobalInventoryComponent: ItemDataTableManager NOT FOUND!"));
    }
}

bool UGlobalInventoryComponent::AddItemToStorage(const FString& ItemId, int32 Quantity)
{
    UE_LOG(LogTemp, Warning, TEXT("AddItemToStorage: Start - ItemId: %s, Quantity: %d"), *ItemId, Quantity);
    
    if (!ItemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("AddItemToStorage: ItemManager is NULL"));
        return false;
    }
    
    if (!ItemManager->IsValidItem(ItemId))
    {
        UE_LOG(LogTemp, Error, TEXT("AddItemToStorage: Invalid ItemId: %s"), *ItemId);
        return false;
    }
    
    if (Quantity <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("AddItemToStorage: Invalid Quantity: %d"), Quantity);
        return false;
    }

    FInventorySlot* Slot = GetOrCreateStorageSlot(ItemId);
    if (!Slot)
    {
        UE_LOG(LogTemp, Error, TEXT("AddItemToStorage: Failed to create storage slot"));
        return false;
    }

    // Get item data for stack size validation
    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        UE_LOG(LogTemp, Error, TEXT("AddItemToStorage: Failed to get item data for: %s"), *ItemId);
        return false;
    }

    if (ItemData.StackSize > 1)
    {
        // Stackable item - just add quantity
        Slot->Quantity += Quantity;
    }
    else
    {
        // Non-stackable item - create individual instances
        for (int32 i = 0; i < Quantity; i++)
        {
            FItemInstance NewInstance;
            NewInstance.InstanceId = FGuid::NewGuid();
            NewInstance.ItemId = ItemId;
            NewInstance.CurrentDurability = ItemData.MaxDurability;
            
            Slot->ItemInstances.Add(NewInstance);
            Slot->Quantity++;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("GlobalInventory: Successfully added %d %s to storage. Total in slot: %d"), Quantity, *ItemId, Slot->Quantity);
    
    // Fire event
    OnStorageChanged.Broadcast(ItemId, Slot->Quantity);
    
    return true;
}

bool UGlobalInventoryComponent::RemoveItemFromStorage(const FString& ItemId, int32 Quantity)
{
    if (!ItemManager || !ItemManager->IsValidItem(ItemId) || Quantity <= 0)
    {
        return false;
    }

    FInventorySlot* Slot = GlobalStorage.Find(ItemId);
    if (!Slot || Slot->Quantity < Quantity)
    {
        return false;
    }

    bool bSuccess = RemoveItemInstance(*Slot, Quantity);
    
    if (bSuccess)
    {
        // Get updated quantity (might be 0 if slot was removed)
        const FInventorySlot* UpdatedSlot = GlobalStorage.Find(ItemId);
        int32 NewQuantity = UpdatedSlot ? UpdatedSlot->Quantity : 0;
        OnStorageChanged.Broadcast(ItemId, NewQuantity);
    }
    
    return bSuccess;
}

bool UGlobalInventoryComponent::HasItemInStorage(const FString& ItemId, int32 RequiredQuantity) const
{
    const FInventorySlot* Slot = GlobalStorage.Find(ItemId);
    return Slot && Slot->Quantity >= RequiredQuantity;
}

int32 UGlobalInventoryComponent::GetItemQuantityInStorage(const FString& ItemId) const
{
    const FInventorySlot* Slot = GlobalStorage.Find(ItemId);
    return Slot ? Slot->Quantity : 0;
}

bool UGlobalInventoryComponent::TransferToCharacter(UCharacterInventoryComponent* CharacterInventory, const FString& ItemId, int32 Quantity)
{
    UE_LOG(LogTemp, Warning, TEXT("TransferToCharacter: Start - ItemId: %s, Quantity: %d"), *ItemId, Quantity);
    
    if (!CharacterInventory)
    {
        UE_LOG(LogTemp, Error, TEXT("TransferToCharacter: CharacterInventory is NULL"));
        return false;
    }
    
    if (!HasItemInStorage(ItemId, Quantity))
    {
        UE_LOG(LogTemp, Error, TEXT("TransferToCharacter: Not enough items in storage. ItemId: %s, Required: %d, Available: %d"), 
            *ItemId, Quantity, GetItemQuantityInStorage(ItemId));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("TransferToCharacter: Removing %d %s from storage"), Quantity, *ItemId);
    
    // Remove from global storage
    if (!RemoveItemFromStorage(ItemId, Quantity))
    {
        UE_LOG(LogTemp, Error, TEXT("TransferToCharacter: Failed to remove items from storage"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("TransferToCharacter: Adding %d %s to character inventory"), Quantity, *ItemId);
    
    // Add to character inventory
    if (!CharacterInventory->AddItem(ItemId, Quantity))
    {
        UE_LOG(LogTemp, Error, TEXT("TransferToCharacter: Failed to add items to character inventory, rolling back"));
        // Rollback: add back to storage
        AddItemToStorage(ItemId, Quantity);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Successfully transferred %d %s to character"), Quantity, *ItemId);
    return true;
}

bool UGlobalInventoryComponent::TransferFromCharacter(UCharacterInventoryComponent* CharacterInventory, const FString& ItemId, int32 Quantity)
{
    if (!CharacterInventory || !CharacterInventory->HasItem(ItemId, Quantity))
    {
        return false;
    }

    // Remove from character inventory
    if (!CharacterInventory->RemoveItem(ItemId, Quantity))
    {
        return false;
    }

    // Add to global storage
    if (!AddItemToStorage(ItemId, Quantity))
    {
        // Rollback: add back to character
        CharacterInventory->AddItem(ItemId, Quantity);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Transferred %d %s from character"), Quantity, *ItemId);
    return true;
}

TArray<FString> UGlobalInventoryComponent::GetAllStoredItemIds() const
{
    TArray<FString> ItemIds;
    GlobalStorage.GetKeys(ItemIds);
    return ItemIds;
}

TMap<FString, int32> UGlobalInventoryComponent::GetAllStoredItems() const
{
    TMap<FString, int32> Result;
    for (const auto& Pair : GlobalStorage)
    {
        Result.Add(Pair.Key, Pair.Value.Quantity);
    }
    return Result;
}

TArray<FInventorySlot> UGlobalInventoryComponent::GetAllStorageSlots() const
{
    TArray<FInventorySlot> Result;
    for (const auto& Pair : GlobalStorage)
    {
        const FInventorySlot& StorageSlot = Pair.Value;
        
        // Get item data to check if it's stackable
        if (ItemManager)
        {
            FItemDataRow ItemData;
            if (ItemManager->GetItemData(StorageSlot.ItemId, ItemData) && ItemData.StackSize == 1)
            {
                // Non-stackable items (like weapons/armor) - create separate slot for each durability
                TMap<int32, FInventorySlot> DurabilitySlots;
                
                for (const FItemInstance& Instance : StorageSlot.ItemInstances)
                {
                    int32 Durability = Instance.CurrentDurability;
                    
                    if (!DurabilitySlots.Contains(Durability))
                    {
                        FInventorySlot NewSlot;
                        NewSlot.ItemId = StorageSlot.ItemId;
                        NewSlot.Quantity = 0;
                        DurabilitySlots.Add(Durability, NewSlot);
                    }
                    
                    DurabilitySlots[Durability].ItemInstances.Add(Instance);
                    DurabilitySlots[Durability].Quantity++;
                }
                
                // Add all durability-based slots to result
                for (const auto& DurabilityPair : DurabilitySlots)
                {
                    Result.Add(DurabilityPair.Value);
                }
            }
            else
            {
                // Stackable items - add as single slot
                Result.Add(StorageSlot);
            }
        }
        else
        {
            // Fallback if ItemManager is not available
            Result.Add(StorageSlot);
        }
    }
    return Result;
}

void UGlobalInventoryComponent::ClearStorage()
{
    GlobalStorage.Empty();
    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Storage cleared"));
}

bool UGlobalInventoryComponent::AddMoney(int32 Amount)
{
    if (Amount < 0)
    {
        return false;
    }

    GlobalMoney += Amount;
    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Added %d money. Total: %d"), Amount, GlobalMoney);
    
    // Fire event
    OnMoneyChanged.Broadcast(GlobalMoney);
    
    return true;
}

bool UGlobalInventoryComponent::SpendMoney(int32 Amount)
{
    if (Amount < 0 || GlobalMoney < Amount)
    {
        return false;
    }

    GlobalMoney -= Amount;
    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Spent %d money. Remaining: %d"), Amount, GlobalMoney);
    
    // Fire event
    OnMoneyChanged.Broadcast(GlobalMoney);
    
    return true;
}

bool UGlobalInventoryComponent::BuyItem(const FString& ItemId, int32 Quantity)
{
    if (!ItemManager || !ItemManager->IsValidItem(ItemId))
    {
        return false;
    }

    int32 TotalCost = GetItemBuyPrice(ItemId, Quantity);
    if (!SpendMoney(TotalCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("GlobalInventory: Not enough money to buy %d %s (Cost: %d, Have: %d)"), 
            Quantity, *ItemId, TotalCost, GlobalMoney);
        return false;
    }

    if (!AddItemToStorage(ItemId, Quantity))
    {
        // Rollback money
        AddMoney(TotalCost);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Bought %d %s for %d money"), Quantity, *ItemId, TotalCost);
    return true;
}

bool UGlobalInventoryComponent::SellItem(const FString& ItemId, int32 Quantity)
{
    if (!HasItemInStorage(ItemId, Quantity))
    {
        return false;
    }

    int32 SellValue = GetItemSellPrice(ItemId, Quantity);
    
    if (!RemoveItemFromStorage(ItemId, Quantity))
    {
        return false;
    }

    AddMoney(SellValue);
    UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Sold %d %s for %d money"), Quantity, *ItemId, SellValue);
    return true;
}

int32 UGlobalInventoryComponent::GetItemBuyPrice(const FString& ItemId, int32 Quantity) const
{
    if (!ItemManager)
    {
        return 0;
    }

    return ItemManager->GetItemValue(ItemId) * Quantity;
}

int32 UGlobalInventoryComponent::GetItemSellPrice(const FString& ItemId, int32 Quantity) const
{
    if (!ItemManager)
    {
        return 0;
    }

    // Sell for 80% of buy price
    return FMath::RoundToInt(ItemManager->GetItemValue(ItemId) * Quantity * 0.8f);
}

bool UGlobalInventoryComponent::SellItemInstance(const FGuid& InstanceId)
{
    FItemInstance* Instance = FindItemInstance(InstanceId);
    if (!Instance)
    {
        UE_LOG(LogTemp, Warning, TEXT("GlobalInventory: Item instance not found for selling: %s"), *InstanceId.ToString());
        return false;
    }

    int32 SellValue = GetItemInstanceSellPrice(InstanceId);
    
    // Find and remove the instance from storage
    for (auto& Pair : GlobalStorage)
    {
        FInventorySlot& Slot = Pair.Value;
        for (int32 i = 0; i < Slot.ItemInstances.Num(); i++)
        {
            if (Slot.ItemInstances[i].InstanceId == InstanceId)
            {
                Slot.ItemInstances.RemoveAt(i);
                Slot.Quantity--;
                
                // Remove slot if empty
                if (Slot.Quantity <= 0)
                {
                    GlobalStorage.Remove(Pair.Key);
                }
                
                AddMoney(SellValue);
                OnStorageChanged.Broadcast(Instance->ItemId, Slot.Quantity);
                
                UE_LOG(LogTemp, Log, TEXT("GlobalInventory: Sold item instance %s for %d money"), *Instance->ItemId, SellValue);
                return true;
            }
        }
    }
    
    return false;
}

int32 UGlobalInventoryComponent::GetItemInstanceSellPrice(const FGuid& InstanceId) const
{
    const FItemInstance* Instance = FindItemInstance(InstanceId);
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

FInventorySlot* UGlobalInventoryComponent::GetOrCreateStorageSlot(const FString& ItemId)
{
    if (FInventorySlot* ExistingSlot = GlobalStorage.Find(ItemId))
    {
        return ExistingSlot;
    }

    // Create new slot
    FInventorySlot NewSlot;
    NewSlot.ItemId = ItemId;
    NewSlot.Quantity = 0;
    
    return &GlobalStorage.Add(ItemId, NewSlot);
}

FItemInstance* UGlobalInventoryComponent::FindItemInstance(const FGuid& InstanceId)
{
    for (auto& Pair : GlobalStorage)
    {
        FInventorySlot& Slot = Pair.Value;
        for (FItemInstance& Instance : Slot.ItemInstances)
        {
            if (Instance.InstanceId == InstanceId)
            {
                return &Instance;
            }
        }
    }
    return nullptr;
}

const FItemInstance* UGlobalInventoryComponent::FindItemInstance(const FGuid& InstanceId) const
{
    for (const auto& Pair : GlobalStorage)
    {
        const FInventorySlot& Slot = Pair.Value;
        for (const FItemInstance& Instance : Slot.ItemInstances)
        {
            if (Instance.InstanceId == InstanceId)
            {
                return &Instance;
            }
        }
    }
    return nullptr;
}

bool UGlobalInventoryComponent::RemoveItemInstance(FInventorySlot& Slot, int32 Quantity)
{
    if (Slot.Quantity < Quantity)
    {
        return false;
    }

    // Get item data to check if stackable
    if (ItemManager)
    {
        FItemDataRow ItemData;
        if (ItemManager->GetItemData(Slot.ItemId, ItemData))
        {
            if (ItemData.StackSize > 1)
            {
                // Stackable item - just reduce quantity
                Slot.Quantity -= Quantity;
            }
            else
            {
                // Non-stackable item - remove instances
                for (int32 i = 0; i < Quantity && Slot.ItemInstances.Num() > 0; i++)
                {
                    Slot.ItemInstances.RemoveAt(Slot.ItemInstances.Num() - 1);
                    Slot.Quantity--;
                }
            }
        }
    }

    // Remove slot if empty
    if (Slot.Quantity <= 0)
    {
        GlobalStorage.Remove(Slot.ItemId);
    }

    return true;
}