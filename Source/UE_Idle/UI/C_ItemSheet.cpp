#include "C_ItemSheet.h"
#include "Engine/World.h"
#include "../Managers/ItemDataTableManager.h"

UC_ItemSheet::UC_ItemSheet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UC_ItemSheet::NativeConstruct()
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
        UpdateAllStats();
    }
}

void UC_ItemSheet::NativeDestruct()
{
    // Clean up cached item data
    if (CachedItemData)
    {
        delete CachedItemData;
        CachedItemData = nullptr;
    }
    
    Super::NativeDestruct();
}

void UC_ItemSheet::InitializeWithItem(const FString& InItemId)
{
    ItemId = InItemId;

    // Cache item data
    if (ItemManager)
    {
        FItemDataRow ItemData;
        if (ItemManager->GetItemData(ItemId, ItemData))
        {
            // Create a copy on heap for caching
            CachedItemData = new FItemDataRow(ItemData);
        }
        else
        {
            CachedItemData = nullptr;
        }
    }

    if (!CachedItemData)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_ItemSheet: Failed to get item data for %s"), *ItemId);
        return;
    }

    bIsInitialized = true;
    UpdateAllStats();
}

void UC_ItemSheet::UpdateAllStats()
{
    if (!bIsInitialized || !CachedItemData)
    {
        return;
    }

    UpdateBasicInfo();
    UpdateWeaponStats();
    UpdateArmorStats();
    UpdateConsumableStats();
    UpdateModifiedStats();
    UpdateVisibilityByItemType();
}

void UC_ItemSheet::UpdateBasicInfo()
{
    if (!CachedItemData)
    {
        return;
    }

    SetTextSafe(ItemNameText, CachedItemData->Name);
    SetTextSafe(ItemIdText, ItemId);
    SetTextSafe(DescriptionText, CachedItemData->Description);
    SetTextSafe(ItemTypeText, GetItemTypeDisplayName(CachedItemData->ItemType));
    SetTextSafe(EquipmentSlotText, GetEquipmentSlotDisplayName(CachedItemData->EquipmentSlot));
    SetTextSafe(QualityText, GetQualityDisplayName(CachedItemData->Quality));
    SetTextSafe(StackSizeText, CachedItemData->StackSize);
    SetTextSafe(WeightText, CachedItemData->Weight);
    SetTextSafe(BaseValueText, CachedItemData->BaseValue);
    SetTextSafe(TradeCategoryText, GetTradeCategoryDisplayName(CachedItemData->TradeCategory));
    SetTextSafe(MaxDurabilityText, CachedItemData->MaxDurability);
}

void UC_ItemSheet::UpdateWeaponStats()
{
    if (!CachedItemData || CachedItemData->ItemType != EItemTypeTable::Weapon)
    {
        return;
    }

    SetTextSafe(AttackPowerText, CachedItemData->AttackPower);
    SetTextSafe(RequiredSkillText, GetSkillTypeDisplayName(CachedItemData->RequiredSkill));
    SetTextSafe(CriticalBonusText, FormatPercentage(CachedItemData->CriticalBonus));
    SetTextSafe(AttackRangeText, CachedItemData->AttackRange);
    SetTextSafe(RequiredStrengthText, CachedItemData->RequiredStrength);
    SetTextSafe(RequiredDexterityText, CachedItemData->RequiredDexterity);
}

void UC_ItemSheet::UpdateArmorStats()
{
    if (!CachedItemData || CachedItemData->ItemType != EItemTypeTable::Armor)
    {
        return;
    }

    SetTextSafe(DefenseText, CachedItemData->Defense);
    // StatBonuses field doesn't exist in FItemDataRow, remove this line
    // SetTextSafe(StatBonusesText, FormatStatBonuses(CachedItemData->StatBonuses));
    SetTextSafe(ArmorRequiredStrengthText, CachedItemData->RequiredStrength);
}

