#include "C_ItemListCard.h"
#include "Engine/World.h"
#include "../Managers/ItemDataTableManager.h"
#include "../Components/InventoryComponent.h"

UC_ItemListCard::UC_ItemListCard(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UC_ItemListCard::NativeConstruct()
{
    Super::NativeConstruct();

    // Get ItemDataTableManager
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
    }

    // Initial update if already initialized
    if (bIsInitialized)
    {
        UpdateDisplay();
    }
}

void UC_ItemListCard::NativeDestruct()
{
    // Clean up cached item data
    if (CachedItemData)
    {
        delete CachedItemData;
        CachedItemData = nullptr;
    }
    
    Super::NativeDestruct();
}

void UC_ItemListCard::InitializeWithSlot(const FInventorySlot& InSlot, UInventoryComponent* InInventoryComponent)
{
    UE_LOG(LogTemp, Log, TEXT("ItemListCard::InitializeWithSlot - ItemId=%s, Quantity=%d"), *InSlot.ItemId, InSlot.Quantity);
    
    ItemSlot = InSlot;
    InventoryComponent = InInventoryComponent;

    // Ensure ItemManager is available
    if (!ItemManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemListCard::InitializeWithSlot - ItemManager is null, attempting to get it..."));
        UWorld* World = GetWorld();
        if (World)
        {
            UGameInstance* GameInstance = World->GetGameInstance();
            if (GameInstance)
            {
                ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
                UE_LOG(LogTemp, Log, TEXT("ItemListCard::InitializeWithSlot - World: Valid, GameInstance: Valid, ItemManager retrieved: %s"), ItemManager ? TEXT("Success") : TEXT("Failed"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("ItemListCard::InitializeWithSlot - GameInstance is null"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ItemListCard::InitializeWithSlot - World is null"));
        }
    }

    // Cache item data
    if (ItemManager)
    {
        UE_LOG(LogTemp, Log, TEXT("ItemListCard::InitializeWithSlot - ItemManager is valid, attempting to get data for %s"), *ItemSlot.ItemId);
        FItemDataRow ItemData;
        if (ItemManager->GetItemData(ItemSlot.ItemId, ItemData))
        {
            // Create a copy on heap for caching
            CachedItemData = new FItemDataRow(ItemData);
            UE_LOG(LogTemp, Log, TEXT("ItemListCard::InitializeWithSlot - Successfully cached item data for %s"), *ItemData.Name.ToString());
        }
        else
        {
            CachedItemData = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("ItemListCard::InitializeWithSlot - Failed to get item data for %s - DataTable might not be set"), *ItemSlot.ItemId);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ItemListCard::InitializeWithSlot - ItemManager is still null after retry"));
    }

    bIsInitialized = true;
    UpdateDisplay();
}

void UC_ItemListCard::UpdateDisplay()
{
    UE_LOG(LogTemp, Log, TEXT("ItemListCard::UpdateDisplay - Initialized=%s, CachedItemData=%s"), 
           bIsInitialized ? TEXT("True") : TEXT("False"),
           CachedItemData ? TEXT("Valid") : TEXT("Null"));
           
    if (!bIsInitialized || !CachedItemData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemListCard::UpdateDisplay - Skipping update due to invalid state"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ItemListCard::UpdateDisplay - Updating display for %s"), *CachedItemData->Name.ToString());
    
    UpdateItemName();
    UpdateDurability();
    UpdateQuantity();
    UpdateItemType();
    UpdateWeight();
    UpdateValue();
    
    UE_LOG(LogTemp, Log, TEXT("ItemListCard::UpdateDisplay - Update complete"));
}

void UC_ItemListCard::UpdateItemName()
{
    if (!CachedItemData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemListCard::UpdateItemName - CachedItemData is null"));
        return;
    }

    // Include quality in name if not common
    FString DisplayName = CachedItemData->Name.ToString();
    if (CachedItemData->Quality != EItemQualityTable::Common)
    {
        FString QualityName = GetQualityDisplayName(CachedItemData->Quality);
        DisplayName = FString::Printf(TEXT("%s %s"), *QualityName, *DisplayName);
    }

    UE_LOG(LogTemp, Log, TEXT("ItemListCard::UpdateItemName - Setting name to '%s', Widget exists: %s"), 
           *DisplayName, ItemNameText ? TEXT("True") : TEXT("False"));
    
    SetTextSafe(ItemNameText, DisplayName);
}

void UC_ItemListCard::UpdateDurability()
{
    // Only show durability for items with durability (StackSize == 1)
    if (CachedItemData && CachedItemData->StackSize == 1 && ItemSlot.ItemInstances.Num() > 0)
    {
        FString DurabilityString = GetDurabilityDisplayString();
        SetTextSafe(DurabilityText, DurabilityString);
    }
    else
    {
        SetTextSafe(DurabilityText, TEXT(""));
    }
}

void UC_ItemListCard::UpdateQuantity()
{
    FString QuantityString = FString::Printf(TEXT("x%d"), ItemSlot.Quantity);
    SetTextSafe(QuantityText, QuantityString);
}

void UC_ItemListCard::UpdateItemType()
{
    if (!CachedItemData)
    {
        return;
    }

    FString TypeName = GetItemTypeDisplayName(CachedItemData->ItemType);
    SetTextSafe(ItemTypeText, TypeName);
}

void UC_ItemListCard::UpdateWeight()
{
    float TotalWeight = GetTotalWeight();
    FString WeightString = FString::Printf(TEXT("%.1f kg"), TotalWeight);
    SetTextSafe(WeightText, WeightString);
}

void UC_ItemListCard::UpdateValue()
{
    int32 TotalValue = GetTotalValue();
    FString ValueString = FString::Printf(TEXT("%d G"), TotalValue);
    SetTextSafe(ValueText, ValueString);
}

FString UC_ItemListCard::GetDurabilityDisplayString() const
{
    if (!CachedItemData || CachedItemData->StackSize != 1)
    {
        return TEXT("");
    }

    if (ItemSlot.ItemInstances.Num() == 0)
    {
        return TEXT("");
    }

    // If single item, show exact durability
    if (ItemSlot.ItemInstances.Num() == 1)
    {
        int32 Current = ItemSlot.ItemInstances[0].CurrentDurability;
        int32 Max = CachedItemData->MaxDurability;
        return FString::Printf(TEXT("%d/%d"), Current, Max);
    }

    // If multiple items, show range
    int32 MinDurability, MaxDurability;
    GetDurabilityRange(MinDurability, MaxDurability);
    
    if (MinDurability == MaxDurability)
    {
        return FString::Printf(TEXT("%d/%d"), MinDurability, CachedItemData->MaxDurability);
    }
    else
    {
        return FString::Printf(TEXT("%d-%d/%d"), MinDurability, MaxDurability, CachedItemData->MaxDurability);
    }
}

float UC_ItemListCard::GetTotalWeight() const
{
    if (!CachedItemData)
    {
        return 0.0f;
    }

    return CachedItemData->Weight * ItemSlot.Quantity;
}

int32 UC_ItemListCard::GetTotalValue() const
{
    if (!CachedItemData)
    {
        return 0;
    }

    // For stackable items, simple multiplication
    if (CachedItemData->StackSize > 1)
    {
        return CachedItemData->BaseValue * ItemSlot.Quantity;
    }

    // For non-stackable items, consider durability
    int32 TotalValue = 0;
    for (const FItemInstance& Instance : ItemSlot.ItemInstances)
    {
        float DurabilityRatio = (float)Instance.CurrentDurability / (float)CachedItemData->MaxDurability;
        TotalValue += FMath::RoundToInt(CachedItemData->BaseValue * DurabilityRatio);
    }

    return TotalValue;
}

FString UC_ItemListCard::GetItemTypeDisplayName(EItemTypeTable ItemType) const
{
    switch (ItemType)
    {
        case EItemTypeTable::Weapon:
            return TEXT("武器");
        case EItemTypeTable::Armor:
            return TEXT("防具");
        case EItemTypeTable::Consumable:
            return TEXT("消耗品");
        case EItemTypeTable::Material:
            return TEXT("素材");
        case EItemTypeTable::Quest:
            return TEXT("クエスト");
        case EItemTypeTable::Misc:
        default:
            return TEXT("その他");
    }
}

FString UC_ItemListCard::GetQualityDisplayName(EItemQualityTable Quality) const
{
    switch (Quality)
    {
        case EItemQualityTable::Poor:
            return TEXT("粗悪な");
        case EItemQualityTable::Common:
            return TEXT("");
        case EItemQualityTable::Good:
            return TEXT("良質な");
        case EItemQualityTable::Masterwork:
            return TEXT("名匠の");
        case EItemQualityTable::Legendary:
            return TEXT("伝説の");
        default:
            return TEXT("");
    }
}

float UC_ItemListCard::GetAverageDurability() const
{
    if (ItemSlot.ItemInstances.Num() == 0)
    {
        return 0.0f;
    }

    int32 TotalDurability = 0;
    for (const FItemInstance& Instance : ItemSlot.ItemInstances)
    {
        TotalDurability += Instance.CurrentDurability;
    }

    return (float)TotalDurability / (float)ItemSlot.ItemInstances.Num();
}

void UC_ItemListCard::GetDurabilityRange(int32& OutMin, int32& OutMax) const
{
    if (ItemSlot.ItemInstances.Num() == 0)
    {
        OutMin = OutMax = 0;
        return;
    }

    OutMin = ItemSlot.ItemInstances[0].CurrentDurability;
    OutMax = ItemSlot.ItemInstances[0].CurrentDurability;

    for (int32 i = 1; i < ItemSlot.ItemInstances.Num(); i++)
    {
        int32 Durability = ItemSlot.ItemInstances[i].CurrentDurability;
        OutMin = FMath::Min(OutMin, Durability);
        OutMax = FMath::Max(OutMax, Durability);
    }
}

void UC_ItemListCard::SetTextSafe(UTextBlock* TextBlock, const FString& Text)
{
    if (TextBlock)
    {
        TextBlock->SetText(FText::FromString(Text));
    }
}

void UC_ItemListCard::SetTextSafe(UTextBlock* TextBlock, const FText& Text)
{
    if (TextBlock)
    {
        TextBlock->SetText(Text);
    }
}