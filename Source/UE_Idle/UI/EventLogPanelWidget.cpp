#include "EventLogPanelWidget.h"
#include "EventLogEntryWidget.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "../Components/EventLogManager.h"
#include "../C_PlayerController.h"
#include "Engine/World.h"

void UEventLogPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // EventLogManagerのイベントにバインド
    if (UEventLogManager* LogManager = GetEventLogManager())
    {
        // 新しいログ追加時
        LogManager->OnEventLogAdded.AddDynamic(this, &UEventLogPanelWidget::OnEventLogAdded);
        
        // サマリー作成時
        LogManager->OnEventSummaryCreated.AddDynamic(this, &UEventLogPanelWidget::OnEventSummaryCreated);
        
        // ログクリア時
        LogManager->OnEventLogCleared.AddDynamic(this, &UEventLogPanelWidget::OnEventLogCleared);
    }
}

void UEventLogPanelWidget::NativeDestruct()
{
    // イベントバインド解除
    if (UEventLogManager* LogManager = GetEventLogManager())
    {
        LogManager->OnEventLogAdded.RemoveDynamic(this, &UEventLogPanelWidget::OnEventLogAdded);
        LogManager->OnEventSummaryCreated.RemoveDynamic(this, &UEventLogPanelWidget::OnEventSummaryCreated);
        LogManager->OnEventLogCleared.RemoveDynamic(this, &UEventLogPanelWidget::OnEventLogCleared);
    }
    
    Super::NativeDestruct();
}

void UEventLogPanelWidget::SetLogPanel(UPanelWidget* Panel)
{
    LogPanel = Panel;
    
    // パネル設定後に初期ログを読み込み
    RefreshAllLogs();
}

