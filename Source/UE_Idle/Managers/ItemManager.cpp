#include "ItemManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

void UItemManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadItemsFromJSON();
}

void UItemManager::Deinitialize()
{
    WeaponDatabase.Empty();
    ArmorDatabase.Empty();
    ConsumableDatabase.Empty();
    ItemDatabase.Empty();
    
    Super::Deinitialize();
}

void UItemManager::LoadItemsFromJSON()
{
    FString ProjectDir = FPaths::ProjectDir();
    
    // Try to load unified items first
    LoadUnifiedItemsFromJSON(ProjectDir + TEXT("Content/Data/items.json"));
    
    // Fallback to legacy files if unified file doesn't exist
    if (UnifiedItemDatabase.Num() == 0)
    {
        LoadWeaponsFromJSON(ProjectDir + TEXT("Content/Data/weapons.json"));
        LoadArmorsFromJSON(ProjectDir + TEXT("Content/Data/armors.json"));
        LoadConsumablesFromJSON(ProjectDir + TEXT("Content/Data/consumables.json"));
        
        UE_LOG(LogTemp, Log, TEXT("ItemManager: Loaded %d weapons, %d armors, %d consumables"), 
            WeaponDatabase.Num(), ArmorDatabase.Num(), ConsumableDatabase.Num());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ItemManager: Loaded %d unified items"), UnifiedItemDatabase.Num());
    }
}

