#include "InventoryComponent.h"
#include "../Managers/ItemDataTableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Initialize basic settings
    Inventory.MaxSlots = 50;
    Inventory.MaxWeight = 100.0f;
    Money = 0;
    
    // Resources初期化削除 - 新採集システムではResourceカテゴリのItemとして管理
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get ItemManager reference
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
        if (!ItemManager)
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryComponent: ItemDataTableManager not found!"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Initialized for %s"), *OwnerId);
}

// ========== Core Inventory Operations ==========

bool UInventoryComponent::AddItem(const FString& ItemId, int32 Quantity)
{
    if (!ItemManager || Quantity <= 0)
    {
        return false;
    }

    // 重量チェック
    if (!CanAddItemByWeight(ItemId, Quantity))
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Cannot add item %s x%d - would exceed carrying capacity"), *ItemId, Quantity);
        return false;
    }

    bool bSuccess = Inventory.AddItem(ItemId, Quantity, ItemManager);
    
    if (bSuccess)
    {
        OnInventoryChanged.Broadcast(ItemId, GetItemCount(ItemId));
    }
    
    return bSuccess;
}

bool UInventoryComponent::RemoveItem(const FString& ItemId, int32 Quantity)
{
    if (Quantity <= 0 || !HasItem(ItemId, Quantity))
    {
        return false;
    }

    bool bSuccess = Inventory.RemoveItem(ItemId, Quantity);
    
    if (bSuccess)
    {
        OnInventoryChanged.Broadcast(ItemId, GetItemCount(ItemId));
    }
    
    return bSuccess;
}

bool UInventoryComponent::HasItem(const FString& ItemId, int32 Quantity) const
{
    return Inventory.GetItemCount(ItemId) >= Quantity;
}

int32 UInventoryComponent::GetItemCount(const FString& ItemId) const
{
    return Inventory.GetItemCount(ItemId);
}

bool UInventoryComponent::TransferTo(UInventoryComponent* TargetInventory, const FString& ItemId, int32 Quantity)
{
    if (!TargetInventory || !HasItem(ItemId, Quantity))
    {
        return false;
    }

    // Remove from source
    if (!RemoveItem(ItemId, Quantity))
    {
        return false;
    }

    // Add to target
    if (!TargetInventory->AddItem(ItemId, Quantity))
    {
        // Rollback: add back to source
        AddItem(ItemId, Quantity);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Transferred %d %s from %s to %s"), 
           Quantity, *ItemId, *OwnerId, *TargetInventory->OwnerId);
    
    return true;
}

TArray<FInventorySlot> UInventoryComponent::GetAllSlots() const
{
    TArray<FInventorySlot> Result;
    for (const FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.Quantity > 0)
        {
            Result.Add(Slot);
        }
    }
    return Result;
}

TMap<FString, int32> UInventoryComponent::GetAllItems() const
{
    TMap<FString, int32> Result;
    for (const FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.Quantity > 0)
        {
            Result.Add(Slot.ItemId, Slot.Quantity);
        }
    }
    return Result;
}

float UInventoryComponent::GetTotalWeight() const
{
    if (!ItemManager)
    {
        return 0.0f;
    }
    
    return Inventory.GetTotalWeight(ItemManager);
}

// ========== Equipment Functions ==========

bool UInventoryComponent::EquipItem(const FString& ItemId)
{
    if (!ItemManager || !HasItem(ItemId))
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData) || !ItemData.IsEquippable())
    {
        return false;
    }

    if (ItemData.IsWeapon())
    {
        return EquipWeapon(ItemId);
    }
    else if (ItemData.IsArmor())
    {
        return EquipArmor(ItemId);
    }

    return false;
}

