#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/EventLogTypes.h"
#include "EventLogPanelWidget.generated.h"

class UPanelWidget;
class UEventLogEntryWidget;
class UEventLogManager;

UCLASS()
class UE_IDLE_API UEventLogPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // パネルを設定してログを初期化
    UFUNCTION(BlueprintCallable, Category = "Event Log Panel")
    void SetLogPanel(UPanelWidget* Panel);
    
    // ログを完全更新（初回表示時）
    UFUNCTION(BlueprintCallable, Category = "Event Log Panel")
    void RefreshAllLogs();
    
    // 閲覧モードの切り替え（trueの間は自動更新停止）
    UFUNCTION(BlueprintCallable, Category = "Event Log Panel")
    void SetViewingMode(bool bIsViewing);
    
    // 閲覧モード状態を取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event Log Panel")
    bool IsViewingMode() const { return bIsViewingMode; }
    
    // 手動でログ追加処理を実行（閲覧終了時など）
    UFUNCTION(BlueprintCallable, Category = "Event Log Panel")
    void ProcessPendingUpdates();

protected:
    // Widget開始時の初期化
    virtual void NativeConstruct() override;
    
    // Widget終了時のクリーンアップ
    virtual void NativeDestruct() override;

private:
    // EventLogManagerのイベント処理
    UFUNCTION()
    void OnEventLogAdded(const FEventLogEntry& LogEntry);
    
    UFUNCTION()
    void OnEventSummaryCreated(const FEventSummary& Summary);
    
    UFUNCTION()
    void OnEventLogCleared();
    
    // 内部処理関数
    void CreateLogEntryWidget(int32 Index);
    void ClearAllLogWidgets();
    UEventLogManager* GetEventLogManager() const;
    
    // 保持データ
    UPROPERTY()
    UPanelWidget* LogPanel = nullptr;
    
    UPROPERTY()
    TArray<UEventLogEntryWidget*> LogEntryWidgets;
    
    UPROPERTY()
    bool bIsViewingMode = false;
    
    UPROPERTY()
    bool bHasPendingUpdates = false;
    
    // Widget作成用クラス（Blueprintで設定）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UEventLogEntryWidget> EventLogEntryWidgetClass;
};