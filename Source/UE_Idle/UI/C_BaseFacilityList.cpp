#include "C_BaseFacilityList.h"
#include "Components/ScrollBox.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "../Components/BaseComponent.h"
#include "../Managers/FacilityManager.h"
#include "C_FacilityCard.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UC_BaseFacilityList::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::NativeConstruct - Starting initialization"));

    // フィルタ初期化
    CurrentTypeFilter = EFacilityType::Housing; // "All"に相当する値がないので、Housingを仮の初期値
    CurrentStateFilter = EFacilityState::Planning; // "All"に相当する値がないので、Planningを仮の初期値

    // FacilityCardClassの自動設定
    if (!FacilityCardClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::NativeConstruct - FacilityCardClass not set, trying to auto-detect"));
        FacilityCardClass = UC_FacilityCard::StaticClass();
        if (FacilityCardClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::NativeConstruct - Auto-set FacilityCardClass"));
        }
    }

    // フィルタコンボボックスのバインド
    if (TypeFilterComboBox)
    {
        TypeFilterComboBox->OnSelectionChanged.AddDynamic(this, &UC_BaseFacilityList::OnTypeFilterChanged);
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::NativeConstruct - TypeFilterComboBox bound"));
    }

    if (StateFilterComboBox)
    {
        StateFilterComboBox->OnSelectionChanged.AddDynamic(this, &UC_BaseFacilityList::OnStateFilterChanged);
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::NativeConstruct - StateFilterComboBox bound"));
    }

    // 自動初期化を試行
    TryAutoInitialize();

    // フィルタコンボボックスの初期化
    UpdateFilterComboBoxes();
}

void UC_BaseFacilityList::NativeDestruct()
{
    // イベントバインド解除
    UnbindBaseComponentEvents();
    UnbindFacilityManagerEvents();

    Super::NativeDestruct();
}

void UC_BaseFacilityList::TryAutoInitialize()
{
    // PlayerControllerからBaseComponentを自動取得
    if (APlayerController* PC = GetOwningPlayer())
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::TryAutoInitialize - PlayerController found"));
        
        if (UBaseComponent* FoundBaseComponent = PC->GetComponentByClass<UBaseComponent>())
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::TryAutoInitialize - BaseComponent found, initializing"));
            InitializeWithBaseComponent(FoundBaseComponent);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::TryAutoInitialize - BaseComponent not found on PlayerController"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::TryAutoInitialize - PlayerController not found"));
    }
}

void UC_BaseFacilityList::InitializeWithBaseComponent(UBaseComponent* InBaseComponent)
{
    if (!InBaseComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::InitializeWithBaseComponent - InBaseComponent is null"));
        return;
    }

    // 既存のイベントバインドを解除
    UnbindBaseComponentEvents();
    UnbindFacilityManagerEvents();

    BaseComponent = InBaseComponent;

    // FacilityManagerを取得
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        FacilityManager = GameInstance->GetSubsystem<UFacilityManager>();
        if (FacilityManager)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::InitializeWithBaseComponent - FacilityManager found"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::InitializeWithBaseComponent - FacilityManager not found"));
        }
    }

    // イベントバインド
    BindBaseComponentEvents();
    BindFacilityManagerEvents();

    // 初回表示更新
    RefreshFacilityList();
}

void UC_BaseFacilityList::RefreshFacilityList()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - START"));

    if (!FacilityListContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::RefreshFacilityList - FacilityListContainer is null"));
        return;
    }

    if (!FacilityManager)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::RefreshFacilityList - FacilityManager is null"));
        return;
    }

    // 既存のカードをクリア
    FacilityListContainer->ClearChildren();
    FacilityCards.Empty();

    // 全設備を取得
    TArray<FFacilityInstance> AllFacilities = FacilityManager->GetAllFacilities();
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - Found %d facilities"), AllFacilities.Num());

    // フィルタを適用してカードを作成
    for (const FFacilityInstance& FacilityInstance : AllFacilities)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - Processing facility: %s, State: %d"), 
            *FacilityInstance.FacilityId, (int32)FacilityInstance.State);
            
        if (PassesCurrentFilters(FacilityInstance))
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - Facility passed filters: %s"), *FacilityInstance.FacilityId);
            
            UC_FacilityCard* NewCard = CreateFacilityCard(FacilityInstance);
            if (NewCard)
            {
                FacilityCards.Add(NewCard);
                FacilityListContainer->AddChild(NewCard);
                UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - Successfully added facility card: %s"), *FacilityInstance.FacilityId);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::RefreshFacilityList - Failed to create card for: %s"), *FacilityInstance.FacilityId);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - Facility filtered out: %s"), *FacilityInstance.FacilityId);
        }
    }

    // サマリーテキスト更新
    UpdateSummaryTexts();

    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RefreshFacilityList - COMPLETED"));
}