void UItemManager::LoadUnifiedItemsFromJSON(const FString& FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load unified items JSON from: %s"), *FilePath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse unified items JSON"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
    if (!JsonObject->TryGetArrayField(TEXT("items"), ItemsArray))
    {
        return;
    }

    for (const auto& ItemValue : *ItemsArray)
    {
        const TSharedPtr<FJsonObject>* ItemObject;
        if (!ItemValue->TryGetObject(ItemObject))
        {
            continue;
        }

        FItemData Item;
        
        (*ItemObject)->TryGetStringField(TEXT("id"), Item.ItemId);
        
        FString Name;
        (*ItemObject)->TryGetStringField(TEXT("name"), Name);
        Item.ItemName = FText::FromString(Name);
        
        FString Description;
        (*ItemObject)->TryGetStringField(TEXT("description"), Description);
        Item.Description = FText::FromString(Description);
        
        FString ItemTypeStr;
        if ((*ItemObject)->TryGetStringField(TEXT("itemType"), ItemTypeStr))
        {
            if (ItemTypeStr == TEXT("Weapon")) Item.ItemType = EItemType::Weapon;
            else if (ItemTypeStr == TEXT("Armor")) Item.ItemType = EItemType::Armor;
            else if (ItemTypeStr == TEXT("Consumable")) Item.ItemType = EItemType::Consumable;
            else if (ItemTypeStr == TEXT("Material")) Item.ItemType = EItemType::Material;
            else if (ItemTypeStr == TEXT("Quest")) Item.ItemType = EItemType::Quest;
            else Item.ItemType = EItemType::Misc;
        }
        
        (*ItemObject)->TryGetNumberField(TEXT("stackSize"), Item.StackSize);
        (*ItemObject)->TryGetNumberField(TEXT("weight"), Item.Weight);
        (*ItemObject)->TryGetNumberField(TEXT("baseValue"), Item.BaseValue);
        
        FString TradeCategoryStr;
        if ((*ItemObject)->TryGetStringField(TEXT("tradeCategory"), TradeCategoryStr))
        {
            if (TradeCategoryStr == TEXT("MeleeWeapons")) Item.TradeCategory = ETradeCategory::MeleeWeapons;
            else if (TradeCategoryStr == TEXT("ModernWeapons")) Item.TradeCategory = ETradeCategory::ModernWeapons;
            else if (TradeCategoryStr == TEXT("Gems")) Item.TradeCategory = ETradeCategory::Gems;
            else if (TradeCategoryStr == TEXT("Antiques")) Item.TradeCategory = ETradeCategory::Antiques;
            else if (TradeCategoryStr == TEXT("Electronics")) Item.TradeCategory = ETradeCategory::Electronics;
            else if (TradeCategoryStr == TEXT("ModernGoods")) Item.TradeCategory = ETradeCategory::ModernGoods;
            else if (TradeCategoryStr == TEXT("Food")) Item.TradeCategory = ETradeCategory::Food;
            else if (TradeCategoryStr == TEXT("MagicMaterials")) Item.TradeCategory = ETradeCategory::MagicMaterials;
            else if (TradeCategoryStr == TEXT("MonsterMaterials")) Item.TradeCategory = ETradeCategory::MonsterMaterials;
            else if (TradeCategoryStr == TEXT("Medicine")) Item.TradeCategory = ETradeCategory::Medicine;
            else if (TradeCategoryStr == TEXT("CommonMaterials")) Item.TradeCategory = ETradeCategory::CommonMaterials;
            else if (TradeCategoryStr == TEXT("Luxury")) Item.TradeCategory = ETradeCategory::Luxury;
        }
        (*ItemObject)->TryGetNumberField(TEXT("maxDurability"), Item.MaxDurability);
        
        FString EquipSlotStr;
        if ((*ItemObject)->TryGetStringField(TEXT("equipmentSlot"), EquipSlotStr))
        {
            if (EquipSlotStr == TEXT("Weapon")) Item.EquipmentSlot = EEquipmentSlot::Weapon;
            else if (EquipSlotStr == TEXT("Shield")) Item.EquipmentSlot = EEquipmentSlot::Shield;
            else if (EquipSlotStr == TEXT("Head")) Item.EquipmentSlot = EEquipmentSlot::Head;
            else if (EquipSlotStr == TEXT("Body")) Item.EquipmentSlot = EEquipmentSlot::Body;
            else if (EquipSlotStr == TEXT("Legs")) Item.EquipmentSlot = EEquipmentSlot::Legs;
            else if (EquipSlotStr == TEXT("Hands")) Item.EquipmentSlot = EEquipmentSlot::Hands;
            else if (EquipSlotStr == TEXT("Feet")) Item.EquipmentSlot = EEquipmentSlot::Feet;
            else if (EquipSlotStr == TEXT("Accessory")) Item.EquipmentSlot = EEquipmentSlot::Accessory;
            else Item.EquipmentSlot = EEquipmentSlot::None;
        }
        
        // Weapon properties
        (*ItemObject)->TryGetNumberField(TEXT("attackPower"), Item.AttackPower);
        (*ItemObject)->TryGetNumberField(TEXT("criticalBonus"), Item.CriticalBonus);
        (*ItemObject)->TryGetNumberField(TEXT("attackRange"), Item.AttackRange);
        (*ItemObject)->TryGetNumberField(TEXT("requiredStrength"), Item.RequiredStrength);
        (*ItemObject)->TryGetNumberField(TEXT("requiredDexterity"), Item.RequiredDexterity);
        
        FString QualityStr;
        if ((*ItemObject)->TryGetStringField(TEXT("quality"), QualityStr))
        {
            if (QualityStr == TEXT("Poor")) Item.Quality = EItemQuality::Poor;
            else if (QualityStr == TEXT("Common")) Item.Quality = EItemQuality::Common;
            else if (QualityStr == TEXT("Good")) Item.Quality = EItemQuality::Good;
            else if (QualityStr == TEXT("Masterwork")) Item.Quality = EItemQuality::Masterwork;
            else if (QualityStr == TEXT("Legendary")) Item.Quality = EItemQuality::Legendary;
            else Item.Quality = EItemQuality::Common;
        }
        
        FString SkillTypeStr;
        if ((*ItemObject)->TryGetStringField(TEXT("requiredSkill"), SkillTypeStr))
        {
            if (SkillTypeStr == TEXT("OneHandedWeapons")) Item.RequiredSkill = ESkillType::OneHandedWeapons;
            else if (SkillTypeStr == TEXT("TwoHandedWeapons")) Item.RequiredSkill = ESkillType::TwoHandedWeapons;
            else if (SkillTypeStr == TEXT("PolearmWeapons")) Item.RequiredSkill = ESkillType::PolearmWeapons;
            else if (SkillTypeStr == TEXT("Archery")) Item.RequiredSkill = ESkillType::Archery;
            else if (SkillTypeStr == TEXT("Firearms")) Item.RequiredSkill = ESkillType::Firearms;
            else if (SkillTypeStr == TEXT("Throwing")) Item.RequiredSkill = ESkillType::Throwing;
            else if (SkillTypeStr == TEXT("Combat")) Item.RequiredSkill = ESkillType::Combat;
        }
        
        // Armor properties
        (*ItemObject)->TryGetNumberField(TEXT("defense"), Item.Defense);
        
        const TSharedPtr<FJsonObject>* StatBonusesObject;
        if ((*ItemObject)->TryGetObjectField(TEXT("statBonuses"), StatBonusesObject))
        {
            for (const auto& Bonus : (*StatBonusesObject)->Values)
            {
                int32 BonusValue;
                if (Bonus.Value->TryGetNumber(BonusValue))
                {
                    Item.StatBonuses.Add(Bonus.Key, BonusValue);
                }
            }
        }
        
        // Direct consumable effect properties (unified system)
        (*ItemObject)->TryGetNumberField(TEXT("health"), Item.Health);
        (*ItemObject)->TryGetNumberField(TEXT("stamina"), Item.Stamina);
        (*ItemObject)->TryGetNumberField(TEXT("strength"), Item.Strength);
        (*ItemObject)->TryGetNumberField(TEXT("agility"), Item.Agility);
        (*ItemObject)->TryGetNumberField(TEXT("intelligence"), Item.Intelligence);
        (*ItemObject)->TryGetNumberField(TEXT("dexterity"), Item.Dexterity);
        (*ItemObject)->TryGetNumberField(TEXT("duration"), Item.Duration);
        (*ItemObject)->TryGetBoolField(TEXT("curePoison"), Item.bCurePoison);
        (*ItemObject)->TryGetBoolField(TEXT("cureBleeding"), Item.bCureBleeding);
        
        UnifiedItemDatabase.Add(Item.ItemId, Item);
        ItemDatabase.Add(Item.ItemId, Item);
    }
}

