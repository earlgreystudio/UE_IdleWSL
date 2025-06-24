#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UE_Idle/Types/CharacterTypes.h"
#include "ItemDataTable.generated.h"

UENUM(BlueprintType)
enum class EItemTypeTable : uint8
{
    Weapon      UMETA(DisplayName = "武器"),
    Armor       UMETA(DisplayName = "防具"),
    Consumable  UMETA(DisplayName = "消耗品"),
    Material    UMETA(DisplayName = "素材"),
    Quest       UMETA(DisplayName = "クエストアイテム"),
    Misc        UMETA(DisplayName = "その他")
};

UENUM(BlueprintType)
enum class ETradeCategoryTable : uint8
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
enum class EEquipmentSlotTable : uint8
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

UENUM(BlueprintType)
enum class EItemQualityTable : uint8
{
    Poor        UMETA(DisplayName = "粗悪"),
    Common      UMETA(DisplayName = "普通"),
    Good        UMETA(DisplayName = "できの良い"),
    Masterwork  UMETA(DisplayName = "名匠"),
    Legendary   UMETA(DisplayName = "マスターワーク")
};

USTRUCT(BlueprintType)
struct UE_IDLE_API FItemDataRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EItemTypeTable ItemType = EItemTypeTable::Misc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 StackSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float Weight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 BaseValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    ETradeCategoryTable TradeCategory = ETradeCategoryTable::CommonMaterials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 MaxDurability = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    EEquipmentSlotTable EquipmentSlot = EEquipmentSlotTable::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
    EItemQualityTable Quality = EItemQualityTable::Common;

    // Weapon Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    int32 AttackPower = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    ESkillType RequiredSkill = ESkillType::OneHandedWeapons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float CriticalBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float AttackRange = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    int32 RequiredStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    int32 RequiredDexterity = 0;

    // Armor Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 Defense = 0;

    // Consumable Properties (Direct Effects)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    int32 Health = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    int32 Stamina = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    int32 Strength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    int32 Agility = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    int32 Intelligence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    int32 Dexterity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    float Duration = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    bool CurePoison = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable Effects")
    bool CureBleeding = false;

    // Helper Functions
    bool IsEquippable() const { return EquipmentSlot != EEquipmentSlotTable::None; }

    bool IsWeapon() const { return EquipmentSlot == EEquipmentSlotTable::Weapon; }

    bool IsArmor() const { return EquipmentSlot >= EEquipmentSlotTable::Shield && EquipmentSlot <= EEquipmentSlotTable::Accessory; }

    bool IsConsumable() const { return ItemType == EItemTypeTable::Consumable; }

    bool HasEffect() const 
    { 
        return Health != 0 || Stamina != 0 || Strength != 0 || 
               Agility != 0 || Intelligence != 0 || Dexterity != 0 ||
               CurePoison || CureBleeding;
    }

    bool IsInstant() const { return Duration <= 0.0f; }

    bool BlocksShield() const 
    {
        return IsWeapon() && (
            RequiredSkill == ESkillType::TwoHandedWeapons ||
            RequiredSkill == ESkillType::PolearmWeapons ||
            RequiredSkill == ESkillType::Archery ||
            RequiredSkill == ESkillType::Firearms
        );
    }

    // Quality-modified values
    int32 GetModifiedAttackPower() const;

    int32 GetModifiedDefense() const;

    int32 GetModifiedDurability() const;

    int32 GetModifiedValue() const;

    static float GetQualityModifier(EItemQualityTable Quality);
};