void UC_ItemSheet::UpdateConsumableStats()
{
    if (!CachedItemData || CachedItemData->ItemType != EItemTypeTable::Consumable)
    {
        return;
    }

    // Only show non-zero values
    if (CachedItemData->Health != 0)
    {
        SetTextSafe(HealthEffectText, CachedItemData->Health);
    }
    else
    {
        SetTextSafe(HealthEffectText, TEXT(""));
    }

    if (CachedItemData->Stamina != 0)
    {
        SetTextSafe(StaminaEffectText, CachedItemData->Stamina);
    }
    else
    {
        SetTextSafe(StaminaEffectText, TEXT(""));
    }

    if (CachedItemData->Strength != 0)
    {
        SetTextSafe(StrengthEffectText, CachedItemData->Strength);
    }
    else
    {
        SetTextSafe(StrengthEffectText, TEXT(""));
    }

    if (CachedItemData->Agility != 0)
    {
        SetTextSafe(AgilityEffectText, CachedItemData->Agility);
    }
    else
    {
        SetTextSafe(AgilityEffectText, TEXT(""));
    }

    if (CachedItemData->Intelligence != 0)
    {
        SetTextSafe(IntelligenceEffectText, CachedItemData->Intelligence);
    }
    else
    {
        SetTextSafe(IntelligenceEffectText, TEXT(""));
    }

    if (CachedItemData->Dexterity != 0)
    {
        SetTextSafe(DexterityEffectText, CachedItemData->Dexterity);
    }
    else
    {
        SetTextSafe(DexterityEffectText, TEXT(""));
    }

    if (CachedItemData->Duration > 0.0f)
    {
        SetTextSafe(DurationText, FormatDuration(CachedItemData->Duration));
    }
    else
    {
        SetTextSafe(DurationText, TEXT("即座"));
    }

    SetTextSafe(CurePoisonText, CachedItemData->CurePoison ? TEXT("はい") : TEXT("いいえ"));
    SetTextSafe(CureBleedingText, CachedItemData->CureBleeding ? TEXT("はい") : TEXT("いいえ"));
}

void UC_ItemSheet::UpdateModifiedStats()
{
    if (!CachedItemData || !ItemManager)
    {
        return;
    }

    // Show modified values based on quality using ItemManager methods
    if (CachedItemData->ItemType == EItemTypeTable::Weapon)
    {
        int32 ModifiedAttack = ItemManager->GetItemModifiedAttackPower(ItemId);
        SetTextSafe(ModifiedAttackPowerText, ModifiedAttack);
    }

    if (CachedItemData->ItemType == EItemTypeTable::Armor)
    {
        int32 ModifiedDefense = ItemManager->GetItemModifiedDefense(ItemId);
        SetTextSafe(ModifiedDefenseText, ModifiedDefense);
    }

    int32 ModifiedDurability = ItemManager->GetItemModifiedDurability(ItemId);
    SetTextSafe(ModifiedDurabilityText, ModifiedDurability);

    int32 ModifiedValue = ItemManager->GetModifiedItemValue(ItemId);
    SetTextSafe(ModifiedValueText, ModifiedValue);
}

FString UC_ItemSheet::GetItemTypeDisplayName(EItemTypeTable ItemType) const
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
            return TEXT("クエストアイテム");
        case EItemTypeTable::Misc:
        default:
            return TEXT("その他");
    }
}

FString UC_ItemSheet::GetEquipmentSlotDisplayName(EEquipmentSlotTable SlotType) const
{
    switch (SlotType)
    {
        case EEquipmentSlotTable::None:
            return TEXT("装備不可");
        case EEquipmentSlotTable::Weapon:
            return TEXT("武器");
        case EEquipmentSlotTable::Shield:
            return TEXT("盾");
        case EEquipmentSlotTable::Head:
            return TEXT("頭");
        case EEquipmentSlotTable::Body:
            return TEXT("胴");
        case EEquipmentSlotTable::Legs:
            return TEXT("脚");
        case EEquipmentSlotTable::Hands:
            return TEXT("手");
        case EEquipmentSlotTable::Feet:
            return TEXT("足");
        case EEquipmentSlotTable::Accessory:
            return TEXT("アクセサリ");
        default:
            return TEXT("不明");
    }
}

FString UC_ItemSheet::GetQualityDisplayName(EItemQualityTable Quality) const
{
    switch (Quality)
    {
        case EItemQualityTable::Poor:
            return TEXT("粗悪");
        case EItemQualityTable::Common:
            return TEXT("普通");
        case EItemQualityTable::Good:
            return TEXT("良質");
        case EItemQualityTable::Masterwork:
            return TEXT("名匠");
        case EItemQualityTable::Legendary:
            return TEXT("伝説");
        default:
            return TEXT("不明");
    }
}

FString UC_ItemSheet::GetTradeCategoryDisplayName(ETradeCategoryTable Category) const
{
    switch (Category)
    {
        case ETradeCategoryTable::MeleeWeapons:
            return TEXT("近接武器");
        case ETradeCategoryTable::ModernWeapons:
            return TEXT("現代武器");
        case ETradeCategoryTable::Gems:
            return TEXT("宝石・貴金属");
        case ETradeCategoryTable::Antiques:
            return TEXT("骨董品・美術品");
        case ETradeCategoryTable::Electronics:
            return TEXT("電子機器");
        case ETradeCategoryTable::ModernGoods:
            return TEXT("現代日用品");
        case ETradeCategoryTable::Food:
            return TEXT("食品・調味料");
        case ETradeCategoryTable::MagicMaterials:
            return TEXT("魔法素材");
        case ETradeCategoryTable::MonsterMaterials:
            return TEXT("モンスター素材");
        case ETradeCategoryTable::Medicine:
            return TEXT("薬品・医療品");
        case ETradeCategoryTable::CommonMaterials:
            return TEXT("一般素材");
        case ETradeCategoryTable::Luxury:
            return TEXT("贅沢品");
        default:
            return TEXT("不明");
    }
}

