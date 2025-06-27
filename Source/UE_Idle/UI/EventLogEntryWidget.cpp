#include "EventLogEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "../Components/EventLogManager.h"
#include "../C_PlayerController.h"
#include "Engine/World.h"

void UEventLogEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Warning, TEXT("EventLogEntryWidget::NativeConstruct called"));
    
    // 保留中の更新があるかチェック
    if (bHasPendingUIUpdate)
    {
        UE_LOG(LogTemp, Warning, TEXT("NativeConstruct: Processing pending UI update"));
        bHasPendingUIUpdate = false;
        UpdateUIDirectly();
    }
    // Widget構築完了後、データが既にある場合は更新
    else if (EventIndex >= 0 && !CurrentSummary.Title.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("NativeConstruct: Data already exists, updating UI"));
        UpdateUIDirectly();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NativeConstruct: No data yet (EventIndex=%d, Title='%s')"), 
            EventIndex, *CurrentSummary.Title);
    }
}

void UEventLogEntryWidget::SetEventIndex(int32 Index)
{
    EventIndex = Index;
    UE_LOG(LogTemp, Warning, TEXT("=== SetEventIndex Called ==="));
    UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Setting index %d"), Index);
    
    // EventLogManagerからデータ取得
    if (UEventLogManager* LogManager = GetEventLogManager())
    {
        TArray<FEventSummary> AllSummaries = LogManager->GetAllEventSummaries();
        UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Found %d summaries"), AllSummaries.Num());
        
        if (Index >= 0 && Index < AllSummaries.Num())
        {
            CurrentSummary = AllSummaries[Index];
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: SUCCESS - Summary title: '%s'"), *CurrentSummary.Title);
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: SUCCESS - Summary result: '%s'"), *CurrentSummary.ResultText);
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: SUCCESS - Detailed logs count: %d"), CurrentSummary.DetailedLogs.Num());
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: SUCCESS - CurrentSummary.Title.IsEmpty() = %s"), CurrentSummary.Title.IsEmpty() ? TEXT("true") : TEXT("false"));
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: SUCCESS - CurrentSummary.ResultText.IsEmpty() = %s"), CurrentSummary.ResultText.IsEmpty() ? TEXT("true") : TEXT("false"));
            
            for (int32 i = 0; i < FMath::Min(3, CurrentSummary.DetailedLogs.Num()); i++)
            {
                UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Detail %d: '%s'"), i, *CurrentSummary.DetailedLogs[i].FormattedText);
            }
            
            // データ設定後、デバッグ情報を出力
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Before any UI update attempts"));
            DebugPrintAllWidgetNames();
            
            // BlueprintのUpdateUIを呼び出し
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Calling UpdateUIFromCpp"));
            UpdateUIFromCpp();
            UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: UpdateUIFromCpp completed"));
            
            // Widget が初期化済みかチェック
            if (GetCachedWidget())
            {
                UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Widget is initialized, attempting direct update"));
                UpdateUIDirectly();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SetEventIndex: Widget not yet initialized, marking for pending update"));
                bHasPendingUIUpdate = true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SetEventIndex: Index %d out of range (0-%d)"), Index, AllSummaries.Num()-1);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SetEventIndex: No EventLogManager found"));
    }
    UE_LOG(LogTemp, Warning, TEXT("=== SetEventIndex End ==="));
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
    UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Called with TitleText=%s, ResultText=%s, DetailsText=%s"), 
        TitleText ? TEXT("Valid") : TEXT("NULL"),
        ResultText ? TEXT("Valid") : TEXT("NULL"),
        DetailsText ? TEXT("Valid") : TEXT("NULL"));
    
    // Widgetが渡されない場合は、名前で探す
    if (!TitleText)
    {
        TitleText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Title")));
        if (TitleText)
        {
            UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Found Text_Title by name"));
        }
    }
    
    if (!ResultText)
    {
        ResultText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Result")));
        if (ResultText)
        {
            UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Found Text_Result by name"));
        }
    }
    
    if (!DetailsText)
    {
        DetailsText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Details")));
        if (DetailsText)
        {
            UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Found Text_Details by name"));
        }
    }
    
    if (!TitleText || !ResultText)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateTexts: Critical widgets are NULL"));
        return;
    }
    
    // タイトルテキスト設定
    TitleText->SetText(FText::FromString(CurrentSummary.Title));
    UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Set title to: %s"), *CurrentSummary.Title);
    
    // 結果テキスト設定
    ResultText->SetText(FText::FromString(CurrentSummary.ResultText));
    UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Set result to: %s"), *CurrentSummary.ResultText);
    
    // 詳細テキスト設定（展開時のみ）
    if (DetailsText)
    {
        UE_LOG(LogTemp, Log, TEXT("UpdateTexts: bIsExpanded = %s"), bIsExpanded ? TEXT("true") : TEXT("false"));
        if (bIsExpanded)
        {
            FString FormattedDetails = FormatDetailsText();
            UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Formatted details: %s"), *FormattedDetails);
            DetailsText->SetText(FText::FromString(FormattedDetails));
        }
        else
        {
            DetailsText->SetText(FText::GetEmpty());
            UE_LOG(LogTemp, Log, TEXT("UpdateTexts: Details collapsed, set to empty"));
        }
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

void UEventLogEntryWidget::DebugPrintCurrentSummary()
{
    UE_LOG(LogTemp, Warning, TEXT("=== DebugPrintCurrentSummary ==="));
    UE_LOG(LogTemp, Warning, TEXT("EventIndex: %d"), EventIndex);
    UE_LOG(LogTemp, Warning, TEXT("CurrentSummary.Title: '%s' (Length: %d)"), *CurrentSummary.Title, CurrentSummary.Title.Len());
    UE_LOG(LogTemp, Warning, TEXT("CurrentSummary.ResultText: '%s' (Length: %d)"), *CurrentSummary.ResultText, CurrentSummary.ResultText.Len());
    UE_LOG(LogTemp, Warning, TEXT("CurrentSummary.DetailedLogs.Num(): %d"), CurrentSummary.DetailedLogs.Num());
    UE_LOG(LogTemp, Warning, TEXT("bIsExpanded: %s"), bIsExpanded ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("=== End DebugPrintCurrentSummary ==="));
}

bool UEventLogEntryWidget::GetCurrentDamageInfo(int32& OutDamage, bool& bIsAllyDamaged) const
{
    if (CurrentSummary.DetailedLogs.Num() == 0)
    {
        return false;
    }
    
    // 最新の戦闘ログからダメージ情報を取得
    for (int32 i = CurrentSummary.DetailedLogs.Num() - 1; i >= 0; i--)
    {
        const FEventLogEntry& LogEntry = CurrentSummary.DetailedLogs[i];
        if (LogEntry.EventCategory == EEventCategory::Combat && LogEntry.CombatData.Damage > 0)
        {
            OutDamage = LogEntry.CombatData.Damage;
            bIsAllyDamaged = !LogEntry.CombatData.bIsPlayerAttacker; // プレイヤーが攻撃者でなければ味方が被害
            return true;
        }
    }
    
    return false;
}

void UEventLogEntryWidget::UpdateTextsRich(URichTextBlock* TitleText, URichTextBlock* ResultText, URichTextBlock* DetailsText)
{
    UE_LOG(LogTemp, Log, TEXT("UpdateTextsRich: Called with TitleText=%s, ResultText=%s, DetailsText=%s"), 
        TitleText ? TEXT("Valid") : TEXT("NULL"),
        ResultText ? TEXT("Valid") : TEXT("NULL"),
        DetailsText ? TEXT("Valid") : TEXT("NULL"));
    
    if (!TitleText || !ResultText || !DetailsText)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateTextsRich: One or more rich text widgets are NULL"));
        return;
    }
    
    // 勝敗判定（結果テキストに「勝利」が含まれているかで判断）
    bool bIsVictory = CurrentSummary.ResultText.Contains(TEXT("勝利"));
    
    // タイトルテキスト設定（勝ったら青、負けたら赤）
    FString ColoredTitle = bIsVictory ? 
        FString::Printf(TEXT("<Blue>%s</>"), *CurrentSummary.Title) :
        FString::Printf(TEXT("<Red>%s</>"), *CurrentSummary.Title);
    TitleText->SetText(FText::FromString(ColoredTitle));
    UE_LOG(LogTemp, Log, TEXT("UpdateTextsRich: Set title to: %s"), *ColoredTitle);
    
    // 結果テキスト設定（勝ったら青、負けたら赤）
    FString ColoredResult = bIsVictory ?
        FString::Printf(TEXT("<Blue>%s</>"), *CurrentSummary.ResultText) :
        FString::Printf(TEXT("<Red>%s</>"), *CurrentSummary.ResultText);
    ResultText->SetText(FText::FromString(ColoredResult));
    UE_LOG(LogTemp, Log, TEXT("UpdateTextsRich: Set result to: %s"), *ColoredResult);
    
    // 詳細テキスト設定（展開時のみ）
    UE_LOG(LogTemp, Log, TEXT("UpdateTextsRich: bIsExpanded = %s"), bIsExpanded ? TEXT("true") : TEXT("false"));
    if (bIsExpanded)
    {
        FString FormattedDetails = FormatDetailsTextWithColors();
        UE_LOG(LogTemp, Log, TEXT("UpdateTextsRich: Formatted details: %s"), *FormattedDetails);
        DetailsText->SetText(FText::FromString(FormattedDetails));
    }
    else
    {
        DetailsText->SetText(FText::GetEmpty());
        UE_LOG(LogTemp, Log, TEXT("UpdateTextsRich: Details collapsed, set to empty"));
    }
}