bool UInventoryComponent::EquipWeapon(const FString& WeaponId)
{
    if (!ItemManager || !HasItem(WeaponId))
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(WeaponId, ItemData) || !ItemData.IsWeapon())
    {
        return false;
    }

    // Unequip current weapon
    if (!Equipment.Weapon.IsEmpty())
    {
        UnequipWeapon();
    }

    // Find unequipped instance
    FItemInstance* InstanceToEquip = nullptr;
    for (FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.ItemId == WeaponId)
        {
            for (FItemInstance& Instance : Slot.ItemInstances)
            {
                if (!Instance.bIsEquipped)
                {
                    InstanceToEquip = &Instance;
                    break;
                }
            }
            if (InstanceToEquip) break;
        }
    }

    if (!InstanceToEquip)
    {
        return false;
    }

    // Equip the item
    InstanceToEquip->bIsEquipped = true;
    InstanceToEquip->EquippedSlot = EEquipmentSlot::Weapon;
    Equipment.Weapon = FEquipmentReference(WeaponId, InstanceToEquip->InstanceId);
    
    OnItemEquipped.Broadcast(WeaponId, EEquipmentSlot::Weapon);
    return true;
}

bool UInventoryComponent::EquipShield(const FString& ShieldId)
{
    if (!ItemManager || !HasItem(ShieldId))
    {
        return false;
    }

    // Unequip current shield
    if (!Equipment.Shield.IsEmpty())
    {
        UnequipShield();
    }

    // Find unequipped instance
    FItemInstance* InstanceToEquip = nullptr;
    for (FInventorySlot& Slot : Inventory.Slots)
    {
        if (Slot.ItemId == ShieldId)
        {
            for (FItemInstance& Instance : Slot.ItemInstances)
            {
                if (!Instance.bIsEquipped)
                {
                    InstanceToEquip = &Instance;
                    break;
                }
            }
            if (InstanceToEquip) break;
        }
    }

    if (!InstanceToEquip)
    {
        return false;
    }

    // Equip the item
    InstanceToEquip->bIsEquipped = true;
    InstanceToEquip->EquippedSlot = EEquipmentSlot::Shield;
    Equipment.Shield = FEquipmentReference(ShieldId, InstanceToEquip->InstanceId);
    
    OnItemEquipped.Broadcast(ShieldId, EEquipmentSlot::Shield);
    return true;
}

bool UInventoryComponent::EquipArmor(const FString& ArmorId)
{
    if (!ItemManager || !HasItem(ArmorId))
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ArmorId, ItemData) || !ItemData.IsArmor())
    {
        return false;
    }

    return EquipToSlot(ArmorId, (EEquipmentSlot)ItemData.EquipmentSlot);
}

bool UInventoryComponent::EquipToSlot(const FString& ItemId, EEquipmentSlot Slot)
{
    if (!HasItem(ItemId))
    {
        return false;
    }

    // Get target slot
    FEquipmentReference* TargetSlot = Equipment.GetSlot(Slot);
    if (!TargetSlot)
    {
        return false;
    }

    // Unequip current item in slot
    if (!TargetSlot->IsEmpty())
    {
        UnequipFromSlot(Slot);
    }

    // Find unequipped instance
    FItemInstance* InstanceToEquip = nullptr;
    for (FInventorySlot& InventorySlot : Inventory.Slots)
    {
        if (InventorySlot.ItemId == ItemId)
        {
            for (FItemInstance& Instance : InventorySlot.ItemInstances)
            {
                if (!Instance.bIsEquipped)
                {
                    InstanceToEquip = &Instance;
                    break;
                }
            }
            if (InstanceToEquip) break;
        }
    }

    if (!InstanceToEquip)
    {
        return false;
    }

    // Equip the item
    InstanceToEquip->bIsEquipped = true;
    InstanceToEquip->EquippedSlot = Slot;
    *TargetSlot = FEquipmentReference(ItemId, InstanceToEquip->InstanceId);

    OnItemEquipped.Broadcast(ItemId, Slot);
    return true;
}

bool UInventoryComponent::UnequipWeapon()
{
    if (Equipment.Weapon.IsEmpty())
    {
        return false;
    }

    FString WeaponId = Equipment.Weapon.ItemId;
    
    // Find and unequip instance
    FItemInstance* EquippedInstance = Inventory.FindInstance(Equipment.Weapon.InstanceId);
    if (EquippedInstance)
    {
        EquippedInstance->bIsEquipped = false;
        EquippedInstance->EquippedSlot = EEquipmentSlot::None;
    }

    Equipment.Weapon.Clear();
    OnItemUnequipped.Broadcast(WeaponId, EEquipmentSlot::Weapon);
    
    return true;
}

