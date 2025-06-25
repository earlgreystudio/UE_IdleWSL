#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/EventLogTypes.h"
#include "EventLogEntryWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;

UCLASS()
class UE_IDLE_API UEventLogEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // インデックスを設定してデータを初期化
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void SetEventIndex(int32 Index);
    
    // 展開/折りたたみ切り替え
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    bool ToggleDetails();
    
    // 展開状態を取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    bool IsExpanded() const { return bIsExpanded; }

protected:
    // Widget開始時の初期化
    virtual void NativeConstruct() override;
    
    // ボタンクリックイベント（内部用）
    UFUNCTION()
    void OnDetailsButtonClicked();
    
    // テキストを更新する関数（Blueprintから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateTexts(UTextBlock* TitleText, UTextBlock* ResultText, UTextBlock* DetailsText);
    
    // 詳細パネルの表示/非表示（Blueprintから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void SetDetailsPanelVisibility(UVerticalBox* DetailsPanel, bool bVisible);
    
    // ボタンテキスト更新（Blueprintから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateButtonText(UButton* Button, bool bExpanded);

private:
    // 保持データ
    UPROPERTY()
    int32 EventIndex = -1;
    
    UPROPERTY()
    FEventSummary CurrentSummary;
    
    UPROPERTY()
    bool bIsExpanded = false;
    
    // EventLogManagerへの参照取得
    class UEventLogManager* GetEventLogManager() const;
    
    // 詳細テキストのフォーマット
    FString FormatDetailsText() const;
};