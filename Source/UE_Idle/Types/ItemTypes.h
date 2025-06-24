#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UE_Idle/Types/CharacterTypes.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Weapon      UMETA(DisplayName = "武器"),
    Armor       UMETA(DisplayName = "防具"),
    Consumable  UMETA(DisplayName = "消耗品"),
    Material    UMETA(DisplayName = "素材"),
    Quest       UMETA(DisplayName = "クエストアイテム"),
    Misc        UMETA(DisplayName = "その他")
};

UENUM(BlueprintType)
enum class ETradeCategory : uint8
{
    MeleeWeapons        UMETA(DisplayName = "近接武器"),
    ModernWeapons       UMETA(DisplayName = "現代武器"),
    Gems                UMETA(DisplayName = "宝石・貴金属"),
    Antiques            UMETA(DisplayName = "骨董品・美術品"),
    Electronics         UMETA(DisplayName = "電子機器"),
    ModernGoods         UMETA(DisplayName = "現代日用品"),
    Food                UMETA(DisplayName = "食品・調味料"),
    MagicMaterials      UMETA(DisplayName = "魔法素材"),
    MonsterMaterials    UMETA(DisplayName = "モンスター素材"),
    Medicine            UMETA(DisplayName = "薬品・医療品"),
    CommonMaterials     UMETA(DisplayName = "一般素材"),
    Luxury              UMETA(DisplayName = "贅沢品")
};


UENUM(BlueprintType)
enum class EItemQuality : uint8
{
    Poor        UMETA(DisplayName = "粗悪"),
    Common      UMETA(DisplayName = "普通"),
    Good        UMETA(DisplayName = "できの良い"),
    Masterwork  UMETA(DisplayName = "名匠"),
    Legendary   UMETA(DisplayName = "マスターワーク")
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    None        UMETA(DisplayName = "装備不可"),
    Weapon      UMETA(DisplayName = "武器"),
    Shield      UMETA(DisplayName = "盾"),
    Head        UMETA(DisplayName = "頭"),
    Body        UMETA(DisplayName = "胴"),
    Legs        UMETA(DisplayName = "脚"),
    Hands       UMETA(DisplayName = "手"),
    Feet        UMETA(DisplayName = "靴"),
    Accessory   UMETA(DisplayName = "アクセサリ")
};

// Backward compatibility
using EArmorSlot = EEquipmentSlot;

UENUM(BlueprintType)
enum class EConsumableType : uint8
{
    HealthRestore   UMETA(DisplayName = "HP回復"),
    StaminaRestore  UMETA(DisplayName = "スタミナ回復"),
    StatBoost       UMETA(DisplayName = "能力値上昇"),
    CureStatus      UMETA(DisplayName = "状態異常回復"),
    Other           UMETA(DisplayName = "その他")
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BaseValue = 0;                    // 基準価格

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETradeCategory TradeCategory = ETradeCategory::CommonMaterials;  // 取引カテゴリ

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxDurability = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UTexture2D* Icon = nullptr;

    // Equipment Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

    // Weapon Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AttackPower = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESkillType RequiredSkill = ESkillType::OneHandedWeapons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackRange = 0.0f;

    // Armor Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Defense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, int32> StatBonuses;

    // Consumable Properties - Direct stat effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Health = 0;           // HP回復/変更

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Stamina = 0;          // スタミナ回復/変更

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Strength = 0;         // 力変更

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Agility = 0;          // 敏捷変更

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Intelligence = 0;     // 知力変更

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Dexterity = 0;        // 器用変更

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Duration = 0.0f;      // 持続時間（0なら即座）

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCurePoison = false;   // 毒治療

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCureBleeding = false; // 出血治療

    // Requirements
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RequiredStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RequiredDexterity = 0;

    // Quality Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemQuality Quality = EItemQuality::Common;

    bool IsEquippable() const { return EquipmentSlot != EEquipmentSlot::None; }
    bool IsWeapon() const { return EquipmentSlot == EEquipmentSlot::Weapon; }
    bool IsArmor() const { return EquipmentSlot >= EEquipmentSlot::Shield && EquipmentSlot <= EEquipmentSlot::Accessory; }
    bool IsConsumable() const { return ItemType == EItemType::Consumable; }
    
    bool HasEffect() const 
    { 
        return Health != 0 || Stamina != 0 || Strength != 0 || 
               Agility != 0 || Intelligence != 0 || Dexterity != 0 ||
               bCurePoison || bCureBleeding;
    }
    
    bool IsInstant() const { return Duration <= 0.0f; }
    
    // 武器装備制限
    bool BlocksShield() const 
    {
        return IsWeapon() && (
            RequiredSkill == ESkillType::TwoHandedWeapons ||
            RequiredSkill == ESkillType::PolearmWeapons ||
            RequiredSkill == ESkillType::Archery ||
            RequiredSkill == ESkillType::Firearms
        );
    }
    
    bool IsTwoHanded() const { return BlocksShield(); }
    
    // 取引価格計算
    int32 GetRealWorldValue() const;
    int32 GetOtherWorldValue() const;
    
    static float GetCategoryRealWorldMultiplier(ETradeCategory Category);
    static float GetCategoryOtherWorldMultiplier(ETradeCategory Category);
    
    // Quality system
    int32 GetModifiedAttackPower() const;
    int32 GetModifiedDefense() const;
    int32 GetModifiedDurability() const;
    int32 GetModifiedValue() const;
    
    static float GetQualityModifier(EItemQuality Quality);
};

USTRUCT(BlueprintType)
struct FWeaponData : public FItemData
{
    GENERATED_BODY()

