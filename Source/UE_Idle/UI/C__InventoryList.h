#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "../Types/ItemDataTable.h"
#include "../Types/ItemTypes.h"
#include "C__InventoryList.generated.h"

// Forward declarations
class UInventoryComponent;
class UC_ItemListCard;
class UItemDataTableManager;

UENUM(BlueprintType)
enum class EInventorySortType : uint8
{
    Name_Asc        UMETA(DisplayName = "名前（昇順）"),
    Name_Desc       UMETA(DisplayName = "名前（降順）"),
    Weight_Asc      UMETA(DisplayName = "重さ（軽い順）"),
    Weight_Desc     UMETA(DisplayName = "重さ（重い順）"),
    Quantity_Asc    UMETA(DisplayName = "個数（少ない順）"),
    Quantity_Desc   UMETA(DisplayName = "個数（多い順）"),
    Type_Asc        UMETA(DisplayName = "種類（昇順）"),
    Type_Desc       UMETA(DisplayName = "種類（降順）"),
    Value_Asc       UMETA(DisplayName = "価値（安い順）"),
    Value_Desc      UMETA(DisplayName = "価値（高い順）")
};

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC__InventoryList : public UUserWidget
{
    GENERATED_BODY()

public:
    UC__InventoryList(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // UI References
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    UPanelWidget* ItemCardPanel;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
    class UTextBlock* InventoryNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
    class UTextBlock* WeightDisplayText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
    class UComboBoxString* SortComboBox;

    // Inventory Management
    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    UInventoryComponent* CachedInventoryComponent;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> CachedInventorySlots;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TArray<UC_ItemListCard*> ItemCardWidgets;

    // Sort Settings
    UPROPERTY(BlueprintReadWrite, Category = "Sort")
    EInventorySortType CurrentSortType = EInventorySortType::Name_Asc;

public:
    // Main Functions
    UFUNCTION(BlueprintCallable, Category = "Inventory List")
    void InitializeWithInventory(UInventoryComponent* InInventoryComponent);

    UFUNCTION(BlueprintCallable, Category = "Inventory List")
    void RefreshInventoryList();

    UFUNCTION(BlueprintCallable, Category = "Inventory List")
    void UpdateItemCards();

    // Sorting Functions
    UFUNCTION(BlueprintCallable, Category = "Inventory List|Sort")
    void SetSortType(EInventorySortType NewSortType);

    UFUNCTION(BlueprintCallable, Category = "Inventory List|Sort")
    void SortInventory();

    // Manual refresh for Blueprint use
    UFUNCTION(BlueprintCallable, Category = "Inventory List")
    void ForceRefresh();

    // Weight Display
    UFUNCTION(BlueprintCallable, Category = "Inventory List")
    void UpdateWeightDisplay();

    // Inventory Name Display
    UFUNCTION(BlueprintCallable, Category = "Inventory List")
    void UpdateInventoryName();

    // Filter Functions (for future expansion)
    UFUNCTION(BlueprintCallable, Category = "Inventory List|Filter")
    void FilterByType(EItemTypeTable ItemType);

    UFUNCTION(BlueprintCallable, Category = "Inventory List|Filter")
    void ClearFilter();

    // Sort ComboBox Functions
    UFUNCTION(BlueprintCallable, Category = "Inventory List|Sort")
    void InitializeSortComboBox();

    UFUNCTION()
    void OnSortComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

protected:
    // Internal Functions
    UFUNCTION()
    void CreateItemCard(const FInventorySlot& InventorySlot);

    UFUNCTION()
    void ClearItemCards();

    // Inventory event handlers
    UFUNCTION()
    void OnInventoryItemChanged(const FString& ItemId, int32 NewQuantity);

    // Sort comparison functions
    bool CompareByName(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const;
    bool CompareByWeight(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const;
    bool CompareByQuantity(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const;
    bool CompareByType(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const;
    bool CompareByValue(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const;

    // Helper Functions
    FItemDataRow* GetItemData(const FString& ItemId) const;

    // Bind/unbind inventory events
    void BindInventoryEvents();
    void UnbindInventoryEvents();

private:
    // Item Card Widget Class
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget Classes", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UC_ItemListCard> ItemCardWidgetClass;

    // Filter state
    bool bIsFiltered = false;
    EItemTypeTable FilterType = EItemTypeTable::Misc;

    // ItemDataTableManager reference
    UPROPERTY()
    class UItemDataTableManager* ItemManager;
};