FString UC_ItemSheet::GetSkillTypeDisplayName(ESkillType SkillType) const
{
    switch (SkillType)
    {
        case ESkillType::OneHandedWeapons:
            return TEXT("片手武器");
        case ESkillType::TwoHandedWeapons:
            return TEXT("両手武器");
        case ESkillType::PolearmWeapons:
            return TEXT("長柄武器");
        case ESkillType::Archery:
            return TEXT("弓術");
        case ESkillType::Firearms:
            return TEXT("銃器");
        case ESkillType::Throwing:
            return TEXT("投擲");
        case ESkillType::Shield:
            return TEXT("盾");
        case ESkillType::Lockpicking:
            return TEXT("解錠");
        case ESkillType::Swimming:
            return TEXT("水泳");
        case ESkillType::Engineering:
            return TEXT("工学");
        case ESkillType::Chemistry:
            return TEXT("化学");
        case ESkillType::Agriculture:
            return TEXT("農業");
        case ESkillType::Cooking:
            return TEXT("料理");
        case ESkillType::Tailoring:
            return TEXT("裁縫");
        case ESkillType::Construction:
            return TEXT("建築");
        case ESkillType::AnimalHandling:
            return TEXT("調教");
        case ESkillType::Survival:
            return TEXT("サバイバル");
        case ESkillType::Crafting:
            return TEXT("工作");
        case ESkillType::Mechanics:
            return TEXT("機械");
        default:
            return TEXT("不明");
    }
}

FString UC_ItemSheet::FormatStatBonuses(const TMap<FString, int32>& StatBonuses) const
{
    if (StatBonuses.Num() == 0)
    {
        return TEXT("なし");
    }

    TArray<FString> BonusStrings;
    for (const auto& Bonus : StatBonuses)
    {
        if (Bonus.Value != 0)
        {
            FString Sign = (Bonus.Value > 0) ? TEXT("+") : TEXT("");
            BonusStrings.Add(FString::Printf(TEXT("%s: %s%d"), *Bonus.Key, *Sign, Bonus.Value));
        }
    }

    return FString::Join(BonusStrings, TEXT(", "));
}

FString UC_ItemSheet::FormatDuration(float Duration) const
{
    if (Duration <= 0.0f)
    {
        return TEXT("即座");
    }

    int32 Minutes = FMath::FloorToInt(Duration / 60.0f);
    int32 Seconds = FMath::FloorToInt(Duration) % 60;

    if (Minutes > 0)
    {
        return FString::Printf(TEXT("%d分%d秒"), Minutes, Seconds);
    }
    else
    {
        return FString::Printf(TEXT("%d秒"), Seconds);
    }
}

FString UC_ItemSheet::FormatPercentage(float Value) const
{
    return FString::Printf(TEXT("%.1f%%"), Value * 100.0f);
}

void UC_ItemSheet::UpdateVisibilityByItemType()
{
    // This function could be used to show/hide UI sections based on item type
    // Implementation depends on the UI structure in Blueprint
    // For now, we'll just log the item type for debugging
    if (CachedItemData)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_ItemSheet: Displaying item type %s"), 
               *GetItemTypeDisplayName(CachedItemData->ItemType));
    }
}

void UC_ItemSheet::SetTextSafe(UTextBlock* TextBlock, const FString& Text)
{
    if (TextBlock)
    {
        TextBlock->SetText(FText::FromString(Text));
    }
}

void UC_ItemSheet::SetTextSafe(UTextBlock* TextBlock, const FText& Text)
{
    if (TextBlock)
    {
        TextBlock->SetText(Text);
    }
}

void UC_ItemSheet::SetTextSafe(UTextBlock* TextBlock, int32 Value)
{
    if (TextBlock)
    {
        TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d"), Value)));
    }
}

void UC_ItemSheet::SetTextSafe(UTextBlock* TextBlock, float Value, int32 DecimalPlaces)
{
    if (TextBlock)
    {
        FString Text;
        if (DecimalPlaces == 0)
        {
            Text = FString::Printf(TEXT("%.0f"), Value);
        }
        else if (DecimalPlaces == 1)
        {
            Text = FString::Printf(TEXT("%.1f"), Value);
        }
        else if (DecimalPlaces == 2)
        {
            Text = FString::Printf(TEXT("%.2f"), Value);
        }
        else
        {
            Text = FString::Printf(TEXT("%.3f"), Value);
        }
        TextBlock->SetText(FText::FromString(Text));
    }
}
