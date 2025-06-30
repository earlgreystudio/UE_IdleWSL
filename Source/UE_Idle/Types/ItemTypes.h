#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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

// 古いFItemData関連の構造体は削除されました
// 新しいFItemDataRowシステム（ItemDataTable.h）を使用してください

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

    // 装備管理用フラグ
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsEquipped = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEquipmentSlot EquippedSlot = EEquipmentSlot::None;

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
struct FEquipmentReference
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid InstanceId;

    FEquipmentReference() {}

    FEquipmentReference(const FString& InItemId, const FGuid& InInstanceId)
        : ItemId(InItemId), InstanceId(InInstanceId) {}

    bool IsEmpty() const 
    { 
        return ItemId.IsEmpty() || !InstanceId.IsValid(); 
    }

    void Clear()
    {
        ItemId.Empty();
        InstanceId.Invalidate();
    }
};

USTRUCT(BlueprintType)
struct FEquipmentSlots
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Shield;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Legs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Hands;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Feet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Accessory1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquipmentReference Accessory2;

    FEquipmentReference* GetSlot(EEquipmentSlot Slot)
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

    FEquipmentReference* GetSecondaryAccessorySlot()
    {
        return &Accessory2;
    }

    bool IsSlotEmpty(EEquipmentSlot Slot) const
    {
        const FEquipmentReference* SlotRef = const_cast<FEquipmentSlots*>(this)->GetSlot(Slot);
        return !SlotRef || SlotRef->IsEmpty();
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