void UItemManager::LoadWeaponsFromJSON(const FString& FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load weapons JSON from: %s"), *FilePath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse weapons JSON"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* WeaponsArray;
    if (!JsonObject->TryGetArrayField(TEXT("weapons"), WeaponsArray))
    {
        return;
    }

    for (const auto& WeaponValue : *WeaponsArray)
    {
        const TSharedPtr<FJsonObject>* WeaponObject;
        if (!WeaponValue->TryGetObject(WeaponObject))
        {
            continue;
        }

        FWeaponData Weapon;
        
        (*WeaponObject)->TryGetStringField(TEXT("id"), Weapon.ItemId);
        
        FString Name;
        (*WeaponObject)->TryGetStringField(TEXT("name"), Name);
        Weapon.ItemName = FText::FromString(Name);
        
        (*WeaponObject)->TryGetNumberField(TEXT("attackPower"), Weapon.AttackPower);
        (*WeaponObject)->TryGetNumberField(TEXT("weight"), Weapon.Weight);
        
        FString SkillTypeStr;
        if ((*WeaponObject)->TryGetStringField(TEXT("skillType"), SkillTypeStr))
        {
            if (SkillTypeStr == TEXT("OneHandedWeapons")) Weapon.RequiredSkill = ESkillType::OneHandedWeapons;
            else if (SkillTypeStr == TEXT("TwoHandedWeapons")) Weapon.RequiredSkill = ESkillType::TwoHandedWeapons;
            else if (SkillTypeStr == TEXT("PolearmWeapons")) Weapon.RequiredSkill = ESkillType::PolearmWeapons;
            else if (SkillTypeStr == TEXT("Archery")) Weapon.RequiredSkill = ESkillType::Archery;
            else if (SkillTypeStr == TEXT("Firearms")) Weapon.RequiredSkill = ESkillType::Firearms;
            else if (SkillTypeStr == TEXT("Throwing")) Weapon.RequiredSkill = ESkillType::Throwing;
            else if (SkillTypeStr == TEXT("Combat")) Weapon.RequiredSkill = ESkillType::Combat;
        }
        
        (*WeaponObject)->TryGetNumberField(TEXT("durability"), Weapon.MaxDurability);
        (*WeaponObject)->TryGetNumberField(TEXT("criticalBonus"), Weapon.CriticalBonus);
        (*WeaponObject)->TryGetNumberField(TEXT("range"), Weapon.AttackRange);
        
        Weapon.ItemType = EItemType::Weapon;
        Weapon.StackSize = 1;
        
        WeaponDatabase.Add(Weapon.ItemId, Weapon);
        ItemDatabase.Add(Weapon.ItemId, Weapon);
    }
}

