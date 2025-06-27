#include "C__InventoryList.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "../Managers/ItemDataTableManager.h"
#include "../Components/InventoryComponent.h"
#include "../Components/TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "C_ItemListCard.h"

UC__InventoryList::UC__InventoryList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UC__InventoryList::NativeConstruct()
{
    Super::NativeConstruct();

    // Get ItemDataTableManager
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>();
    }

    // Initialize sort combo box
    InitializeSortComboBox();

    // Initial refresh if inventory is already set
    if (CachedInventoryComponent)
    {
        RefreshInventoryList();
    }
}

void UC__InventoryList::NativeDestruct()
{
    UnbindInventoryEvents();
    Super::NativeDestruct();
}

void UC__InventoryList::InitializeWithInventory(UInventoryComponent* InInventoryComponent)
{
    if (!InInventoryComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC__InventoryList: InitializeWithInventory called with null inventory"));
        // Clear the display when no inventory is provided
        ClearItemCards();
        if (WeightDisplayText)
        {
            WeightDisplayText->SetText(FText::FromString(TEXT("0.0kg / 0.0kg")));
        }
        if (InventoryNameText)
        {
            InventoryNameText->SetText(FText::FromString(TEXT("")));
        }
        CachedInventoryComponent = nullptr;
        return;
    }

    // Unbind from previous inventory
    if (CachedInventoryComponent)
    {
        UnbindInventoryEvents();
    }

    // Set new inventory
    CachedInventoryComponent = InInventoryComponent;
    
    // Bind to new inventory
    BindInventoryEvents();

    // Refresh display
    RefreshInventoryList();
    
    // Update inventory name immediately after setting new inventory
    UpdateInventoryName();
}

void UC__InventoryList::RefreshInventoryList()
{
    UE_LOG(LogTemp, Log, TEXT("InventoryList::RefreshInventoryList - Starting refresh"));
    
    if (!CachedInventoryComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC__InventoryList: No cached inventory component"));
        return;
    }

    // Get all inventory slots
    CachedInventorySlots = CachedInventoryComponent->GetAllSlots();
    UE_LOG(LogTemp, Log, TEXT("InventoryList::RefreshInventoryList - Retrieved %d slots from inventory"), CachedInventorySlots.Num());

    // Apply filter if active
    if (bIsFiltered)
    {
        CachedInventorySlots = CachedInventorySlots.FilterByPredicate([this](const FInventorySlot& InventorySlot)
        {
            if (FItemDataRow* ItemData = GetItemData(InventorySlot.ItemId))
            {
                return ItemData->ItemType == FilterType;
            }
            return false;
        });
        UE_LOG(LogTemp, Log, TEXT("InventoryList::RefreshInventoryList - After filtering: %d slots"), CachedInventorySlots.Num());
    }

    // Sort inventory
    SortInventory();

    // Update display
    UpdateItemCards();

    // Update weight display
    UpdateWeightDisplay();
    
    // Update inventory name
    UpdateInventoryName();
    
    UE_LOG(LogTemp, Log, TEXT("InventoryList::RefreshInventoryList - Refresh complete"));
}

void UC__InventoryList::UpdateItemCards()
{
    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateItemCards - Starting update, CachedSlots: %d"), CachedInventorySlots.Num());
    
    if (!ItemCardPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC__InventoryList: ItemCardPanel is null"));
        return;
    }

    // Clear existing cards
    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateItemCards - Clearing existing cards..."));
    ClearItemCards();

    // Create new cards for each slot
    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateItemCards - Creating %d new cards..."), CachedInventorySlots.Num());
    for (const FInventorySlot& InventorySlot : CachedInventorySlots)
    {
        CreateItemCard(InventorySlot);
    }

    // Update weight display
    UpdateWeightDisplay();
    
    // Update inventory name
    UpdateInventoryName();
    
    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateItemCards - Update complete"));
}

void UC__InventoryList::SetSortType(EInventorySortType NewSortType)
{
    CurrentSortType = NewSortType;
    SortInventory();
    UpdateItemCards();
}

