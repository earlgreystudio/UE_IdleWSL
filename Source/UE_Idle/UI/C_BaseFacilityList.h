#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/FacilityDataTable.h"
#include "C_BaseFacilityList.generated.h"

class UScrollBox;
class UC_FacilityCard;
class UBaseComponent;
class UFacilityManager;
class UComboBoxString;
class UTextBlock;

/**
 * 拠点設備一覧表示ウィジェット
 * FacilityCardを並べて設備の状態を一覧表示
 */
UCLASS()
class UE_IDLE_API UC_BaseFacilityList : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // BaseComponentとの連携
    UFUNCTION(BlueprintCallable, Category = "Base Facility List")
    void InitializeWithBaseComponent(UBaseComponent* InBaseComponent);

    // 設備リストの更新
    UFUNCTION(BlueprintCallable, Category = "Base Facility List")
    void RefreshFacilityList();

    // フィルタリング機能
    UFUNCTION(BlueprintCallable, Category = "Base Facility List")
    void FilterByType(EFacilityType FilterType);

    UFUNCTION(BlueprintCallable, Category = "Base Facility List")
    void FilterByState(EFacilityState FilterState);

    UFUNCTION(BlueprintCallable, Category = "Base Facility List")
    void ClearFilters();

protected:
    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UScrollBox* FacilityListContainer;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* TypeFilterComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* StateFilterComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* TotalFacilitiesText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ActiveFacilitiesText;

    // 設備カードのウィジェットクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Facility List")
    TSubclassOf<UC_FacilityCard> FacilityCardClass;

    // マネージャーへの参照
    UPROPERTY()
    UBaseComponent* BaseComponent;

    UPROPERTY()
    UFacilityManager* FacilityManager;

    // 作成済み設備カードのリスト
    UPROPERTY()
    TArray<UC_FacilityCard*> FacilityCards;

    // フィルタ設定
    UPROPERTY()
    EFacilityType CurrentTypeFilter;

    UPROPERTY()
    EFacilityState CurrentStateFilter;

private:
    // PlayerControllerから自動初期化
    void TryAutoInitialize();

    // イベントハンドラー
    UFUNCTION()
    void OnFacilityAdded(const FGuid& InstanceId, const FString& FacilityId);

    UFUNCTION()
    void OnFacilityRemoved(const FGuid& InstanceId);

    UFUNCTION()
    void OnFacilityStateChanged(const FGuid& InstanceId, EFacilityState OldState, EFacilityState NewState);

    UFUNCTION()
    void OnFacilityLevelChanged(const FGuid& InstanceId, int32 NewLevel);

    UFUNCTION()
    void OnResourceChanged(const FString& ResourceId, int32 NewAmount);

    // フィルタコンボボックスハンドラー
    UFUNCTION()
    void OnTypeFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnStateFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    // 設備カード作成・管理
    UC_FacilityCard* CreateFacilityCard(const FFacilityInstance& FacilityInstance);
    void RemoveFacilityCard(const FGuid& InstanceId);
    UC_FacilityCard* FindFacilityCard(const FGuid& InstanceId);

    // フィルタ処理
    bool PassesCurrentFilters(const FFacilityInstance& FacilityInstance) const;
    void ApplyFiltersToExistingCards();

    // UI更新
    void UpdateFilterComboBoxes();
    void UpdateSummaryTexts();

    // イベントバインディング
    void BindBaseComponentEvents();
    void UnbindBaseComponentEvents();
    void BindFacilityManagerEvents();
    void UnbindFacilityManagerEvents();
};