void UItemManager::LoadArmorsFromJSON(const FString& FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load armors JSON from: %s"), *FilePath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse armors JSON"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* ArmorsArray;
    if (!JsonObject->TryGetArrayField(TEXT("armors"), ArmorsArray))
    {
        return;
    }

    for (const auto& ArmorValue : *ArmorsArray)
    {
        const TSharedPtr<FJsonObject>* ArmorObject;
        if (!ArmorValue->TryGetObject(ArmorObject))
        {
            continue;
        }

        FArmorData Armor;
        
        (*ArmorObject)->TryGetStringField(TEXT("id"), Armor.ItemId);
        
        FString Name;
        (*ArmorObject)->TryGetStringField(TEXT("name"), Name);
        Armor.ItemName = FText::FromString(Name);
        
        (*ArmorObject)->TryGetNumberField(TEXT("defense"), Armor.Defense);
        (*ArmorObject)->TryGetNumberField(TEXT("weight"), Armor.Weight);
        
        FString SlotStr;
        if ((*ArmorObject)->TryGetStringField(TEXT("slot"), SlotStr))
        {
            if (SlotStr == TEXT("Head")) Armor.EquipmentSlot = EEquipmentSlot::Head;
            else if (SlotStr == TEXT("Body")) Armor.EquipmentSlot = EEquipmentSlot::Body;
            else if (SlotStr == TEXT("Legs")) Armor.EquipmentSlot = EEquipmentSlot::Legs;
            else if (SlotStr == TEXT("Hands")) Armor.EquipmentSlot = EEquipmentSlot::Hands;
            else if (SlotStr == TEXT("Feet")) Armor.EquipmentSlot = EEquipmentSlot::Feet;
            else if (SlotStr == TEXT("Accessory")) Armor.EquipmentSlot = EEquipmentSlot::Accessory;
        }
        
        (*ArmorObject)->TryGetNumberField(TEXT("durability"), Armor.MaxDurability);
        
        const TSharedPtr<FJsonObject>* BonusesObject;
        if ((*ArmorObject)->TryGetObjectField(TEXT("bonuses"), BonusesObject))
        {
            for (const auto& Bonus : (*BonusesObject)->Values)
            {
                int32 BonusValue;
                if (Bonus.Value->TryGetNumber(BonusValue))
                {
                    Armor.StatBonuses.Add(Bonus.Key, BonusValue);
                }
            }
        }
        
        Armor.ItemType = EItemType::Armor;
        Armor.StackSize = 1;
        
        ArmorDatabase.Add(Armor.ItemId, Armor);
        ItemDatabase.Add(Armor.ItemId, Armor);
    }
}

void UItemManager::LoadConsumablesFromJSON(const FString& FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load consumables JSON from: %s"), *FilePath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse consumables JSON"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* ConsumablesArray;
    if (!JsonObject->TryGetArrayField(TEXT("consumables"), ConsumablesArray))
    {
        return;
    }

    for (const auto& ConsumableValue : *ConsumablesArray)
    {
        const TSharedPtr<FJsonObject>* ConsumableObject;
        if (!ConsumableValue->TryGetObject(ConsumableObject))
        {
            continue;
        }

        FConsumableData Consumable;
        
        (*ConsumableObject)->TryGetStringField(TEXT("id"), Consumable.ItemId);
        
        FString Name;
        (*ConsumableObject)->TryGetStringField(TEXT("name"), Name);
        Consumable.ItemName = FText::FromString(Name);
        
        FString Description;
        (*ConsumableObject)->TryGetStringField(TEXT("description"), Description);
        Consumable.Description = FText::FromString(Description);
        
        (*ConsumableObject)->TryGetNumberField(TEXT("weight"), Consumable.Weight);
        (*ConsumableObject)->TryGetNumberField(TEXT("value"), Consumable.BaseValue);
        (*ConsumableObject)->TryGetNumberField(TEXT("stackSize"), Consumable.StackSize);
        
        FString TypeStr;
        if ((*ConsumableObject)->TryGetStringField(TEXT("consumableType"), TypeStr))
        {
            if (TypeStr == TEXT("HealthRestore")) Consumable.ConsumableType = EConsumableType::HealthRestore;
            else if (TypeStr == TEXT("StaminaRestore")) Consumable.ConsumableType = EConsumableType::StaminaRestore;
            else if (TypeStr == TEXT("StatBoost")) Consumable.ConsumableType = EConsumableType::StatBoost;
            else if (TypeStr == TEXT("CureStatus")) Consumable.ConsumableType = EConsumableType::CureStatus;
        }
        
        (*ConsumableObject)->TryGetNumberField(TEXT("restoreAmount"), Consumable.RestoreAmount);
        (*ConsumableObject)->TryGetNumberField(TEXT("duration"), Consumable.Duration);
        (*ConsumableObject)->TryGetBoolField(TEXT("instant"), Consumable.bIsInstant);
        
        Consumable.ItemType = EItemType::Consumable;
        
        ConsumableDatabase.Add(Consumable.ItemId, Consumable);
        ItemDatabase.Add(Consumable.ItemId, Consumable);
    }
}