void UC_BaseFacilityList::FilterByType(EFacilityType FilterType)
{
    CurrentTypeFilter = FilterType;
    ApplyFiltersToExistingCards();
    UpdateSummaryTexts();
}

void UC_BaseFacilityList::FilterByState(EFacilityState FilterState)
{
    CurrentStateFilter = FilterState;
    ApplyFiltersToExistingCards();
    UpdateSummaryTexts();
}

void UC_BaseFacilityList::ClearFilters()
{
    // 初期値にリセット（現在の実装ではHousingとPlanningが初期値）
    CurrentTypeFilter = EFacilityType::Housing;
    CurrentStateFilter = EFacilityState::Planning;
    
    // コンボボックスも更新
    UpdateFilterComboBoxes();
    
    // フィルタを適用
    ApplyFiltersToExistingCards();
    UpdateSummaryTexts();
}

void UC_BaseFacilityList::BindBaseComponentEvents()
{
    if (!BaseComponent)
    {
        return;
    }

    BaseComponent->OnFacilityAdded.AddDynamic(this, &UC_BaseFacilityList::OnFacilityAdded);
    BaseComponent->OnFacilityRemoved.AddDynamic(this, &UC_BaseFacilityList::OnFacilityRemoved);
    // BaseComponentにリソース変更イベントがあれば追加
    // BaseComponent->OnResourceChanged.AddDynamic(this, &UC_BaseFacilityList::OnResourceChanged);

    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::BindBaseComponentEvents - Events bound"));
}

void UC_BaseFacilityList::UnbindBaseComponentEvents()
{
    if (!BaseComponent)
    {
        return;
    }

    BaseComponent->OnFacilityAdded.RemoveDynamic(this, &UC_BaseFacilityList::OnFacilityAdded);
    BaseComponent->OnFacilityRemoved.RemoveDynamic(this, &UC_BaseFacilityList::OnFacilityRemoved);
    // BaseComponent->OnResourceChanged.RemoveDynamic(this, &UC_BaseFacilityList::OnResourceChanged);
}

void UC_BaseFacilityList::BindFacilityManagerEvents()
{
    if (!FacilityManager)
    {
        return;
    }

    FacilityManager->OnFacilityStateChanged.AddDynamic(this, &UC_BaseFacilityList::OnFacilityStateChanged);
    FacilityManager->OnFacilityLevelChanged.AddDynamic(this, &UC_BaseFacilityList::OnFacilityLevelChanged);

    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::BindFacilityManagerEvents - Events bound"));
}

void UC_BaseFacilityList::UnbindFacilityManagerEvents()
{
    if (!FacilityManager)
    {
        return;
    }

    FacilityManager->OnFacilityStateChanged.RemoveDynamic(this, &UC_BaseFacilityList::OnFacilityStateChanged);
    FacilityManager->OnFacilityLevelChanged.RemoveDynamic(this, &UC_BaseFacilityList::OnFacilityLevelChanged);
}

void UC_BaseFacilityList::OnFacilityAdded(const FGuid& InstanceId, const FString& FacilityId)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::OnFacilityAdded - Facility added: %s"), *FacilityId);
    
    if (!FacilityManager)
    {
        return;
    }

    // 新しい設備インスタンスを取得
    FFacilityInstance NewFacility = FacilityManager->GetFacilityInstance(InstanceId);
    
    if (PassesCurrentFilters(NewFacility))
    {
        UC_FacilityCard* NewCard = CreateFacilityCard(NewFacility);
        if (NewCard)
        {
            FacilityCards.Add(NewCard);
            FacilityListContainer->AddChild(NewCard);
            UpdateSummaryTexts();
        }
    }
}