void UC__InventoryList::UpdateWeightDisplay()
{
    if (!WeightDisplayText || !CachedInventoryComponent)
    {
        return;
    }

    float CurrentWeight = CachedInventoryComponent->GetTotalWeight();
    float MaxCapacity = CachedInventoryComponent->GetMaxCarryingCapacity();

    FString WeightText;
    if (MaxCapacity == FLT_MAX)
    {
        // 無限積載量の場合
        WeightText = FString::Printf(TEXT("%.1fkg / ∞"), CurrentWeight);
    }
    else
    {
        // 有限積載量の場合
        WeightText = FString::Printf(TEXT("%.1fkg / %.1fkg"), CurrentWeight, MaxCapacity);
    }

    WeightDisplayText->SetText(FText::FromString(WeightText));
}

void UC__InventoryList::UpdateInventoryName()
{
    if (!InventoryNameText || !CachedInventoryComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryList::UpdateInventoryName - Missing requirements: NameText=%s, Component=%s"),
               InventoryNameText ? TEXT("Valid") : TEXT("Null"),
               CachedInventoryComponent ? TEXT("Valid") : TEXT("Null"));
        return;
    }

    FString InventoryName = TEXT("不明なインベントリ");
    
    // Get the owner of the inventory component
    AActor* Owner = CachedInventoryComponent->GetOwner();
    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateInventoryName - Owner: %s"), 
           Owner ? *Owner->GetClass()->GetName() : TEXT("Null"));
           
    if (Owner)
    {
        // First check if this is a Team inventory by OwnerId
        if (CachedInventoryComponent->OwnerId.StartsWith(TEXT("TeamInventory_")))
        {
            // Extract team index from OwnerId
            FString TeamIndexStr = CachedInventoryComponent->OwnerId.Replace(TEXT("TeamInventory_"), TEXT(""));
            int32 TeamIndex = FCString::Atoi(*TeamIndexStr);
            
            // Get team name from TeamComponent
            if (UTeamComponent* TeamComp = Owner->FindComponentByClass<UTeamComponent>())
            {
                TArray<FTeam> AllTeams = TeamComp->GetAllTeams();
                if (AllTeams.IsValidIndex(TeamIndex))
                {
                    FString CarrierName = AllTeams[TeamIndex].GetCarrierDisplayName();
                    
                    if (!AllTeams[TeamIndex].TeamName.IsEmpty())
                    {
                        InventoryName = FString::Printf(TEXT("%s の%s"), *AllTeams[TeamIndex].TeamName, *CarrierName);
                    }
                    else
                    {
                        FString TeamLetter = FString::Printf(TEXT("%c"), 'A' + TeamIndex);
                        InventoryName = FString::Printf(TEXT("チーム%s の%s"), *TeamLetter, *CarrierName);
                    }
                    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateInventoryName - Identified as Team %d inventory: %s (Carrier: %s)"), TeamIndex, *InventoryName, *CarrierName);
                }
            }
        }
        // Check if owner is PlayerController (Base storage)
        else if (Owner->IsA<APlayerController>())
        {
            InventoryName = TEXT("拠点の倉庫");
            UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateInventoryName - Identified as PlayerController (Base)"));
        }
        // Check if owner is IdleCharacter
        else if (AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(Owner))
        {
            InventoryName = Character->GetCharacterName_Implementation();
            UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateInventoryName - Identified as Character: %s (Object: %s)"), *InventoryName, *Character->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("InventoryList::UpdateInventoryName - Owner type not recognized: %s"), *Owner->GetClass()->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryList::UpdateInventoryName - Owner is null"));
    }

    UE_LOG(LogTemp, Log, TEXT("InventoryList::UpdateInventoryName - Setting name to: %s"), *InventoryName);
    InventoryNameText->SetText(FText::FromString(InventoryName));
}

void UC__InventoryList::SortInventory()
{
    if (!ItemManager)
    {
        return;
    }

    CachedInventorySlots.Sort([this](const FInventorySlot& A, const FInventorySlot& B)
    {
        switch (CurrentSortType)
        {
            case EInventorySortType::Name_Asc:
                return CompareByName(A, B, true);
            case EInventorySortType::Name_Desc:
                return CompareByName(A, B, false);
            case EInventorySortType::Weight_Asc:
                return CompareByWeight(A, B, true);
            case EInventorySortType::Weight_Desc:
                return CompareByWeight(A, B, false);
            case EInventorySortType::Quantity_Asc:
                return CompareByQuantity(A, B, true);
            case EInventorySortType::Quantity_Desc:
                return CompareByQuantity(A, B, false);
            case EInventorySortType::Type_Asc:
                return CompareByType(A, B, true);
            case EInventorySortType::Type_Desc:
                return CompareByType(A, B, false);
            case EInventorySortType::Value_Asc:
                return CompareByValue(A, B, true);
            case EInventorySortType::Value_Desc:
                return CompareByValue(A, B, false);
            default:
                return CompareByName(A, B, true);
        }
    });
}

