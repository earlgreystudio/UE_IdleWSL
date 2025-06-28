#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/FacilityDataTable.h"
#include "C_FacilityCard.generated.h"

class UTextBlock;
class UProgressBar;
class UImage;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UBaseComponent;
class UFacilityManager;

/**
 * 設備カードウィジェット
 * 単一設備の詳細情報を表示
 */
UCLASS()
class UE_IDLE_API UC_FacilityCard : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // 設備情報の設定
    UFUNCTION(BlueprintCallable, Category = "Facility Card")
    void InitializeWithFacility(const FFacilityInstance& InFacilityInstance, UBaseComponent* InBaseComponent);

    // 表示更新
    UFUNCTION(BlueprintCallable, Category = "Facility Card")
    void UpdateDisplay();

    // アクセサー
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Facility Card")
    FGuid GetFacilityInstanceId() const { return FacilityInstanceId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Facility Card")
    EFacilityState GetFacilityState() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Facility Card")
    EFacilityType GetFacilityType() const;

protected:
    // === 基本情報表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* FacilityNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* FacilityLevelText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* FacilityTypeText;

    // === アイコン表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* FacilityIconImage;


    // === 状態表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* FacilityStateText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UProgressBar* ConstructionProgressBar;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ProgressPercentText;

    // === 耐久度表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UProgressBar* DurabilityBar;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* DurabilityText;

    // === ワーカー情報 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* WorkersText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* WorkerEfficiencyText;

    // === 効果表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* ContinuousEffectsContainer;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* UnlockEffectsContainer;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* AutoProductionContainer;

    // === コスト表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* UpgradeCostsContainer;

    // === 依存関係表示 ===
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* DependenciesContainer;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* UnlocksContainer;


    // 設備データ
    UPROPERTY(BlueprintReadOnly, Category = "Facility Card")
    FGuid FacilityInstanceId;

    UPROPERTY(BlueprintReadOnly, Category = "Facility Card")
    FFacilityInstance FacilityInstance;

    UPROPERTY(BlueprintReadOnly, Category = "Facility Card")
    FFacilityDataRow FacilityData;

    // DataTable問い合わせ結果をキャッシュ
    UPROPERTY()
    bool bHasValidFacilityData;

    // マネージャーへの参照
    UPROPERTY()
    UBaseComponent* BaseComponent;

    UPROPERTY()
    UFacilityManager* FacilityManager;

    // サブウィジェットクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Facility Card")
    TSubclassOf<UTextBlock> EffectTextBlockClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Facility Card")
    TSubclassOf<UTextBlock> CostTextBlockClass;

private:
    // === 表示更新メソッド ===
    void UpdateBasicInfo();
    void UpdateIconDisplay();
    void UpdateStateInfo();
    void UpdateDurabilityInfo();
    void UpdateWorkerInfo();
    void UpdateEffectsDisplay();
    void UpdateCostsDisplay();
    void UpdateDependenciesDisplay();

    // === 効果表示の詳細 ===
    void UpdateContinuousEffects();
    void UpdateUnlockEffects();
    void UpdateAutoProduction();

    // === コスト表示の詳細 ===
    void UpdateUpgradeCosts();

    // === 依存関係表示の詳細 ===
    void UpdateDependencies();
    void UpdateUnlocks();

    // === 進捗計算 ===
    float GetConstructionProgress() const;
    float GetUpgradeProgress() const;

    // === ヘルパー関数 ===
    FString GetStateDisplayText() const;
    FString GetTypeDisplayText() const;
    FString GetEffectDisplayName(EFacilityEffectType EffectType) const;
    FLinearColor GetStateColor() const;
    void CreateDefaultFacilityData();


};