FString UEventLogEntryWidget::FormatDetailsText() const
{
    UE_LOG(LogTemp, Log, TEXT("FormatDetailsText: Starting with %d detailed logs"), CurrentSummary.DetailedLogs.Num());
    
    FString DetailsText;
    
    // 詳細ログをフォーマット
    for (int32 i = 0; i < CurrentSummary.DetailedLogs.Num(); i++)
    {
        const FEventLogEntry& LogEntry = CurrentSummary.DetailedLogs[i];
        UE_LOG(LogTemp, Log, TEXT("FormatDetailsText: Processing log %d, category: %s"), 
            i, *UEnum::GetValueAsString(LogEntry.EventCategory));
        
        // 戦闘ログの場合の特殊フォーマット
        if (LogEntry.EventCategory == EEventCategory::Combat)
        {
            const FCombatEventData& CombatData = LogEntry.CombatData;
            UE_LOG(LogTemp, Log, TEXT("FormatDetailsText: Combat log - Attacker: %s, Defender: %s, Damage: %d"), 
                *CombatData.AttackerName, *CombatData.DefenderName, CombatData.Damage);
            
            // 味方（プレイヤー）と敵の名前・HPを固定配置で決定
            FString AllyName, EnemyName;
            int32 AllyHP, AllyMaxHP, EnemyHP, EnemyMaxHP;
            
            if (CombatData.bIsPlayerAttacker)
            {
                // プレイヤー（味方）が攻撃者の場合
                AllyName = CombatData.AttackerName;
                AllyHP = CombatData.AttackerHP;
                AllyMaxHP = CombatData.AttackerMaxHP;
                EnemyName = CombatData.DefenderName;
                EnemyHP = CombatData.DefenderHP;
                EnemyMaxHP = CombatData.DefenderMaxHP;
            }
            else
            {
                // 敵が攻撃者の場合
                AllyName = CombatData.DefenderName;  // 味方は防御者
                AllyHP = CombatData.DefenderHP;
                AllyMaxHP = CombatData.DefenderMaxHP;
                EnemyName = CombatData.AttackerName;  // 敵は攻撃者
                EnemyHP = CombatData.AttackerHP;
                EnemyMaxHP = CombatData.AttackerMaxHP;
            }
            
            // HP表示行（味方 左、敵 右 - 固定）
            DetailsText += FString::Printf(TEXT("%s(%d/%d) <===[VS]===> %s(%d/%d)\n"),
                *AllyName, AllyHP, AllyMaxHP, *EnemyName, EnemyHP, EnemyMaxHP);
            
            // 攻撃方向表示（攻撃者によって方向が変わる）
            if (CombatData.bIsPlayerAttacker)
            {
                // 味方攻撃：左から右へ
                FString CriticalText = CombatData.bIsCritical ? TEXT(" クリティカル！") : TEXT("");
                DetailsText += FString::Printf(TEXT("          %s攻撃%s → （%d）"),
                    *CombatData.WeaponName, *CriticalText, CombatData.Damage);
            }
            else
            {
                // 敵攻撃：右から左へ
                FString CriticalText = CombatData.bIsCritical ? TEXT(" クリティカル！") : TEXT("");
                DetailsText += FString::Printf(TEXT("          （%d） ← %s攻撃%s"),
                    CombatData.Damage, *CombatData.WeaponName, *CriticalText);
            }
            
            DetailsText += TEXT("\n\n");
        }
        else
        {
            // その他のイベントタイプ
            UE_LOG(LogTemp, Log, TEXT("FormatDetailsText: Non-combat log - FormattedText: %s"), *LogEntry.FormattedText);
            DetailsText += LogEntry.FormattedText + TEXT("\n");
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("FormatDetailsText: Final result length: %d"), DetailsText.Len());
    return DetailsText;
}

FString UEventLogEntryWidget::FormatDetailsTextWithColors() const
{
    UE_LOG(LogTemp, Log, TEXT("FormatDetailsTextWithColors: Starting with %d detailed logs"), CurrentSummary.DetailedLogs.Num());
    
    FString DetailsText;
    
    // 詳細ログをフォーマット
    for (int32 i = 0; i < CurrentSummary.DetailedLogs.Num(); i++)
    {
        const FEventLogEntry& LogEntry = CurrentSummary.DetailedLogs[i];
        UE_LOG(LogTemp, Log, TEXT("FormatDetailsTextWithColors: Processing log %d, category: %s"), 
            i, *UEnum::GetValueAsString(LogEntry.EventCategory));
        
        // 戦闘ログの場合の特殊フォーマット
        if (LogEntry.EventCategory == EEventCategory::Combat)
        {
            const FCombatEventData& CombatData = LogEntry.CombatData;
            UE_LOG(LogTemp, Log, TEXT("FormatDetailsTextWithColors: Combat log - Attacker: %s, Defender: %s, Damage: %d"), 
                *CombatData.AttackerName, *CombatData.DefenderName, CombatData.Damage);
            
            // 味方と敵の判定（味方を常に左、敵を常に右に配置）
            FString AllyName, EnemyName;
            int32 AllyHP, AllyMaxHP, EnemyHP, EnemyMaxHP;
            bool bAllyAttacked = false; // 味方が攻撃を受けたか
            
            if (CombatData.bIsPlayerAttacker)
            {
                // プレイヤー（味方）が攻撃者
                AllyName = CombatData.AttackerName;
                AllyHP = CombatData.AttackerHP;
                AllyMaxHP = CombatData.AttackerMaxHP;
                EnemyName = CombatData.DefenderName;
                EnemyHP = CombatData.DefenderHP;
                EnemyMaxHP = CombatData.DefenderMaxHP;
                bAllyAttacked = false; // 敵が攻撃を受けた
            }
            else
            {
                // 敵が攻撃者
                AllyName = CombatData.DefenderName;
                AllyHP = CombatData.DefenderHP;
                AllyMaxHP = CombatData.DefenderMaxHP;
                EnemyName = CombatData.AttackerName;
                EnemyHP = CombatData.AttackerHP;
                EnemyMaxHP = CombatData.AttackerMaxHP;
                bAllyAttacked = true; // 味方が攻撃を受けた
            }
            
            // HP表示行（味方 左、敵 右）
            DetailsText += FString::Printf(TEXT("%s(%d/%d) <===[VS]===> %s(%d/%d)\n"),
                *AllyName, AllyHP, AllyMaxHP, *EnemyName, EnemyHP, EnemyMaxHP);
            
            // ダメージ表示行（被害者の位置にダメージ表示、色付き）
            if (bAllyAttacked)
            {
                // 味方が攻撃を受けた場合：左端にダメージ（赤色）
                DetailsText += FString::Printf(TEXT("<Red>(%d)</> &lt;- %s攻撃"),
                    CombatData.Damage, *CombatData.WeaponName);
            }
            else
            {
                // 敵が攻撃を受けた場合：右端にダメージ（青色）
                DetailsText += FString::Printf(TEXT("          %s攻撃 -&gt; <Blue>(%d)</>"),
                    *CombatData.WeaponName, CombatData.Damage);
            }
            
            if (CombatData.bIsCritical)
            {
                DetailsText += TEXT(" <Gold>クリティカル！</>");
            }
            
            DetailsText += TEXT("\n\n");
        }
        else
        {
            // その他のイベントタイプ
            UE_LOG(LogTemp, Log, TEXT("FormatDetailsTextWithColors: Non-combat log - FormattedText: %s"), *LogEntry.FormattedText);
            DetailsText += LogEntry.FormattedText + TEXT("\n");
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("FormatDetailsTextWithColors: Final result length: %d"), DetailsText.Len());
    return DetailsText;
}

bool UEventLogEntryWidget::IsVictory() const
{
    return CurrentSummary.ResultText.Contains(TEXT("勝利"));
}

void UEventLogEntryWidget::ForceUpdateUI()
{
    UE_LOG(LogTemp, Log, TEXT("ForceUpdateUI: EventIndex=%d, HasData=%s"), 
        EventIndex, CurrentSummary.Title.IsEmpty() ? TEXT("false") : TEXT("true"));
    
    // 複数回UpdateUIFromCppを呼び出して確実にUI更新
    UpdateUIFromCpp();
    
    // 次フレームでも再度実行
    if (GetWorld())
    {
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (IsValid(this))
            {
                UpdateUIFromCpp();
                UE_LOG(LogTemp, Log, TEXT("ForceUpdateUI: Delayed update executed"));
            }
        }, 0.05f, false);
    }
}