void UC__InventoryList::ForceRefresh()
{
    RefreshInventoryList();
}

void UC__InventoryList::FilterByType(EItemTypeTable ItemType)
{
    bIsFiltered = true;
    FilterType = ItemType;
    RefreshInventoryList();
}

void UC__InventoryList::ClearFilter()
{
    bIsFiltered = false;
    RefreshInventoryList();
}

void UC__InventoryList::CreateItemCard(const FInventorySlot& InventorySlot)
{
    UE_LOG(LogTemp, Log, TEXT("InventoryList::CreateItemCard - Creating card for ItemId=%s, Quantity=%d"), 
           *InventorySlot.ItemId, InventorySlot.Quantity);
           
    if (!ItemCardPanel || !ItemCardWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryList::CreateItemCard - Missing requirements: Panel=%s, WidgetClass=%s"),
               ItemCardPanel ? TEXT("Valid") : TEXT("Null"),
               ItemCardWidgetClass ? TEXT("Valid") : TEXT("Null"));
        return;
    }

    // Create widget
    UC_ItemListCard* NewCard = CreateWidget<UC_ItemListCard>(this, ItemCardWidgetClass);
    if (!NewCard)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryList::CreateItemCard - Failed to create item card widget"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("InventoryList::CreateItemCard - Successfully created widget, initializing..."));

    // Initialize card with slot data
    NewCard->InitializeWithSlot(InventorySlot, CachedInventoryComponent);

    // Add to panel
    ItemCardPanel->AddChild(NewCard);
    ItemCardWidgets.Add(NewCard);
    
    UE_LOG(LogTemp, Log, TEXT("InventoryList::CreateItemCard - Card added to panel. Total cards: %d"), ItemCardWidgets.Num());
}

void UC__InventoryList::ClearItemCards()
{
    UE_LOG(LogTemp, Log, TEXT("InventoryList::ClearItemCards - Clearing %d existing cards"), ItemCardWidgets.Num());
    
    if (!ItemCardPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryList::ClearItemCards - ItemCardPanel is null"));
        return;
    }

    // Remove all cards from panel
    ItemCardPanel->ClearChildren();

    // Clear array
    ItemCardWidgets.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("InventoryList::ClearItemCards - Clear complete"));
}

void UC__InventoryList::OnInventoryItemChanged(const FString& ItemId, int32 NewQuantity)
{
    // Simple refresh for now - could be optimized to update only affected cards
    RefreshInventoryList();
}