bool UItemManager::GetItemData(const FString& ItemId, FItemData& OutItemData)
{
    if (FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        OutItemData = *Item;
        return true;
    }
    
    // Fallback to legacy system
    if (FItemData* Item = ItemDatabase.Find(ItemId))
    {
        OutItemData = *Item;
        return true;
    }
    
    return false;
}

FItemData* UItemManager::GetItemDataPtr(const FString& ItemId)
{
    if (FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item;
    }
    
    // Fallback to legacy system
    if (FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item;
    }
    
    return nullptr;
}

bool UItemManager::CanEquipToSlot(const FString& ItemId, EEquipmentSlot Slot) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->EquipmentSlot == Slot || 
               (Item->EquipmentSlot == EEquipmentSlot::Accessory && Slot == EEquipmentSlot::Accessory);
    }
    
    // Legacy compatibility
    if (Slot == EEquipmentSlot::Weapon && WeaponDatabase.Contains(ItemId))
    {
        return true;
    }
    if ((Slot >= EEquipmentSlot::Shield && Slot <= EEquipmentSlot::Accessory) && ArmorDatabase.Contains(ItemId))
    {
        const FArmorData* Armor = ArmorDatabase.Find(ItemId);
        return Armor && Armor->EquipmentSlot == Slot;
    }
    
    return false;
}

FItemData* UItemManager::GetLegacyItemData(const FString& ItemId)
{
    return ItemDatabase.Find(ItemId);
}

bool UItemManager::GetWeaponData(const FString& ItemId, FWeaponData& OutWeaponData)
{
    // Try unified system first
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        if (Item->IsWeapon())
        {
            // Convert FItemData to FWeaponData
            OutWeaponData.ItemId = Item->ItemId;
            OutWeaponData.ItemName = Item->ItemName;
            OutWeaponData.Description = Item->Description;
            OutWeaponData.ItemType = Item->ItemType;
            OutWeaponData.StackSize = Item->StackSize;
            OutWeaponData.Weight = Item->Weight;
            OutWeaponData.BaseValue = Item->BaseValue;
            OutWeaponData.MaxDurability = Item->MaxDurability;
            OutWeaponData.AttackPower = Item->AttackPower;
            OutWeaponData.RequiredSkill = Item->RequiredSkill;
            OutWeaponData.CriticalBonus = Item->CriticalBonus;
            OutWeaponData.AttackRange = Item->AttackRange;
            OutWeaponData.RequiredStrength = Item->RequiredStrength;
            OutWeaponData.RequiredDexterity = Item->RequiredDexterity;
            return true;
        }
    }
    
    // Fallback to legacy system
    if (FWeaponData* Weapon = WeaponDatabase.Find(ItemId))
    {
        OutWeaponData = *Weapon;
        return true;
    }
    
    return false;
}

FWeaponData* UItemManager::GetWeaponDataPtr(const FString& ItemId)
{
    // Try unified system first
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        if (Item->IsWeapon())
        {
            // Create a temporary FWeaponData for backward compatibility
            static FWeaponData TempWeapon;
            TempWeapon.ItemId = Item->ItemId;
            TempWeapon.ItemName = Item->ItemName;
            TempWeapon.Description = Item->Description;
            TempWeapon.ItemType = Item->ItemType;
            TempWeapon.StackSize = Item->StackSize;
            TempWeapon.Weight = Item->Weight;
            TempWeapon.BaseValue = Item->BaseValue;
            TempWeapon.MaxDurability = Item->MaxDurability;
            TempWeapon.AttackPower = Item->AttackPower;
            TempWeapon.RequiredSkill = Item->RequiredSkill;
            TempWeapon.CriticalBonus = Item->CriticalBonus;
            TempWeapon.AttackRange = Item->AttackRange;
            TempWeapon.RequiredStrength = Item->RequiredStrength;
            TempWeapon.RequiredDexterity = Item->RequiredDexterity;
            return &TempWeapon;
        }
    }
    
    // Fallback to legacy system
    return WeaponDatabase.Find(ItemId);
}

