#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "../Types/ItemTypes.h"
#include "../Types/ItemDataTable.h"
#include "C_ItemListCard.generated.h"

// Forward declarations
class UInventoryComponent;
class UItemDataTableManager;

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_ItemListCard : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_ItemListCard(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Text Display References - Not required (BindWidgetOptional)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Text")
    UTextBlock* ItemNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Text")
    UTextBlock* DurabilityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Text")
    UTextBlock* QuantityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Text")
    UTextBlock* ItemTypeText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Text")
    UTextBlock* WeightText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Text")
    UTextBlock* ValueText;

    // Item Data
    UPROPERTY(BlueprintReadOnly, Category = "Item", meta = (ExposeOnSpawn = "true"))
    FInventorySlot ItemSlot;

    // Cache item data internally (not exposed to Blueprint)
    FItemDataRow* CachedItemData;

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    UInventoryComponent* InventoryComponent;

public:
    // Initialization - called once when card is created
    UFUNCTION(BlueprintCallable, Category = "Item Card")
    void InitializeWithSlot(const FInventorySlot& InSlot, UInventoryComponent* InInventoryComponent);

    // Update display with current item data
    UFUNCTION(BlueprintCallable, Category = "Item Card")
    void UpdateDisplay();

    // Get item ID for this card
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Card")
    FString GetItemId() const { return ItemSlot.ItemId; }

    // Get quantity for this card
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Card")
    int32 GetQuantity() const { return ItemSlot.Quantity; }

    // Check if this card represents the given item
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Card")
    bool IsItemCard(const FString& TestItemId) const { return ItemSlot.ItemId == TestItemId; }

    // Get formatted durability string
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Card")
    FString GetDurabilityDisplayString() const;

    // Get total weight
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Card")
    float GetTotalWeight() const;

    // Get total value
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Card")
    int32 GetTotalValue() const;

protected:
    // Internal Update Functions
    void UpdateItemName();
    void UpdateDurability();
    void UpdateQuantity();
    void UpdateItemType();
    void UpdateWeight();
    void UpdateValue();

    // Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Item Card")
    FString GetItemTypeDisplayName(EItemTypeTable ItemType) const;

    UFUNCTION(BlueprintCallable, Category = "Item Card")
    FString GetQualityDisplayName(EItemQualityTable Quality) const;

    // Get average durability for display
    float GetAverageDurability() const;
    
    // Get min/max durability for range display
    void GetDurabilityRange(int32& OutMin, int32& OutMax) const;

private:
    // Internal state tracking
    bool bIsInitialized = false;

    // ItemDataTableManager reference
    UPROPERTY()
    UItemDataTableManager* ItemManager;

    // Update text safely (checks for null)
    void SetTextSafe(UTextBlock* TextBlock, const FString& Text);
    void SetTextSafe(UTextBlock* TextBlock, const FText& Text);
};