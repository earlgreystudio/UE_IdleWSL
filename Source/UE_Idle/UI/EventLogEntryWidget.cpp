#include "EventLogEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "../Components/EventLogManager.h"
#include "../C_PlayerController.h"
#include "Engine/World.h"

void UEventLogEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UEventLogEntryWidget::SetEventIndex(int32 Index)
{
    EventIndex = Index;
    
    // EventLogManagerからデータ取得
    if (UEventLogManager* LogManager = GetEventLogManager())
    {
        TArray<FEventSummary> AllSummaries = LogManager->GetAllEventSummaries();
        if (Index >= 0 && Index < AllSummaries.Num())
        {
            CurrentSummary = AllSummaries[Index];
        }
    }
}

bool UEventLogEntryWidget::ToggleDetails()
{
    bIsExpanded = !bIsExpanded;
    return bIsExpanded;
}

void UEventLogEntryWidget::OnDetailsButtonClicked()
{
    ToggleDetails();
}

void UEventLogEntryWidget::UpdateTexts(UTextBlock* TitleText, UTextBlock* ResultText, UTextBlock* DetailsText)
{
    if (!TitleText || !ResultText || !DetailsText)
    {
        return;
    }
    
    // タイトルテキスト設定
    TitleText->SetText(FText::FromString(CurrentSummary.Title));
    
    // 結果テキスト設定
    ResultText->SetText(FText::FromString(CurrentSummary.ResultText));
    
    // 詳細テキスト設定（展開時のみ）
    if (bIsExpanded)
    {
        FString FormattedDetails = FormatDetailsText();
        DetailsText->SetText(FText::FromString(FormattedDetails));
    }
    else
    {
        DetailsText->SetText(FText::GetEmpty());
    }
}

void UEventLogEntryWidget::SetDetailsPanelVisibility(UVerticalBox* DetailsPanel, bool bVisible)
{
    if (!DetailsPanel)
    {
        return;
    }
    
    // Visibility設定
    if (bVisible)
    {
        DetailsPanel->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        DetailsPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UEventLogEntryWidget::UpdateButtonText(UButton* Button, bool bExpanded)
{
    if (!Button)
    {
        return;
    }
    
    // ボタンの最初の子要素（TextBlock）を取得
    if (Button->GetChildrenCount() > 0)
    {
        if (UTextBlock* ButtonText = Cast<UTextBlock>(Button->GetChildAt(0)))
        {
            FString Text = bExpanded ? TEXT("【閉じる】") : TEXT("【詳細】");
            ButtonText->SetText(FText::FromString(Text));
        }
    }
}

UEventLogManager* UEventLogEntryWidget::GetEventLogManager() const
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

FString UEventLogEntryWidget::FormatDetailsText() const
{
    FString DetailsText;
    
    // 詳細ログをフォーマット
    for (const FEventLogEntry& LogEntry : CurrentSummary.DetailedLogs)
    {
        // 戦闘ログの場合の特殊フォーマット
        if (LogEntry.EventCategory == EEventCategory::Combat)
        {
            const FCombatEventData& CombatData = LogEntry.CombatData;
            
            // HP表示行（フォント問題回避）
            DetailsText += FString::Printf(TEXT("%s(%d/%d) <===[⚔️]===> %s(%d/%d)\n"),
                *CombatData.AttackerName, CombatData.AttackerHP, CombatData.AttackerMaxHP,
                *CombatData.DefenderName, CombatData.DefenderHP, CombatData.DefenderMaxHP);
            
            // 攻撃結果行
            if (CombatData.bIsPlayerAttacker)
            {
                DetailsText += FString::Printf(TEXT("          %s攻撃 → （%d）"),
                    *CombatData.WeaponName, CombatData.Damage);
            }
            else
            {
                DetailsText += FString::Printf(TEXT("          （%d） ← %s攻撃"),
                    CombatData.Damage, *CombatData.WeaponName);
            }
            
            if (CombatData.bIsCritical)
            {
                DetailsText += TEXT(" クリティカル！");
            }
            
            DetailsText += TEXT("\n\n");
        }
        else
        {
            // その他のイベントタイプ
            DetailsText += LogEntry.FormattedText + TEXT("\n");
        }
    }
    
    return DetailsText;
}