bool UItemManager::GetArmorData(const FString& ItemId, FArmorData& OutArmorData)
{
    // Try unified system first
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        if (Item->IsArmor())
        {
            // Convert FItemData to FArmorData
            OutArmorData.ItemId = Item->ItemId;
            OutArmorData.ItemName = Item->ItemName;
            OutArmorData.Description = Item->Description;
            OutArmorData.ItemType = Item->ItemType;
            OutArmorData.StackSize = Item->StackSize;
            OutArmorData.Weight = Item->Weight;
            OutArmorData.BaseValue = Item->BaseValue;
            OutArmorData.MaxDurability = Item->MaxDurability;
            OutArmorData.Defense = Item->Defense;
            OutArmorData.EquipmentSlot = Item->EquipmentSlot;
            OutArmorData.StatBonuses = Item->StatBonuses;
            OutArmorData.RequiredStrength = Item->RequiredStrength;
            return true;
        }
    }
    
    // Fallback to legacy system
    if (FArmorData* Armor = ArmorDatabase.Find(ItemId))
    {
        OutArmorData = *Armor;
        return true;
    }
    
    return false;
}

FArmorData* UItemManager::GetArmorDataPtr(const FString& ItemId)
{
    // Try unified system first
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        if (Item->IsArmor())
        {
            // Create a temporary FArmorData for backward compatibility
            static FArmorData TempArmor;
            TempArmor.ItemId = Item->ItemId;
            TempArmor.ItemName = Item->ItemName;
            TempArmor.Description = Item->Description;
            TempArmor.ItemType = Item->ItemType;
            TempArmor.StackSize = Item->StackSize;
            TempArmor.Weight = Item->Weight;
            TempArmor.BaseValue = Item->BaseValue;
            TempArmor.MaxDurability = Item->MaxDurability;
            TempArmor.Defense = Item->Defense;
            TempArmor.EquipmentSlot = Item->EquipmentSlot;
            TempArmor.StatBonuses = Item->StatBonuses;
            TempArmor.RequiredStrength = Item->RequiredStrength;
            return &TempArmor;
        }
    }
    
    // Fallback to legacy system
    return ArmorDatabase.Find(ItemId);
}

bool UItemManager::GetConsumableData(const FString& ItemId, FConsumableData& OutConsumableData)
{
    // Try unified system first
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        if (Item->IsConsumable())
        {
            // Convert FItemData to FConsumableData
            OutConsumableData.ItemId = Item->ItemId;
            OutConsumableData.ItemName = Item->ItemName;
            OutConsumableData.Description = Item->Description;
            OutConsumableData.ItemType = Item->ItemType;
            OutConsumableData.StackSize = Item->StackSize;
            OutConsumableData.Weight = Item->Weight;
            OutConsumableData.BaseValue = Item->BaseValue;
            OutConsumableData.MaxDurability = Item->MaxDurability;
            OutConsumableData.Duration = Item->Duration;
            OutConsumableData.bIsInstant = Item->IsInstant();
            return true;
        }
    }
    
    // Fallback to legacy system
    if (FConsumableData* Consumable = ConsumableDatabase.Find(ItemId))
    {
        OutConsumableData = *Consumable;
        return true;
    }
    
    return false;
}

FConsumableData* UItemManager::GetConsumableDataPtr(const FString& ItemId)
{
    // Try unified system first
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        if (Item->IsConsumable())
        {
            // Create a temporary FConsumableData for backward compatibility
            static FConsumableData TempConsumable;
            TempConsumable.ItemId = Item->ItemId;
            TempConsumable.ItemName = Item->ItemName;
            TempConsumable.Description = Item->Description;
            TempConsumable.ItemType = Item->ItemType;
            TempConsumable.StackSize = Item->StackSize;
            TempConsumable.Weight = Item->Weight;
            TempConsumable.BaseValue = Item->BaseValue;
            TempConsumable.MaxDurability = Item->MaxDurability;
            TempConsumable.Duration = Item->Duration;
            TempConsumable.bIsInstant = Item->IsInstant();
            return &TempConsumable;
        }
    }
    
    // Fallback to legacy system
    return ConsumableDatabase.Find(ItemId);
}

