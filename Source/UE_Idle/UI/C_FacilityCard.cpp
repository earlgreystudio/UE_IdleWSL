#include "C_FacilityCard.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "../Components/BaseComponent.h"
#include "../Managers/FacilityManager.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Engine/AssetManager.h"

void UC_FacilityCard::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("UC_FacilityCard::NativeConstruct - Facility card constructed"));


    // ウィジェットクラスの自動設定
    if (!EffectTextBlockClass)
    {
        EffectTextBlockClass = UTextBlock::StaticClass();
    }

    if (!CostTextBlockClass)
    {
        CostTextBlockClass = UTextBlock::StaticClass();
    }
}

void UC_FacilityCard::NativeDestruct()
{
    Super::NativeDestruct();
}

void UC_FacilityCard::InitializeWithFacility(const FFacilityInstance& InFacilityInstance, UBaseComponent* InBaseComponent)
{
    FacilityInstance = InFacilityInstance;
    FacilityInstanceId = InFacilityInstance.InstanceId;
    BaseComponent = InBaseComponent;
    bHasValidFacilityData = false;

    // FacilityManagerを取得
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        FacilityManager = GameInstance->GetSubsystem<UFacilityManager>();
    }

    // 設備マスターデータを取得（1回のみ）
    if (FacilityManager)
    {
        if (FacilityManager->GetFacilityData(FacilityInstance.FacilityId, FacilityData))
        {
            bHasValidFacilityData = true;
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("UC_FacilityCard::InitializeWithFacility - No DataTable entry for test facility: %s, using default data"), *FacilityInstance.FacilityId);
            // テスト設備用のデフォルトデータを作成
            CreateDefaultFacilityData();
            bHasValidFacilityData = false;  // DataTableには無いが表示は可能
        }
    }

    // 初回表示更新
    UpdateDisplay();

    UE_LOG(LogTemp, Warning, TEXT("UC_FacilityCard::InitializeWithFacility - Initialized with facility: %s"), *FacilityInstance.FacilityId);
}

void UC_FacilityCard::UpdateDisplay()
{
    if (!FacilityInstanceId.IsValid())
    {
        return;
    }

    // 最新の設備インスタンス情報を取得
    if (FacilityManager)
    {
        FacilityInstance = FacilityManager->GetFacilityInstance(FacilityInstanceId);
        
        // DataTableデータはInitialize時に取得済みなのでここでは問い合わせしない
        // bHasValidFacilityDataがfalseの場合はすでにデフォルトデータが設定済み
    }

    UpdateBasicInfo();
    UpdateIconDisplay();
    UpdateStateInfo();
    UpdateDurabilityInfo();
    UpdateWorkerInfo();
    UpdateEffectsDisplay();
    UpdateCostsDisplay();
    UpdateDependenciesDisplay();
}

EFacilityState UC_FacilityCard::GetFacilityState() const
{
    return FacilityInstance.State;
}

EFacilityType UC_FacilityCard::GetFacilityType() const
{
    return FacilityData.FacilityType;
}

void UC_FacilityCard::UpdateBasicInfo()
{
    // 設備名
    if (FacilityNameText)
    {
        FacilityNameText->SetText(FacilityData.Name);
    }

    // レベル
    if (FacilityLevelText)
    {
        FString LevelText = FString::Printf(TEXT("Lv.%d"), FacilityInstance.Level);
        FacilityLevelText->SetText(FText::FromString(LevelText));
    }

    // タイプ
    if (FacilityTypeText)
    {
        FString TypeText = GetTypeDisplayText();
        FacilityTypeText->SetText(FText::FromString(TypeText));
    }

}

void UC_FacilityCard::UpdateIconDisplay()
{
    if (!FacilityIconImage)
    {
        return;
    }

    // IconPathが設定されている場合のみアイコンを表示
    if (!FacilityData.IconPath.IsEmpty())
    {
        // アセットパスからソフトオブジェクトポインタを作成
        TSoftObjectPtr<UTexture2D> SoftIconTexture(FacilityData.IconPath);
        
        // アセットが存在するかチェック
        if (SoftIconTexture.IsValid())
        {
            UTexture2D* IconTexture = SoftIconTexture.LoadSynchronous();
            if (IconTexture)
            {
                FacilityIconImage->SetBrushFromTexture(IconTexture);
                FacilityIconImage->SetVisibility(ESlateVisibility::Visible);
                UE_LOG(LogTemp, Log, TEXT("UC_FacilityCard::UpdateIconDisplay - Icon loaded: %s"), *FacilityData.IconPath);
            }
            else
            {
                // ロードに失敗した場合は非表示
                FacilityIconImage->SetVisibility(ESlateVisibility::Collapsed);
                UE_LOG(LogTemp, Log, TEXT("UC_FacilityCard::UpdateIconDisplay - Icon load failed: %s"), *FacilityData.IconPath);
            }
        }
        else
        {
            // アセットが見つからない場合は非表示（警告レベル下げる）
            FacilityIconImage->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Log, TEXT("UC_FacilityCard::UpdateIconDisplay - Icon asset not found: %s (optional)"), *FacilityData.IconPath);
        }
    }
    else
    {
        // IconPathが設定されていない場合は非表示（エラーではない）
        FacilityIconImage->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Log, TEXT("UC_FacilityCard::UpdateIconDisplay - No icon path set for %s (optional)"), *FacilityInstance.FacilityId);
    }
}

