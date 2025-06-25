#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/EventLogTypes.h"
#include "EventLogEntryWidget.generated.h"

class UTextBlock;
class URichTextBlock;
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
    
    // 強制的にUI更新
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void ForceUpdateUI();
    
    // タイトル取得（Blueprint用）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    FString GetCurrentTitle() const { return CurrentSummary.Title; }
    
    // 結果取得（Blueprint用）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    FString GetCurrentResult() const { return CurrentSummary.ResultText; }
    
    // 直接UI更新（Blueprintに依存しない）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateUIDirectly();
    
    // C++からTextBlockを直接更新（Blueprint不要）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void SetTextBlocksDirectly(UTextBlock* TitleTextBlock, UTextBlock* ResultTextBlock);
    
    // Widget初期化完了後に呼ばれる
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void OnWidgetInitialized();
    
    // デバッグ用：全てのWidget名を出力
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void DebugPrintAllWidgetNames();

protected:
    // Widget開始時の初期化
    virtual void NativeConstruct() override;
    
    // ボタンクリックイベント（内部用）
    UFUNCTION()
    void OnDetailsButtonClicked();
    
    // テキストを更新する関数（Blueprintから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateTexts(UTextBlock* TitleText, UTextBlock* ResultText, UTextBlock* DetailsText);
    
    // Rich Text Block版の更新関数
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateTextsRich(URichTextBlock* TitleText, URichTextBlock* ResultText, URichTextBlock* DetailsText);
    
    // プレーンテキスト版（色なし、記号を普通の文字に）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateTextsPlain(UTextBlock* TitleText, UTextBlock* ResultText, UTextBlock* DetailsText);
    
    // ダメージ情報を取得（色設定用）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    bool GetCurrentDamageInfo(int32& OutDamage, bool& bIsAllyDamaged) const;
    
    // 詳細パネルの表示/非表示（Blueprintから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void SetDetailsPanelVisibility(UVerticalBox* DetailsPanel, bool bVisible);
    
    // ボタンテキスト更新（Blueprintから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void UpdateButtonText(UButton* Button, bool bExpanded);
    
    // デバッグ用：現在のサマリーデータを出力
    UFUNCTION(BlueprintCallable, Category = "Event Log")
    void DebugPrintCurrentSummary();
    
    // C++からBlueprintのUI更新を呼び出し
    UFUNCTION(BlueprintImplementableEvent, Category = "Event Log")
    void UpdateUIFromCpp();
    
    // 勝敗判定関数
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log")
    bool IsVictory() const;

private:
    // 保持データ
    UPROPERTY()
    int32 EventIndex = -1;
    
    UPROPERTY()
    FEventSummary CurrentSummary;
    
    UPROPERTY()
    bool bIsExpanded = false;
    
    // UI更新が保留中かどうか
    bool bHasPendingUIUpdate = false;
    
    // EventLogManagerへの参照取得
    class UEventLogManager* GetEventLogManager() const;
    
    // 詳細テキストのフォーマット
    FString FormatDetailsText() const;
    
    // 詳細テキストのフォーマット（色付き）
    FString FormatDetailsTextWithColors() const;
    
    // 詳細テキストのフォーマット（プレーン、記号安全）
    FString FormatDetailsTextPlain() const;
};