#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "../Types/ItemTypes.h"
#include "../Types/ItemDataTable.h"
#include "../Types/CharacterTypes.h"
#include "C_ItemSheet.generated.h"

// Forward declarations
class UItemDataTableManager;

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_ItemSheet : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_ItemSheet(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Basic Info - All optional (BindWidgetOptional)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* ItemNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* ItemIdText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* DescriptionText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* ItemTypeText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* EquipmentSlotText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* QualityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* StackSizeText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* WeightText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* BaseValueText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* TradeCategoryText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Basic")
    UTextBlock* MaxDurabilityText;

    // Weapon Stats
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Weapon")
    UTextBlock* AttackPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Weapon")
    UTextBlock* RequiredSkillText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Weapon")
    UTextBlock* CriticalBonusText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Weapon")
    UTextBlock* AttackRangeText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Weapon")
    UTextBlock* RequiredStrengthText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Weapon")
    UTextBlock* RequiredDexterityText;

    // Armor Stats
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Armor")
    UTextBlock* DefenseText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Armor")
    UTextBlock* StatBonusesText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Armor")
    UTextBlock* ArmorRequiredStrengthText;

    // Consumable Stats
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* HealthEffectText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* StaminaEffectText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* StrengthEffectText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* AgilityEffectText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* IntelligenceEffectText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* DexterityEffectText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* DurationText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* CurePoisonText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Consumable")
    UTextBlock* CureBleedingText;

    // Quality Modified Stats (calculated values)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Modified")
    UTextBlock* ModifiedAttackPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Modified")
    UTextBlock* ModifiedDefenseText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Modified")
    UTextBlock* ModifiedDurabilityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Modified")
    UTextBlock* ModifiedValueText;

    // Item Data
    UPROPERTY(BlueprintReadOnly, Category = "Item", meta = (ExposeOnSpawn = "true"))
    FString ItemId;

    // Cache item data internally (not exposed to Blueprint)
    FItemDataRow* CachedItemData;

public:
    // Initialization - called once when sheet is created
    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void InitializeWithItem(const FString& InItemId);

    // Update display with current item data
    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void UpdateAllStats();

    // Get item ID for this sheet
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Sheet")
    FString GetItemId() const { return ItemId; }

protected:
    // Update Functions
    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void UpdateBasicInfo();

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void UpdateWeaponStats();

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void UpdateArmorStats();

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void UpdateConsumableStats();

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    void UpdateModifiedStats();

    // Display Name Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    FString GetItemTypeDisplayName(EItemTypeTable ItemType) const;

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    FString GetEquipmentSlotDisplayName(EEquipmentSlotTable SlotType) const;

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    FString GetQualityDisplayName(EItemQualityTable Quality) const;

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    FString GetTradeCategoryDisplayName(ETradeCategoryTable Category) const;

    UFUNCTION(BlueprintCallable, Category = "Item Sheet")
    FString GetSkillTypeDisplayName(ESkillType SkillType) const;

    // Format helper functions
    FString FormatStatBonuses(const TMap<FString, int32>& StatBonuses) const;
    FString FormatDuration(float Duration) const;
    FString FormatPercentage(float Value) const;

private:
    // Internal state
    bool bIsInitialized = false;

    // ItemDataTableManager reference
    UPROPERTY()
    UItemDataTableManager* ItemManager;

    // Update text safely (checks for null)
    void SetTextSafe(UTextBlock* TextBlock, const FString& Text);
    void SetTextSafe(UTextBlock* TextBlock, const FText& Text);
    void SetTextSafe(UTextBlock* TextBlock, int32 Value);
    void SetTextSafe(UTextBlock* TextBlock, float Value, int32 DecimalPlaces = 1);

    // Helper to show/hide sections based on item type
    void UpdateVisibilityByItemType();
};