bool UInventoryComponent::UnequipShield()
{
    if (Equipment.Shield.IsEmpty())
    {
        return false;
    }

    FString ShieldId = Equipment.Shield.ItemId;
    
    // Find and unequip instance
    FItemInstance* EquippedInstance = Inventory.FindInstance(Equipment.Shield.InstanceId);
    if (EquippedInstance)
    {
        EquippedInstance->bIsEquipped = false;
        EquippedInstance->EquippedSlot = EEquipmentSlot::None;
    }

    Equipment.Shield.Clear();
    OnItemUnequipped.Broadcast(ShieldId, EEquipmentSlot::Shield);
    
    return true;
}

bool UInventoryComponent::UnequipArmor(EEquipmentSlot Slot)
{
    return UnequipFromSlot(Slot);
}

bool UInventoryComponent::UnequipFromSlot(EEquipmentSlot Slot)
{
    FEquipmentReference* SlotRef = Equipment.GetSlot(Slot);
    if (!SlotRef || SlotRef->IsEmpty())
    {
        return false;
    }

    FString ItemId = SlotRef->ItemId;
    
    // Find and unequip instance
    FItemInstance* EquippedInstance = Inventory.FindInstance(SlotRef->InstanceId);
    if (EquippedInstance)
    {
        EquippedInstance->bIsEquipped = false;
        EquippedInstance->EquippedSlot = EEquipmentSlot::None;
    }

    SlotRef->Clear();
    OnItemUnequipped.Broadcast(ItemId, Slot);
    
    return true;
}

bool UInventoryComponent::CanEquipItem(const FString& ItemId) const
{
    if (!ItemManager)
    {
        return false;
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        return false;
    }

    return ItemData.IsEquippable();
}

float UInventoryComponent::GetTotalEquipmentWeight() const
{
    if (!ItemManager)
    {
        return 0.0f;
    }
    
    float TotalWeight = 0.0f;
    
    auto AddWeightFromSlot = [&](const FEquipmentReference& SlotRef)
    {
        if (!SlotRef.IsEmpty())
        {
            FItemInstance* Instance = const_cast<FInventory&>(Inventory).FindInstance(SlotRef.InstanceId);
            if (Instance)
            {
                FItemDataRow ItemData;
                if (ItemManager->GetItemData(Instance->ItemId, ItemData))
                {
                    TotalWeight += ItemData.Weight;
                }
            }
        }
    };
    
    AddWeightFromSlot(Equipment.Weapon);
    AddWeightFromSlot(Equipment.Shield);
    AddWeightFromSlot(Equipment.Head);
    AddWeightFromSlot(Equipment.Body);
    AddWeightFromSlot(Equipment.Legs);
    AddWeightFromSlot(Equipment.Hands);
    AddWeightFromSlot(Equipment.Feet);
    AddWeightFromSlot(Equipment.Accessory1);
    AddWeightFromSlot(Equipment.Accessory2);
    
    return TotalWeight;
}

int32 UInventoryComponent::GetTotalDefense() const
{
    if (!ItemManager)
    {
        return 0;
    }
    
    int32 TotalDefense = 0;
    
    auto AddDefenseFromSlot = [&](const FEquipmentReference& SlotRef)
    {
        if (!SlotRef.IsEmpty())
        {
            FItemInstance* Instance = const_cast<FInventory&>(Inventory).FindInstance(SlotRef.InstanceId);
            if (Instance)
            {
                FItemDataRow ItemData;
                if (ItemManager->GetItemData(Instance->ItemId, ItemData))
                {
                    TotalDefense += ItemData.Defense;
                }
            }
        }
    };
    
    AddDefenseFromSlot(Equipment.Shield);
    AddDefenseFromSlot(Equipment.Head);
    AddDefenseFromSlot(Equipment.Body);
    AddDefenseFromSlot(Equipment.Legs);
    AddDefenseFromSlot(Equipment.Hands);
    AddDefenseFromSlot(Equipment.Feet);
    
    return TotalDefense;
}

// ========== Money Functions ==========