EItemType UItemManager::GetItemType(const FString& ItemId) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->ItemType;
    }
    
    // Fallback to legacy system
    if (const FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item->ItemType;
    }
    
    return EItemType::Misc;
}

float UItemManager::GetItemWeight(const FString& ItemId) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->Weight;
    }
    
    // Fallback to legacy system
    if (const FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item->Weight;
    }
    
    return 0.0f;
}

int32 UItemManager::GetItemStackSize(const FString& ItemId) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->StackSize;
    }
    
    // Fallback to legacy system
    if (const FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item->StackSize;
    }
    
    return 1;
}

FText UItemManager::GetItemName(const FString& ItemId) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->ItemName;
    }
    
    // Fallback to legacy system
    if (const FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item->ItemName;
    }
    
    return FText::FromString(TEXT("Unknown Item"));
}

bool UItemManager::IsValidItem(const FString& ItemId) const
{
    return UnifiedItemDatabase.Contains(ItemId) || ItemDatabase.Contains(ItemId);
}

int32 UItemManager::GetRealWorldPrice(const FString& ItemId) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->GetRealWorldValue();
    }
    
    // Fallback to legacy system
    if (const FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item->BaseValue; // Legacy items use standard value
    }
    
    return 0;
}

int32 UItemManager::GetOtherWorldPrice(const FString& ItemId) const
{
    if (const FItemData* Item = UnifiedItemDatabase.Find(ItemId))
    {
        return Item->GetOtherWorldValue();
    }
    
    // Fallback to legacy system
    if (const FItemData* Item = ItemDatabase.Find(ItemId))
    {
        return Item->BaseValue; // Legacy items use standard value
    }
    
    return 0;
}

TArray<FString> UItemManager::GetAllItemIds() const
{
    TArray<FString> ItemIds;
    
    // From unified system
    UnifiedItemDatabase.GetKeys(ItemIds);
    
    // Add legacy items if not already present
    TArray<FString> LegacyIds;
    ItemDatabase.GetKeys(LegacyIds);
    for (const FString& LegacyId : LegacyIds)
    {
        ItemIds.AddUnique(LegacyId);
    }
    
    return ItemIds;
}

TArray<FItemData> UItemManager::GetAllItems() const
{
    TArray<FItemData> Items;
    
    // From unified system
    for (const auto& Pair : UnifiedItemDatabase)
    {
        Items.Add(Pair.Value);
    }
    
    // Add legacy items converted to FItemData if not already present
    for (const auto& Pair : ItemDatabase)
    {
        if (!UnifiedItemDatabase.Contains(Pair.Key))
        {
            FItemData ConvertedItem;
            ConvertedItem.ItemId = Pair.Value.ItemId;
            ConvertedItem.ItemName = Pair.Value.ItemName;
            ConvertedItem.Description = Pair.Value.Description;
            ConvertedItem.ItemType = Pair.Value.ItemType;
            ConvertedItem.StackSize = Pair.Value.StackSize;
            ConvertedItem.Weight = Pair.Value.Weight;
            ConvertedItem.BaseValue = Pair.Value.BaseValue;
            ConvertedItem.MaxDurability = Pair.Value.MaxDurability;
            Items.Add(ConvertedItem);
        }
    }
    
    return Items;
}

TArray<FString> UItemManager::GetItemsByType(EItemType ItemType) const
{
    TArray<FString> FilteredItems;
    
    for (const auto& Pair : UnifiedItemDatabase)
    {
        if (Pair.Value.ItemType == ItemType)
        {
            FilteredItems.Add(Pair.Key);
        }
    }
    
    // Check legacy items
    for (const auto& Pair : ItemDatabase)
    {
        if (!UnifiedItemDatabase.Contains(Pair.Key) && Pair.Value.ItemType == ItemType)
        {
            FilteredItems.Add(Pair.Key);
        }
    }
    
    return FilteredItems;
}

TArray<FString> UItemManager::GetItemsByQuality(EItemQuality Quality) const
{
    TArray<FString> FilteredItems;
    
    for (const auto& Pair : UnifiedItemDatabase)
    {
        if (Pair.Value.Quality == Quality)
        {
            FilteredItems.Add(Pair.Key);
        }
    }
    
    return FilteredItems;
}