bool UC__InventoryList::CompareByName(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const
{
    FItemDataRow* ItemA = GetItemData(A.ItemId);
    FItemDataRow* ItemB = GetItemData(B.ItemId);
    
    if (!ItemA || !ItemB)
    {
        return false;
    }

    FString NameA = ItemA->Name.ToString();
    FString NameB = ItemB->Name.ToString();

    return bAscending ? (NameA < NameB) : (NameA > NameB);
}

bool UC__InventoryList::CompareByWeight(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const
{
    FItemDataRow* ItemA = GetItemData(A.ItemId);
    FItemDataRow* ItemB = GetItemData(B.ItemId);
    
    if (!ItemA || !ItemB)
    {
        return false;
    }

    float WeightA = ItemA->Weight * A.Quantity;
    float WeightB = ItemB->Weight * B.Quantity;

    return bAscending ? (WeightA < WeightB) : (WeightA > WeightB);
}

bool UC__InventoryList::CompareByQuantity(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const
{
    return bAscending ? (A.Quantity < B.Quantity) : (A.Quantity > B.Quantity);
}

bool UC__InventoryList::CompareByType(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const
{
    FItemDataRow* ItemA = GetItemData(A.ItemId);
    FItemDataRow* ItemB = GetItemData(B.ItemId);
    
    if (!ItemA || !ItemB)
    {
        return false;
    }

    uint8 TypeA = (uint8)ItemA->ItemType;
    uint8 TypeB = (uint8)ItemB->ItemType;

    return bAscending ? (TypeA < TypeB) : (TypeA > TypeB);
}

bool UC__InventoryList::CompareByValue(const FInventorySlot& A, const FInventorySlot& B, bool bAscending) const
{
    FItemDataRow* ItemA = GetItemData(A.ItemId);
    FItemDataRow* ItemB = GetItemData(B.ItemId);
    
    if (!ItemA || !ItemB)
    {
        return false;
    }

    int32 ValueA = ItemA->BaseValue * A.Quantity;
    int32 ValueB = ItemB->BaseValue * B.Quantity;

    return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
}

FItemDataRow* UC__InventoryList::GetItemData(const FString& ItemId) const
{
    if (!ItemManager)
    {
        return nullptr;
    }

    // Create a static cache for item data
    static TMap<FString, FItemDataRow> ItemCache;
    
    if (FItemDataRow* CachedData = ItemCache.Find(ItemId))
    {
        return CachedData;
    }

    FItemDataRow ItemData;
    if (ItemManager->GetItemData(ItemId, ItemData))
    {
        ItemCache.Add(ItemId, ItemData);
        return &ItemCache[ItemId];
    }

    return nullptr;
}

void UC__InventoryList::BindInventoryEvents()
{
    if (!CachedInventoryComponent)
    {
        return;
    }

    // Bind to inventory change event
    CachedInventoryComponent->OnInventoryChanged.AddDynamic(this, &UC__InventoryList::OnInventoryItemChanged);
}

void UC__InventoryList::UnbindInventoryEvents()
{
    if (!CachedInventoryComponent)
    {
        return;
    }

    // Unbind from inventory change event
    CachedInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UC__InventoryList::OnInventoryItemChanged);
}

// ========== Sort ComboBox Functions ==========

void UC__InventoryList::InitializeSortComboBox()
{
    if (!SortComboBox)
    {
        return; // ComboBoxが存在しない場合は何もしない
    }

    // ComboBoxのオプションを自動設定
    SortComboBox->ClearOptions();
    SortComboBox->AddOption(TEXT("名前（昇順）"));
    SortComboBox->AddOption(TEXT("名前（降順）"));
    SortComboBox->AddOption(TEXT("重さ（軽い順）"));
    SortComboBox->AddOption(TEXT("重さ（重い順）"));
    SortComboBox->AddOption(TEXT("個数（少ない順）"));
    SortComboBox->AddOption(TEXT("個数（多い順）"));
    SortComboBox->AddOption(TEXT("種類（昇順）"));
    SortComboBox->AddOption(TEXT("種類（降順）"));
    SortComboBox->AddOption(TEXT("価値（安い順）"));
    SortComboBox->AddOption(TEXT("価値（高い順）"));

    // デフォルト選択
    SortComboBox->SetSelectedOption(TEXT("名前（昇順）"));

    // イベントバインド
    SortComboBox->OnSelectionChanged.AddDynamic(this, &UC__InventoryList::OnSortComboBoxSelectionChanged);
}

void UC__InventoryList::OnSortComboBoxSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::Direct)
    {
        return; // プログラムによる変更は無視
    }

    // 文字列からソートタイプに変換
    EInventorySortType NewSortType = EInventorySortType::Name_Asc; // デフォルト

    if (SelectedItem == TEXT("名前（昇順）"))
    {
        NewSortType = EInventorySortType::Name_Asc;
    }
    else if (SelectedItem == TEXT("名前（降順）"))
    {
        NewSortType = EInventorySortType::Name_Desc;
    }
    else if (SelectedItem == TEXT("重さ（軽い順）"))
    {
        NewSortType = EInventorySortType::Weight_Asc;
    }
    else if (SelectedItem == TEXT("重さ（重い順）"))
    {
        NewSortType = EInventorySortType::Weight_Desc;
    }
    else if (SelectedItem == TEXT("個数（少ない順）"))
    {
        NewSortType = EInventorySortType::Quantity_Asc;
    }
    else if (SelectedItem == TEXT("個数（多い順）"))
    {
        NewSortType = EInventorySortType::Quantity_Desc;
    }
    else if (SelectedItem == TEXT("種類（昇順）"))
    {
        NewSortType = EInventorySortType::Type_Asc;
    }
    else if (SelectedItem == TEXT("種類（降順）"))
    {
        NewSortType = EInventorySortType::Type_Desc;
    }
    else if (SelectedItem == TEXT("価値（安い順）"))
    {
        NewSortType = EInventorySortType::Value_Asc;
    }
    else if (SelectedItem == TEXT("価値（高い順）"))
    {
        NewSortType = EInventorySortType::Value_Desc;
    }

    // ソートタイプを適用
    SetSortType(NewSortType);
}