void UEventLogEntryWidget::UpdateUIDirectly()
{
    // 絶対に最初にログを出す（関数が呼ばれたことを確認）
    UE_LOG(LogTemp, Error, TEXT("!!! UpdateUIDirectly ENTRY POINT !!!"));
    
    UE_LOG(LogTemp, Warning, TEXT("=== UpdateUIDirectly Called ==="));
    UE_LOG(LogTemp, Warning, TEXT("EventIndex: %d"), EventIndex);
    UE_LOG(LogTemp, Warning, TEXT("CurrentSummary.Title: '%s' (Length: %d)"), *CurrentSummary.Title, CurrentSummary.Title.Len());
    UE_LOG(LogTemp, Warning, TEXT("CurrentSummary.ResultText: '%s' (Length: %d)"), *CurrentSummary.ResultText, CurrentSummary.ResultText.Len());
    UE_LOG(LogTemp, Warning, TEXT("CurrentSummary.Title.IsEmpty(): %s"), CurrentSummary.Title.IsEmpty() ? TEXT("TRUE") : TEXT("FALSE"));
    
    // Widget内のTextBlockを名前で検索して直接更新
    UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Searching for Text_Title widget..."));
    if (UTextBlock* TitleTextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Title"))))
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Found Text_Title widget, current text='%s'"), *TitleTextBlock->GetText().ToString());
        TitleTextBlock->SetText(FText::FromString(CurrentSummary.Title));
        UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Updated Text_Title to '%s'"), *CurrentSummary.Title);
        
        // Verify the change
        FString NewText = TitleTextBlock->GetText().ToString();
        UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: TitleText verification - new text='%s'"), *NewText);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateUIDirectly: Text_Title widget NOT FOUND"));
        
        // Try to list all child widgets by name
        TArray<FName> AllWidgetNames;
        AllWidgetNames.Add(TEXT("TitleText"));
        AllWidgetNames.Add(TEXT("ResultText"));
        AllWidgetNames.Add(TEXT("DetailsText"));
        AllWidgetNames.Add(TEXT("Title"));
        AllWidgetNames.Add(TEXT("Result"));
        AllWidgetNames.Add(TEXT("TextBlock_Title"));
        AllWidgetNames.Add(TEXT("TextBlock_Result"));
        
        UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Trying common widget names..."));
        for (const FName& WidgetName : AllWidgetNames)
        {
            if (UWidget* Widget = GetWidgetFromName(WidgetName))
            {
                UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Found widget with name '%s' (Type: %s)"), 
                    *WidgetName.ToString(), *Widget->GetClass()->GetName());
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Searching for Text_Result widget..."));
    if (UTextBlock* ResultTextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Result"))))
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Found Text_Result widget, current text='%s'"), *ResultTextBlock->GetText().ToString());
        ResultTextBlock->SetText(FText::FromString(CurrentSummary.ResultText));
        UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Updated Text_Result to '%s'"), *CurrentSummary.ResultText);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateUIDirectly: Text_Result widget NOT FOUND"));
    }
    
    // Blueprintの実装があれば呼び出し
    UE_LOG(LogTemp, Warning, TEXT("UpdateUIDirectly: Calling UpdateUIFromCpp"));
    UpdateUIFromCpp();
    UE_LOG(LogTemp, Warning, TEXT("=== UpdateUIDirectly End ==="));
}