void UC_BaseFacilityList::OnFacilityRemoved(const FGuid& InstanceId)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::OnFacilityRemoved - Facility removed"));
    
    RemoveFacilityCard(InstanceId);
    UpdateSummaryTexts();
}

void UC_BaseFacilityList::OnFacilityStateChanged(const FGuid& InstanceId, EFacilityState OldState, EFacilityState NewState)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::OnFacilityStateChanged - State changed for facility"));
    
    if (UC_FacilityCard* Card = FindFacilityCard(InstanceId))
    {
        Card->UpdateDisplay();
        
        // フィルタチェック（状態変更により表示/非表示が変わる可能性）
        FFacilityInstance FacilityInstance = FacilityManager->GetFacilityInstance(InstanceId);
        if (!PassesCurrentFilters(FacilityInstance))
        {
            // フィルタに合わなくなった場合は非表示
            Card->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            Card->SetVisibility(ESlateVisibility::Visible);
        }
    }
    
    UpdateSummaryTexts();
}

void UC_BaseFacilityList::OnFacilityLevelChanged(const FGuid& InstanceId, int32 NewLevel)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::OnFacilityLevelChanged - Level changed for facility to %d"), NewLevel);
    
    if (UC_FacilityCard* Card = FindFacilityCard(InstanceId))
    {
        Card->UpdateDisplay();
    }
}

void UC_BaseFacilityList::OnResourceChanged(const FString& ResourceId, int32 NewAmount)
{
    // リソース変更時の処理（必要に応じて実装）
    // 例：アップグレード可能性の更新など
    for (UC_FacilityCard* Card : FacilityCards)
    {
        if (Card)
        {
            Card->UpdateDisplay();
        }
    }
}

void UC_BaseFacilityList::OnTypeFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::OnTypeFilterChanged - %s"), *SelectedItem);
    
    // 文字列から設備分類に変換（実装が必要）
    // 仮実装：全タイプとして扱う
    ApplyFiltersToExistingCards();
    UpdateSummaryTexts();
}

void UC_BaseFacilityList::OnStateFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::OnStateFilterChanged - %s"), *SelectedItem);
    
    // 文字列から設備状態に変換（実装が必要）
    // 仮実装：全状態として扱う
    ApplyFiltersToExistingCards();
    UpdateSummaryTexts();
}

UC_FacilityCard* UC_BaseFacilityList::CreateFacilityCard(const FFacilityInstance& FacilityInstance)
{
    if (!FacilityCardClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::CreateFacilityCard - FacilityCardClass is null"));
        return nullptr;
    }

    UC_FacilityCard* NewCard = CreateWidget<UC_FacilityCard>(this, FacilityCardClass);
    if (NewCard)
    {
        NewCard->InitializeWithFacility(FacilityInstance, BaseComponent);
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::CreateFacilityCard - Created card for %s"), *FacilityInstance.FacilityId);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_BaseFacilityList::CreateFacilityCard - Failed to create widget"));
    }

    return NewCard;
}

void UC_BaseFacilityList::RemoveFacilityCard(const FGuid& InstanceId)
{
    for (int32 i = FacilityCards.Num() - 1; i >= 0; --i)
    {
        if (FacilityCards[i] && FacilityCards[i]->GetFacilityInstanceId() == InstanceId)
        {
            FacilityCards[i]->RemoveFromParent();
            FacilityCards.RemoveAt(i);
            UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::RemoveFacilityCard - Removed facility card"));
            break;
        }
    }
}

UC_FacilityCard* UC_BaseFacilityList::FindFacilityCard(const FGuid& InstanceId)
{
    for (UC_FacilityCard* Card : FacilityCards)
    {
        if (Card && Card->GetFacilityInstanceId() == InstanceId)
        {
            return Card;
        }
    }
    return nullptr;
}