void UC_FacilityCard::UpdateStateInfo()
{
    // 状態表示
    if (FacilityStateText)
    {
        FString StateText = GetStateDisplayText();
        FacilityStateText->SetText(FText::FromString(StateText));
        
        // 状態に応じた色設定
        FLinearColor StateColor = GetStateColor();
        FacilityStateText->SetColorAndOpacity(FSlateColor(StateColor));
    }

    // 進捗バー
    if (ConstructionProgressBar)
    {
        float Progress = 0.0f;
        bool bShowProgress = false;

        if (FacilityInstance.State == EFacilityState::Constructing || 
            FacilityInstance.State == EFacilityState::Upgrading)
        {
            Progress = FacilityInstance.CompletionProgress / 100.0f; // 0-100を0-1に変換
            bShowProgress = true;
        }

        ConstructionProgressBar->SetPercent(Progress);
        ConstructionProgressBar->SetVisibility(bShowProgress ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 進捗パーセント
    if (ProgressPercentText)
    {
        if (FacilityInstance.State == EFacilityState::Constructing || 
            FacilityInstance.State == EFacilityState::Upgrading)
        {
            FString PercentText = FString::Printf(TEXT("%.1f%%"), FacilityInstance.CompletionProgress);
            ProgressPercentText->SetText(FText::FromString(PercentText));
            ProgressPercentText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            ProgressPercentText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UC_FacilityCard::UpdateDurabilityInfo()
{
    // 最大耐久度を計算
    int32 MaxDurability = FacilityData.GetMaxDurabilityAtLevel(FacilityInstance.Level);
    
    // 耐久度バー
    if (DurabilityBar)
    {
        float DurabilityPercent = MaxDurability > 0 ? (float)FacilityInstance.CurrentDurability / MaxDurability : 0.0f;
        DurabilityBar->SetPercent(DurabilityPercent);
        
        // 耐久度に応じた色設定
        FLinearColor DurabilityColor = FLinearColor::Green;
        if (DurabilityPercent < 0.3f)
        {
            DurabilityColor = FLinearColor::Red;
        }
        else if (DurabilityPercent < 0.7f)
        {
            DurabilityColor = FLinearColor::Yellow;
        }
        
        DurabilityBar->SetFillColorAndOpacity(DurabilityColor);
    }

    // 耐久度テキスト
    if (DurabilityText)
    {
        FString DurabilityString = FString::Printf(TEXT("%d / %d"), FacilityInstance.CurrentDurability, MaxDurability);
        DurabilityText->SetText(FText::FromString(DurabilityString));
    }
}

void UC_FacilityCard::UpdateWorkerInfo()
{
    // ワーカー数表示
    if (WorkersText)
    {
        FString WorkerText = FString::Printf(TEXT("ワーカー: %d / %d"), 
            FacilityInstance.AssignedWorkers, FacilityData.RequiredWorkers);
        WorkersText->SetText(FText::FromString(WorkerText));
    }

    // ワーカー効率表示
    if (WorkerEfficiencyText)
    {
        // TODO: ワーカー効率の計算ロジック実装
        float Efficiency = FacilityInstance.AssignedWorkers > 0 ? 
            (100.0f + (FacilityInstance.AssignedWorkers - 1) * 20.0f) : 0.0f;
        
        FString EfficiencyText = FString::Printf(TEXT("効率: %.0f%%"), Efficiency);
        WorkerEfficiencyText->SetText(FText::FromString(EfficiencyText));
    }
}

void UC_FacilityCard::UpdateEffectsDisplay()
{
    UpdateContinuousEffects();
    UpdateUnlockEffects();
    UpdateAutoProduction();
}

void UC_FacilityCard::UpdateContinuousEffects()
{
    if (!ContinuousEffectsContainer)
    {
        return;
    }

    ContinuousEffectsContainer->ClearChildren();

    for (const FFacilityEffect& Effect : FacilityData.Effects)
    {
        // 継続効果のみ表示
        if (Effect.EffectType == EFacilityEffectType::ProductionSpeed ||
            Effect.EffectType == EFacilityEffectType::ResourceGain ||
            Effect.EffectType == EFacilityEffectType::StorageCapacity ||
            Effect.EffectType == EFacilityEffectType::PopulationCap ||
            Effect.EffectType == EFacilityEffectType::MoraleBonus ||
            Effect.EffectType == EFacilityEffectType::DefenseBonus ||
            Effect.EffectType == EFacilityEffectType::ResearchSpeed ||
            Effect.EffectType == EFacilityEffectType::TrainingSpeed)
        {
            float EffectValue = FacilityData.GetEffectValueAtLevel(Effect, FacilityInstance.Level);
            if (EffectValue > 0.0f)
            {
                UTextBlock* EffectText = NewObject<UTextBlock>(this);
                if (EffectText)
                {
                    FString EffectString = FString::Printf(TEXT("%s: +%.1f"), 
                        *GetEffectDisplayName(Effect.EffectType), EffectValue);
                    EffectText->SetText(FText::FromString(EffectString));
                    ContinuousEffectsContainer->AddChildToVerticalBox(EffectText);
                }
            }
        }
    }
}

void UC_FacilityCard::UpdateUnlockEffects()
{
    if (!UnlockEffectsContainer)
    {
        return;
    }

    UnlockEffectsContainer->ClearChildren();

    for (const FFacilityEffect& Effect : FacilityData.Effects)
    {
        // アンロック効果のみ表示
        if (Effect.EffectType == EFacilityEffectType::UnlockRecipe ||
            Effect.EffectType == EFacilityEffectType::UnlockItem ||
            Effect.EffectType == EFacilityEffectType::UnlockFacility)
        {
            if (FacilityInstance.Level >= Effect.RequiredLevel)
            {
                UTextBlock* UnlockText = NewObject<UTextBlock>(this);
                if (UnlockText)
                {
                    FString UnlockString = FString::Printf(TEXT("解放: %s"), *Effect.TargetId);
                    UnlockText->SetText(FText::FromString(UnlockString));
                    UnlockEffectsContainer->AddChildToVerticalBox(UnlockText);
                }
            }
        }
    }
}

void UC_FacilityCard::UpdateAutoProduction()
{
    if (!AutoProductionContainer)
    {
        return;
    }

    AutoProductionContainer->ClearChildren();

    for (const FFacilityEffect& Effect : FacilityData.Effects)
    {
        if (Effect.EffectType == EFacilityEffectType::AutoProduction)
        {
            float ProductionRate = FacilityData.GetEffectValueAtLevel(Effect, FacilityInstance.Level);
            if (ProductionRate > 0.0f)
            {
                UTextBlock* ProductionText = NewObject<UTextBlock>(this);
                if (ProductionText)
                {
                    FString ProductionString = FString::Printf(TEXT("自動生産: %s %.1f/分"), 
                        *Effect.TargetId, ProductionRate);
                    ProductionText->SetText(FText::FromString(ProductionString));
                    AutoProductionContainer->AddChildToVerticalBox(ProductionText);
                }
            }
        }
    }
}

void UC_FacilityCard::UpdateCostsDisplay()
{
    UpdateUpgradeCosts();
}

void UC_FacilityCard::UpdateUpgradeCosts()
{
    if (!UpgradeCostsContainer)
    {
        return;
    }

    UpgradeCostsContainer->ClearChildren();

    // 次のレベルへのアップグレードコストを表示
    if (FacilityInstance.Level < FacilityData.MaxLevel)
    {
        int32 NextLevel = FacilityInstance.Level + 1;
        
        for (const FFacilityResourceCost& Cost : FacilityData.UpgradeCosts)
        {
            int32 RequiredAmount = FacilityData.GetResourceCostAtLevel(Cost, NextLevel);
            
            UTextBlock* CostText = NewObject<UTextBlock>(this);
            if (CostText)
            {
                FString CostString = FString::Printf(TEXT("%s: %d"), *Cost.ItemId, RequiredAmount);
                CostText->SetText(FText::FromString(CostString));
                UpgradeCostsContainer->AddChildToVerticalBox(CostText);
            }
        }
    }
}


void UC_FacilityCard::UpdateDependenciesDisplay()
{
    UpdateDependencies();
    UpdateUnlocks();
}

void UC_FacilityCard::UpdateDependencies()
{
    if (!DependenciesContainer)
    {
        return;
    }

    DependenciesContainer->ClearChildren();

    for (const FFacilityDependency& Dependency : FacilityData.Dependencies)
    {
        UTextBlock* DependencyText = NewObject<UTextBlock>(this);
        if (DependencyText)
        {
            FString DependencyString = FString::Printf(TEXT("必要: %s Lv.%d"), 
                *Dependency.RequiredFacilityId, Dependency.RequiredLevel);
            DependencyText->SetText(FText::FromString(DependencyString));
            DependenciesContainer->AddChildToVerticalBox(DependencyText);
        }
    }
}

void UC_FacilityCard::UpdateUnlocks()
{
    if (!UnlocksContainer)
    {
        return;
    }

    UnlocksContainer->ClearChildren();
    
    // TODO: この設備が前提となる他の設備を検索して表示
    // FacilityManagerから全設備データを取得して依存関係をチェック
}


FString UC_FacilityCard::GetStateDisplayText() const
{
    switch (FacilityInstance.State)
    {
        case EFacilityState::Planning: return TEXT("計画中");
        case EFacilityState::Constructing: return TEXT("建設中");
        case EFacilityState::Active: return TEXT("稼働中");
        case EFacilityState::Damaged: return TEXT("損傷");
        case EFacilityState::Broken: return TEXT("破壊");
        case EFacilityState::Upgrading: return TEXT("アップグレード中");
        default: return TEXT("不明");
    }
}

FString UC_FacilityCard::GetTypeDisplayText() const
{
    switch (FacilityData.FacilityType)
    {
        case EFacilityType::Housing: return TEXT("住居");
        case EFacilityType::Production: return TEXT("生産施設");
        case EFacilityType::Military: return TEXT("軍事施設");
        case EFacilityType::Research: return TEXT("研究施設");
        case EFacilityType::Storage: return TEXT("貯蔵施設");
        case EFacilityType::Utility: return TEXT("ユーティリティ");
        default: return TEXT("不明");
    }
}

FString UC_FacilityCard::GetEffectDisplayName(EFacilityEffectType EffectType) const
{
    switch (EffectType)
    {
        case EFacilityEffectType::ProductionSpeed: return TEXT("生産速度");
        case EFacilityEffectType::ResourceGain: return TEXT("資源獲得量");
        case EFacilityEffectType::StorageCapacity: return TEXT("貯蔵容量");
        case EFacilityEffectType::PopulationCap: return TEXT("人口上限");
        case EFacilityEffectType::MoraleBonus: return TEXT("士気");
        case EFacilityEffectType::DefenseBonus: return TEXT("防御力");
        case EFacilityEffectType::ResearchSpeed: return TEXT("研究速度");
        case EFacilityEffectType::TrainingSpeed: return TEXT("訓練速度");
        default: return TEXT("効果");
    }
}

FLinearColor UC_FacilityCard::GetStateColor() const
{
    switch (FacilityInstance.State)
    {
        case EFacilityState::Planning: return FLinearColor::Gray;
        case EFacilityState::Constructing: return FLinearColor::Yellow;
        case EFacilityState::Active: return FLinearColor::Green;
        case EFacilityState::Damaged: return FLinearColor(1.0f, 0.5f, 0.0f); // オレンジ
        case EFacilityState::Broken: return FLinearColor::Red;
        case EFacilityState::Upgrading: return FLinearColor::Blue;
        default: return FLinearColor::White;
    }
}

void UC_FacilityCard::CreateDefaultFacilityData()
{
    // テスト設備用のデフォルトデータを作成
    FacilityData.Name = FText::FromString(FacilityInstance.FacilityId);
    FacilityData.Description = FText::FromString(TEXT("Test Facility"));
    FacilityData.FacilityType = EFacilityType::Production;
    FacilityData.MaxLevel = 5;
    FacilityData.BaseDurability = 100;
    FacilityData.DurabilityPerLevel = 20;
    FacilityData.BaseConstructionTime = 60.0f;
    FacilityData.TimePerLevel = 30.0f;
    FacilityData.RequiredWorkers = 2;
    
    // アイコンとメッシュパスは空のまま（オプション）
    FacilityData.IconPath = TEXT("");
    FacilityData.MeshPath = TEXT("");
    
    UE_LOG(LogTemp, Log, TEXT("UC_FacilityCard::CreateDefaultFacilityData - Created default data for %s (no icon/mesh)"), *FacilityInstance.FacilityId);
}