void UEventLogEntryWidget::SetTextBlocksDirectly(UTextBlock* TitleTextBlock, UTextBlock* ResultTextBlock)
{
    if (TitleTextBlock && !CurrentSummary.Title.IsEmpty())
    {
        TitleTextBlock->SetText(FText::FromString(CurrentSummary.Title));
        UE_LOG(LogTemp, Warning, TEXT("SetTextBlocksDirectly: Set title to '%s'"), *CurrentSummary.Title);
    }
    
    if (ResultTextBlock && !CurrentSummary.ResultText.IsEmpty())
    {
        ResultTextBlock->SetText(FText::FromString(CurrentSummary.ResultText));
        UE_LOG(LogTemp, Warning, TEXT("SetTextBlocksDirectly: Set result to '%s'"), *CurrentSummary.ResultText);
    }
}

void UEventLogEntryWidget::UpdateTextsPlain(UTextBlock* TitleText, UTextBlock* ResultText, UTextBlock* DetailsText)
{
    UE_LOG(LogTemp, Log, TEXT("UpdateTextsPlain: Called - fallback for Rich Text issues"));
    
    if (!TitleText || !ResultText || !DetailsText)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateTextsPlain: One or more text widgets are NULL"));
        return;
    }
    
    // プレーンテキスト設定（色なし）
    TitleText->SetText(FText::FromString(CurrentSummary.Title));
    ResultText->SetText(FText::FromString(CurrentSummary.ResultText));
    
    // 詳細テキスト設定（記号を普通の文字に変更）
    if (bIsExpanded)
    {
        FString PlainDetails = FormatDetailsTextPlain();
        DetailsText->SetText(FText::FromString(PlainDetails));
    }
    else
    {
        DetailsText->SetText(FText::GetEmpty());
    }
}