bool UInventoryComponent::AddMoney(int32 Amount)
{
    if (Amount < 0)
    {
        return false;
    }

    Money += Amount;
    OnMoneyChanged.Broadcast(Money);
    
    return true;
}

bool UInventoryComponent::SpendMoney(int32 Amount)
{
    if (Amount < 0 || Money < Amount)
    {
        return false;
    }

    Money -= Amount;
    OnMoneyChanged.Broadcast(Money);
    
    return true;
}

bool UInventoryComponent::BuyItem(const FString& ItemId, int32 Quantity)
{
    if (!ItemManager || !ItemManager->IsValidItem(ItemId))
    {
        return false;
    }

    int32 TotalCost = ItemManager->GetItemValue(ItemId) * Quantity;
    if (!SpendMoney(TotalCost))
    {
        return false;
    }

    if (!AddItem(ItemId, Quantity))
    {
        // Rollback money
        AddMoney(TotalCost);
        return false;
    }

    return true;
}

bool UInventoryComponent::SellItem(const FString& ItemId, int32 Quantity)
{
    if (!HasItem(ItemId, Quantity))
    {
        return false;
    }

    if (!ItemManager)
    {
        return false;
    }

    int32 SellValue = FMath::RoundToInt(ItemManager->GetItemValue(ItemId) * Quantity * 0.8f);
    
    if (!RemoveItem(ItemId, Quantity))
    {
        return false;
    }

    AddMoney(SellValue);
    return true;
}

// ========== Resource Functions 削除 ==========
// 新採集システムではResourceカテゴリのItemとして管理

// ========== Carrying Capacity Functions ==========

float UInventoryComponent::GetMaxCarryingCapacity() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return 0.0f;
    }

    // TeamInventory削除 - 新採集システムでは使用しない

    // PlayerController = 無限積載量（拠点の倉庫）
    if (Owner->IsA<APlayerController>())
    {
        UE_LOG(LogTemp, Log, TEXT("InventoryComponent::GetMaxCarryingCapacity - PlayerController (Base storage): Infinite"));
        return FLT_MAX; // 実質無限
    }

    // Character = そのキャラクターのCarryingCapacity
    if (AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(Owner))
    {
        if (UCharacterStatusComponent* StatusComp = Character->FindComponentByClass<UCharacterStatusComponent>())
        {
            float CharacterCapacity = StatusComp->GetCarryingCapacity();
            UE_LOG(LogTemp, VeryVerbose, TEXT("InventoryComponent::GetMaxCarryingCapacity - Character capacity: %.1fkg"), CharacterCapacity);
            return CharacterCapacity;
        }
        return 20.0f; // デフォルト値
    }

    UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::GetMaxCarryingCapacity - Unknown owner type: %s"), *Owner->GetClass()->GetName());
    return 0.0f;
}

float UInventoryComponent::GetLoadRatio() const
{
    float MaxCapacity = GetMaxCarryingCapacity();
    if (MaxCapacity <= 0.0f || MaxCapacity == FLT_MAX)
    {
        return 0.0f; // 無限の場合は0%扱い
    }

    float CurrentWeight = GetTotalWeight();
    return CurrentWeight / MaxCapacity;
}

bool UInventoryComponent::IsOverweight() const
{
    float MaxCapacity = GetMaxCarryingCapacity();
    if (MaxCapacity == FLT_MAX)
    {
        return false; // 無限積載の場合はオーバーしない
    }

    return GetTotalWeight() > MaxCapacity;
}

bool UInventoryComponent::CanAddItemByWeight(const FString& ItemId, int32 Quantity) const
{
    if (!ItemManager)
    {
        return false;
    }

    float MaxCapacity = GetMaxCarryingCapacity();
    if (MaxCapacity == FLT_MAX)
    {
        return true; // 無限積載の場合は常にOK
    }

    FItemDataRow ItemData;
    if (!ItemManager->GetItemData(ItemId, ItemData))
    {
        return false;
    }

    float ItemWeight = ItemData.Weight * Quantity;
    float CurrentWeight = GetTotalWeight();
    
    return (CurrentWeight + ItemWeight) <= MaxCapacity;
}