    FWeaponData()
    {
        ItemType = EItemType::Weapon;
        EquipmentSlot = EEquipmentSlot::Weapon;
    }
};

USTRUCT(BlueprintType)
struct FArmorData : public FItemData
{
    GENERATED_BODY()

    // ArmorSlot is now mapped to EquipmentSlot from base class
    EEquipmentSlot GetArmorSlot() const { return EquipmentSlot; }
    void SetArmorSlot(EEquipmentSlot Slot) { EquipmentSlot = Slot; }

    FArmorData()
    {
        ItemType = EItemType::Armor;
    }
};

USTRUCT(BlueprintType)
struct FConsumableData : public FItemData
{
    GENERATED_BODY()

    // Legacy properties for backward compatibility
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EConsumableType ConsumableType = EConsumableType::HealthRestore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RestoreAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, int32> StatModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsInstant = true;

    FConsumableData()
    {
        ItemType = EItemType::Consumable;
        StackSize = 99;
    }
};

// Backward compatibility
using FItemBase = FItemData;

USTRUCT(BlueprintType)
struct FItemInstance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid InstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentDurability = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, float> CustomData;

    FItemInstance() 
    {
        InstanceId = FGuid::NewGuid();
    }

    FItemInstance(const FString& InItemId, int32 InDurability = 100)
        : ItemId(InItemId), CurrentDurability(InDurability)
    {
        InstanceId = FGuid::NewGuid();
    }
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemInstance> ItemInstances;

    FInventorySlot() {}

    FInventorySlot(const FString& InItemId)
        : ItemId(InItemId), Quantity(0) {}

    void AddStackableItem(int32 Amount)
    {
        Quantity += Amount;
    }

    void AddInstance(const FItemInstance& Instance)
    {
        ItemInstances.Add(Instance);
        Quantity = ItemInstances.Num();
    }

    bool RemoveStackableItem(int32 Amount)
    {
        if (Quantity >= Amount)
        {
            Quantity -= Amount;
            return true;
        }
        return false;
    }

    bool RemoveInstance(const FGuid& InstanceId)
    {
        int32 RemovedCount = ItemInstances.RemoveAll([&InstanceId](const FItemInstance& Instance)
        {
            return Instance.InstanceId == InstanceId;
        });
        Quantity = ItemInstances.Num();
        return RemovedCount > 0;
    }

    FItemInstance* GetInstance(const FGuid& InstanceId)
    {
        for (FItemInstance& Instance : ItemInstances)
        {
            if (Instance.InstanceId == InstanceId)
            {
                return &Instance;
            }
        }
        return nullptr;
    }
    
    const FItemInstance* GetInstance(const FGuid& InstanceId) const
    {
        for (const FItemInstance& Instance : ItemInstances)
        {
            if (Instance.InstanceId == InstanceId)
            {
                return &Instance;
            }
        }
        return nullptr;
    }
};

USTRUCT(BlueprintType)
struct FEquipmentSlots
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Shield;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Legs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Hands;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Feet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Accessory1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemInstance Accessory2;

    FItemInstance* GetSlot(EEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EEquipmentSlot::Weapon: return &Weapon;
        case EEquipmentSlot::Shield: return &Shield;
        case EEquipmentSlot::Head: return &Head;
        case EEquipmentSlot::Body: return &Body;
        case EEquipmentSlot::Legs: return &Legs;
        case EEquipmentSlot::Hands: return &Hands;
        case EEquipmentSlot::Feet: return &Feet;
        case EEquipmentSlot::Accessory: return &Accessory1;
        default: return nullptr;
        }
    }

    FItemInstance* GetSecondaryAccessorySlot()
    {
        return &Accessory2;
    }

    bool IsSlotEmpty(EEquipmentSlot Slot) const
    {
        const FItemInstance* SlotItem = const_cast<FEquipmentSlots*>(this)->GetSlot(Slot);
        return !SlotItem || SlotItem->ItemId.IsEmpty();
    }

    bool CanEquipToSlot(EEquipmentSlot ItemSlot, EEquipmentSlot TargetSlot) const
    {
        if (ItemSlot == TargetSlot)
        {
            return true;
        }
        
        // Accessories can go to either accessory slot
        if (ItemSlot == EEquipmentSlot::Accessory && 
            (TargetSlot == EEquipmentSlot::Accessory))
        {
            return true;
        }
        
        return false;
    }

    float GetTotalWeight(class UItemDataTableManager* ItemManager) const;
    int32 GetTotalDefense(class UItemDataTableManager* ItemManager) const;
};

USTRUCT(BlueprintType)
struct FInventory
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInventorySlot> Slots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxSlots = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxWeight = 100.0f;

    bool AddItem(const FString& ItemId, int32 Quantity, class UItemDataTableManager* ItemManager);
    bool AddItemInstance(const FItemInstance& Instance, class UItemDataTableManager* ItemManager);
    bool RemoveItem(const FString& ItemId, int32 Quantity);
    bool RemoveItemInstance(const FGuid& InstanceId, FItemInstance& OutInstance);
    FInventorySlot* FindSlot(const FString& ItemId);
    FItemInstance* FindInstance(const FGuid& InstanceId);
    const FItemInstance* FindInstance(const FGuid& InstanceId) const;
    int32 GetItemCount(const FString& ItemId) const;
    float GetTotalWeight(class UItemDataTableManager* ItemManager) const;
    bool HasSpace(const FString& ItemId, int32 Quantity, class UItemDataTableManager* ItemManager) const;
    int32 GetUsedSlots() const;
};