void UEventLogEntryWidget::OnWidgetInitialized()
{
    UE_LOG(LogTemp, Warning, TEXT("OnWidgetInitialized called"));
    
    // Widget初期化完了後にUI更新を試みる
    if (EventIndex >= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnWidgetInitialized: Valid EventIndex %d, updating UI"), EventIndex);
        UpdateUIDirectly();
    }
}

void UEventLogEntryWidget::DebugPrintAllWidgetNames()
{
    UE_LOG(LogTemp, Warning, TEXT("=== DebugPrintAllWidgetNames ==="));
    
    // 一般的なWidget名を試す
    TArray<FString> CommonNames = {
        TEXT("TitleText"), TEXT("ResultText"), TEXT("DetailsText"),
        TEXT("Title"), TEXT("Result"), TEXT("Details"),
        TEXT("TextBlock_Title"), TEXT("TextBlock_Result"), TEXT("TextBlock_Details"),
        TEXT("TB_Title"), TEXT("TB_Result"), TEXT("TB_Details"),
        TEXT("Text_Title"), TEXT("Text_Result"), TEXT("Text_Details")
    };
    
    for (const FString& Name : CommonNames)
    {
        if (UWidget* Widget = GetWidgetFromName(*Name))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found widget: %s (Type: %s)"), 
                *Name, *Widget->GetClass()->GetName());
            
            // TextBlockの場合は現在のテキストも表示
            if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
            {
                UE_LOG(LogTemp, Warning, TEXT("  Current text: '%s'"), 
                    *TextBlock->GetText().ToString());
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== End DebugPrintAllWidgetNames ==="));
}

FString UEventLogEntryWidget::FormatDetailsTextPlain() const
{
    FString DetailsText;
    
    // 詳細ログをフォーマット（記号を安全な文字に変更）
    for (int32 i = 0; i < CurrentSummary.DetailedLogs.Num(); i++)
    {
        const FEventLogEntry& LogEntry = CurrentSummary.DetailedLogs[i];
        
        if (LogEntry.EventCategory == EEventCategory::Combat)
        {
            const FCombatEventData& CombatData = LogEntry.CombatData;
            
            // 味方と敵の判定（味方を常に左、敵を常に右に配置）
            FString AllyName, EnemyName;
            int32 AllyHP, AllyMaxHP, EnemyHP, EnemyMaxHP;
            bool bAllyAttacked = false;
            
            if (CombatData.bIsPlayerAttacker)
            {
                AllyName = CombatData.AttackerName;
                AllyHP = CombatData.AttackerHP;
                AllyMaxHP = CombatData.AttackerMaxHP;
                EnemyName = CombatData.DefenderName;
                EnemyHP = CombatData.DefenderHP;
                EnemyMaxHP = CombatData.DefenderMaxHP;
                bAllyAttacked = false;
            }
            else
            {
                AllyName = CombatData.DefenderName;
                AllyHP = CombatData.DefenderHP;
                AllyMaxHP = CombatData.DefenderMaxHP;
                EnemyName = CombatData.AttackerName;
                EnemyHP = CombatData.AttackerHP;
                EnemyMaxHP = CombatData.AttackerMaxHP;
                bAllyAttacked = true;
            }
            
            // HP表示行（安全な文字のみ）
            DetailsText += FString::Printf(TEXT("%s(%d/%d) [VS] %s(%d/%d)\n"),
                *AllyName, AllyHP, AllyMaxHP, *EnemyName, EnemyHP, EnemyMaxHP);
            
            // ダメージ表示行（記号を安全な文字に）
            if (bAllyAttacked)
            {
                // 味方が攻撃を受けた場合：左端にダメージ
                FString CriticalText = CombatData.bIsCritical ? TEXT(" クリティカル！") : TEXT("");
                DetailsText += FString::Printf(TEXT("(%d) <- %s攻撃%s"),
                    CombatData.Damage, *CombatData.WeaponName, *CriticalText);
            }
            else
            {
                // 敵が攻撃を受けた場合：右端にダメージ
                FString CriticalText = CombatData.bIsCritical ? TEXT(" クリティカル！") : TEXT("");
                DetailsText += FString::Printf(TEXT("          %s攻撃%s -> (%d)"),
                    *CombatData.WeaponName, *CriticalText, CombatData.Damage);
            }
            
            DetailsText += TEXT("\n\n");
        }
        else
        {
            DetailsText += LogEntry.FormattedText + TEXT("\n");
        }
    }
    
    return DetailsText;
}