bool UC_BaseFacilityList::PassesCurrentFilters(const FFacilityInstance& FacilityInstance) const
{
    if (!FacilityManager)
    {
        return true; // FacilityManagerがない場合はすべて通す
    }

    // 設備データを取得（テスト設備の場合は取得できなくても通す）
    FFacilityDataRow FacilityData;
    bool bHasFacilityData = FacilityManager->GetFacilityData(FacilityInstance.FacilityId, FacilityData);
    
    // テスト設備の場合はDataTableにないのでそのまま通す
    if (!bHasFacilityData && FacilityInstance.FacilityId.StartsWith(TEXT("test_facility")))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_BaseFacilityList::PassesCurrentFilters - Allowing test facility: %s"), *FacilityInstance.FacilityId);
        return true;
    }
    
    // 通常の設備でDataTableにない場合は除外
    if (!bHasFacilityData)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_BaseFacilityList::PassesCurrentFilters - No DataTable entry for: %s"), *FacilityInstance.FacilityId);
        return false;
    }

    // タイプフィルタ（すべてのタイプを表示する仮実装）
    // TODO: "All"タイプが定義されたら、それ以外の場合はタイプチェックを行う

    // 状態フィルタ（すべての状態を表示する仮実装）
    // TODO: "All"状態が定義されたら、それ以外の場合は状態チェックを行う

    return true;
}

void UC_BaseFacilityList::ApplyFiltersToExistingCards()
{
    if (!FacilityManager)
    {
        return;
    }

    for (UC_FacilityCard* Card : FacilityCards)
    {
        if (Card)
        {
            FFacilityInstance FacilityInstance = FacilityManager->GetFacilityInstance(Card->GetFacilityInstanceId());
            
            if (PassesCurrentFilters(FacilityInstance))
            {
                Card->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                Card->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

void UC_BaseFacilityList::UpdateFilterComboBoxes()
{
    // タイプフィルタコンボボックスの設定
    if (TypeFilterComboBox)
    {
        TypeFilterComboBox->ClearOptions();
        TypeFilterComboBox->AddOption(TEXT("すべて"));
        TypeFilterComboBox->AddOption(TEXT("住居"));
        TypeFilterComboBox->AddOption(TEXT("生産施設"));
        TypeFilterComboBox->AddOption(TEXT("軍事施設"));
        TypeFilterComboBox->AddOption(TEXT("研究施設"));
        TypeFilterComboBox->AddOption(TEXT("貯蔵施設"));
        TypeFilterComboBox->AddOption(TEXT("ユーティリティ"));
        TypeFilterComboBox->SetSelectedIndex(0); // デフォルト：すべて
    }

    // 状態フィルタコンボボックスの設定
    if (StateFilterComboBox)
    {
        StateFilterComboBox->ClearOptions();
        StateFilterComboBox->AddOption(TEXT("すべて"));
        StateFilterComboBox->AddOption(TEXT("計画中"));
        StateFilterComboBox->AddOption(TEXT("建設中"));
        StateFilterComboBox->AddOption(TEXT("稼働中"));
        StateFilterComboBox->AddOption(TEXT("損傷"));
        StateFilterComboBox->AddOption(TEXT("破壊"));
        StateFilterComboBox->AddOption(TEXT("アップグレード中"));
        StateFilterComboBox->SetSelectedIndex(0); // デフォルト：すべて
    }
}

void UC_BaseFacilityList::UpdateSummaryTexts()
{
    if (!FacilityManager)
    {
        return;
    }

    TArray<FFacilityInstance> AllFacilities = FacilityManager->GetAllFacilities();
    int32 TotalCount = AllFacilities.Num();
    int32 ActiveCount = 0;

    // 稼働中の設備をカウント
    for (const FFacilityInstance& Facility : AllFacilities)
    {
        if (Facility.State == EFacilityState::Active)
        {
            ActiveCount++;
        }
    }

    // 表示更新
    if (TotalFacilitiesText)
    {
        FString TotalText = FString::Printf(TEXT("総設備数: %d"), TotalCount);
        TotalFacilitiesText->SetText(FText::FromString(TotalText));
    }

    if (ActiveFacilitiesText)
    {
        FString ActiveText = FString::Printf(TEXT("稼働中: %d"), ActiveCount);
        ActiveFacilitiesText->SetText(FText::FromString(ActiveText));
    }
}