void UEventLogPanelWidget::RefreshAllLogs()
{
    if (!LogPanel || !EventLogEntryWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshAllLogs: LogPanel(%s) or EventLogEntryWidgetClass(%s) is not set"), 
            LogPanel ? TEXT("SET") : TEXT("NULL"), 
            EventLogEntryWidgetClass ? TEXT("SET") : TEXT("NULL"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("RefreshAllLogs: Starting refresh..."));
    
    // 既存のWidgetをクリア（詳細ログ付き）
    ClearAllLogWidgets();
    
    // EventLogManagerからサマリー取得
    if (UEventLogManager* LogManager = GetEventLogManager())
    {
        TArray<FEventSummary> AllSummaries = LogManager->GetAllEventSummaries();
        UE_LOG(LogTemp, Log, TEXT("RefreshAllLogs: Found %d summaries"), AllSummaries.Num());
        
        // 各サマリーに対してWidgetを作成
        for (int32 i = 0; i < AllSummaries.Num(); i++)
        {
            CreateLogEntryWidget(i);
        }
        
        UE_LOG(LogTemp, Log, TEXT("RefreshAllLogs: Successfully refreshed %d log entries"), AllSummaries.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RefreshAllLogs: No EventLogManager found"));
    }
}

void UEventLogPanelWidget::SetViewingMode(bool bIsViewing)
{
    bIsViewingMode = bIsViewing;
    
    UE_LOG(LogTemp, Log, TEXT("Viewing mode set to: %s"), bIsViewing ? TEXT("true") : TEXT("false"));
    
    // 閲覧終了時に保留中の更新を処理
    if (!bIsViewing && bHasPendingUpdates)
    {
        ProcessPendingUpdates();
    }
}

void UEventLogPanelWidget::ProcessPendingUpdates()
{
    if (bHasPendingUpdates)
    {
        UE_LOG(LogTemp, Log, TEXT("Processing pending log updates"));
        RefreshAllLogs();
        bHasPendingUpdates = false;
    }
}

void UEventLogPanelWidget::OnEventLogAdded(const FEventLogEntry& LogEntry)
{
    // 閲覧中は更新を保留
    if (bIsViewingMode)
    {
        bHasPendingUpdates = true;
        UE_LOG(LogTemp, Log, TEXT("Log update pending (viewing mode)"));
        return;
    }
    
    // 通常時は何もしない（サマリー作成時にまとめて処理）
}

void UEventLogPanelWidget::OnEventSummaryCreated(const FEventSummary& Summary)
{
    // 閲覧中は更新を保留
    if (bIsViewingMode)
    {
        bHasPendingUpdates = true;
        UE_LOG(LogTemp, Log, TEXT("Summary update pending (viewing mode)"));
        return;
    }
    
    // 新しいサマリーに対応するWidgetを追加
    if (LogPanel && EventLogEntryWidgetClass)
    {
        UEventLogManager* LogManager = GetEventLogManager();
        if (LogManager)
        {
            TArray<FEventSummary> AllSummaries = LogManager->GetAllEventSummaries();
            int32 NewIndex = AllSummaries.Num() - 1; // 最新のサマリーのインデックス
            
            if (NewIndex >= 0)
            {
                CreateLogEntryWidget(NewIndex);
                UE_LOG(LogTemp, Log, TEXT("Added new log entry widget for summary: %s"), *Summary.Title);
            }
        }
    }
}

void UEventLogPanelWidget::OnEventLogCleared()
{
    // 閲覧中でもクリアは即座に実行
    ClearAllLogWidgets();
    bHasPendingUpdates = false;
    
    UE_LOG(LogTemp, Log, TEXT("All log entries cleared"));
}

void UEventLogPanelWidget::CreateLogEntryWidget(int32 Index)
{
    if (!LogPanel || !EventLogEntryWidgetClass)
    {
        return;
    }
    
    // 新しいWidget作成
    UEventLogEntryWidget* NewLogWidget = CreateWidget<UEventLogEntryWidget>(this, EventLogEntryWidgetClass);
    if (NewLogWidget)
    {
        // パネルに追加
        LogPanel->AddChild(NewLogWidget);
        
        // 管理用配列に追加
        LogEntryWidgets.Add(NewLogWidget);
        
        // インデックス設定（AddChild後に実行）
        NewLogWidget->SetEventIndex(Index);
        
        // Widget作成直後にテキストデータを強制反映
        FString CurrentTitle = NewLogWidget->GetCurrentTitle();
        FString CurrentResult = NewLogWidget->GetCurrentResult();
        
        UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Triggering text update"));
        UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Widget Name = %s"), *NewLogWidget->GetName());
        UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Title='%s', Result='%s'"), *CurrentTitle, *CurrentResult);
        UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Title Length=%d, Result Length=%d"), CurrentTitle.Len(), CurrentResult.Len());
        
        // 複数の方法でUI更新をトリガー
        UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Calling UpdateUIDirectly"));
        
        // 呼び出し前の状態を確認
        if (!NewLogWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("CreateLogEntryWidget: NewLogWidget is NULL!"));
            return;
        }
        
        if (!IsValid(NewLogWidget))
        {
            UE_LOG(LogTemp, Error, TEXT("CreateLogEntryWidget: NewLogWidget is not valid!"));
            return;
        }
        
        UE_LOG(LogTemp, Error, TEXT("CreateLogEntryWidget: About to call UpdateUIDirectly on %s"), *NewLogWidget->GetName());
        NewLogWidget->UpdateUIDirectly();
        UE_LOG(LogTemp, Error, TEXT("CreateLogEntryWidget: UpdateUIDirectly call completed"));
        
        // 次フレームでも再実行（確実性のため）
        if (GetWorld())
        {
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, [NewLogWidget, Index]()
            {
                if (IsValid(NewLogWidget))
                {
                    FString DelayedTitle = NewLogWidget->GetCurrentTitle();
                    FString DelayedResult = NewLogWidget->GetCurrentResult();
                    UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Delayed update - Title='%s', Result='%s'"), *DelayedTitle, *DelayedResult);
                    NewLogWidget->UpdateUIDirectly();
                    UE_LOG(LogTemp, Warning, TEXT("CreateLogEntryWidget: Delayed UI update triggered for index %d"), Index);
                }
            }, 0.1f, false);
        }
        
        UE_LOG(LogTemp, Log, TEXT("EventLogPanelWidget: Text update triggered for new widget"));
        
        UE_LOG(LogTemp, Log, TEXT("Created log entry widget for index: %d"), Index);
    }
}

void UEventLogPanelWidget::ClearAllLogWidgets()
{
    if (LogPanel)
    {
        LogPanel->ClearChildren();
    }
    
    LogEntryWidgets.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("Cleared all log entry widgets"));
}

UEventLogManager* UEventLogPanelWidget::GetEventLogManager() const
{
    if (UWorld* World = GetWorld())
    {
        if (AC_PlayerController* PC = Cast<AC_PlayerController>(World->GetFirstPlayerController()))
        {
            return PC->EventLogManager;
        }
    }
    return nullptr;
}
