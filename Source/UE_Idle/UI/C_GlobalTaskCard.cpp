#include "C_GlobalTaskCard.h"
#include "../Components/TaskManagerComponent.h"
#include "../Managers/ItemDataTableManager.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Engine/GameInstance.h"

void UC_GlobalTaskCard::NativeConstruct()
{
    Super::NativeConstruct();

    // ボタンイベントのバインド
    if (PriorityUpButton)
    {
        PriorityUpButton->OnClicked.AddDynamic(this, &UC_GlobalTaskCard::OnPriorityUpClicked);
    }

    if (PriorityDownButton)
    {
        PriorityDownButton->OnClicked.AddDynamic(this, &UC_GlobalTaskCard::OnPriorityDownClicked);
    }

    if (DeleteButton)
    {
        DeleteButton->OnClicked.AddDynamic(this, &UC_GlobalTaskCard::OnDeleteClicked);
    }

    // ItemDataManagerの取得
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        ItemDataManager = GameInstance->GetSubsystem<UItemDataTableManager>();
    }
}

void UC_GlobalTaskCard::NativeDestruct()
{
    Super::NativeDestruct();
}

void UC_GlobalTaskCard::InitializeWithTask(const FGlobalTask& InTask, int32 InTaskIndex, UTaskManagerComponent* InTaskManager)
{
    UE_LOG(LogTemp, Warning, TEXT("=== GlobalTaskCard::InitializeWithTask START ==="));
    UE_LOG(LogTemp, Warning, TEXT("Task: %s (Index: %d)"), *InTask.DisplayName, InTaskIndex);
    
    TaskData = InTask;
    TaskIndex = InTaskIndex;
    TaskManager = InTaskManager;

    UE_LOG(LogTemp, Warning, TEXT("Calling UpdateDisplay..."));
    UpdateDisplay();
    
    UE_LOG(LogTemp, Warning, TEXT("=== GlobalTaskCard::InitializeWithTask COMPLETED ==="));
}

void UC_GlobalTaskCard::UpdateDisplay()
{
    UE_LOG(LogTemp, Warning, TEXT("=== GlobalTaskCard::UpdateDisplay START ==="));
    
    UpdateTaskTypeDisplay();
    UpdateItemNameDisplay();
    UpdateQuantityDisplay();
    UpdateSkillsDisplay();
    UpdatePriorityDisplay();
    UpdateProgressDisplay();
    UpdateButtonStates();
    
    UE_LOG(LogTemp, Warning, TEXT("=== GlobalTaskCard::UpdateDisplay COMPLETED ==="));
}

void UC_GlobalTaskCard::UpdateTaskTypeDisplay()
{
    UE_LOG(LogTemp, Warning, TEXT("UpdateTaskTypeDisplay: TaskTypeText = %s"), TaskTypeText ? TEXT("OK") : TEXT("NULL"));
    
    if (TaskTypeText)
    {
        FString TypeName = UTaskTypeUtils::GetTaskTypeDisplayName(TaskData.TaskType);
        TaskTypeText->SetText(FText::FromString(TypeName));
        UE_LOG(LogTemp, Warning, TEXT("TaskTypeText set to: %s"), *TypeName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TaskTypeText is NULL - BindWidget not working"));
    }
}

void UC_GlobalTaskCard::UpdateItemNameDisplay()
{
    if (ItemNameText)
    {
        FString ItemName = GetItemDisplayName(TaskData.TargetItemId);
        ItemNameText->SetText(FText::FromString(ItemName));
    }
}

void UC_GlobalTaskCard::UpdateQuantityDisplay()
{
    if (QuantityText)
    {
        FString QuantityStr;
        
        // 採集タスクの数量タイプ別表示
        if (TaskData.TaskType == ETaskType::Gathering)
        {
            switch (TaskData.GatheringQuantityType)
            {
                case EGatheringQuantityType::Unlimited:
                    QuantityStr = TEXT("無制限");
                    break;
                case EGatheringQuantityType::Keep:
                    QuantityStr = FString::Printf(TEXT("%d をキープ"), TaskData.TargetQuantity);
                    break;
                case EGatheringQuantityType::Specified:
                    QuantityStr = FString::Printf(TEXT("%d 個"), TaskData.TargetQuantity);
                    break;
                default:
                    QuantityStr = FString::Printf(TEXT("%d"), TaskData.TargetQuantity);
                    break;
            }
        }
        else
        {
            // 他のタスクタイプは通常表示
            QuantityStr = FString::Printf(TEXT("%d"), TaskData.TargetQuantity);
            
            // キープフラグがtrueなら「をキープ」を追加（廃止予定フラグ）
            if (TaskData.bIsKeepQuantity)
            {
                QuantityStr += TEXT(" をキープ");
            }
        }
        
        QuantityText->SetText(FText::FromString(QuantityStr));
    }
}

void UC_GlobalTaskCard::UpdateSkillsDisplay()
{
    if (SkillsText)
    {
        TArray<FString> SkillNames = UTaskTypeUtils::GetTaskSkillDisplayNames(TaskData.TaskType);
        FString SkillsStr = TEXT("必要スキル: ");
        
        if (SkillNames.Num() > 0)
        {
            SkillsStr += FString::Join(SkillNames, TEXT(", "));
        }
        else
        {
            SkillsStr += TEXT("なし");
        }
        
        SkillsText->SetText(FText::FromString(SkillsStr));
    }
}

void UC_GlobalTaskCard::UpdatePriorityDisplay()
{
    if (PriorityText)
    {
        FString PriorityStr = FString::Printf(TEXT("優先度: %d"), TaskData.Priority);
        PriorityText->SetText(FText::FromString(PriorityStr));
    }
}

void UC_GlobalTaskCard::UpdateProgressDisplay()
{
    if (ProgressBox)
    {
        // タスクは完了時に削除されるため、常に進行中として表示
        ProgressBox->SetVisibility(ESlateVisibility::Visible);
        
        if (ProgressText)
        {
            FString ProgressStr;
            
            // 無制限採集タスクの場合は特別な表示
            if (TaskData.TaskType == ETaskType::Gathering && TaskData.GatheringQuantityType == EGatheringQuantityType::Unlimited)
            {
                ProgressStr = FString::Printf(TEXT("進行: %d (無制限)"), TaskData.CurrentProgress);
            }
            else
            {
                ProgressStr = FString::Printf(TEXT("進行: %d / %d (%.1f%%)"),
                    TaskData.CurrentProgress,
                    TaskData.TargetQuantity,
                    TaskData.GetProgressRatio() * 100.0f);
            }
            
            ProgressText->SetText(FText::FromString(ProgressStr));
        }
    }
}

FString UC_GlobalTaskCard::GetItemDisplayName(const FString& ItemId) const
{
    UE_LOG(LogTemp, Warning, TEXT("=== GetItemDisplayName START: ItemId = '%s' ==="), *ItemId);
    
    if (ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GetItemDisplayName: ItemId is empty"));
        return TEXT("アイテム未設定");
    }

    // ItemDataTableManagerからアイテム名を取得
    if (ItemDataManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetItemDisplayName: ItemDataManager exists, calling GetItemData"));
        FItemDataRow ItemData;
        if (ItemDataManager->GetItemData(ItemId, ItemData))
        {
            FString DisplayName = ItemData.Name.ToString();
            UE_LOG(LogTemp, Warning, TEXT("GetItemDisplayName: SUCCESS - %s -> '%s'"), *ItemId, *DisplayName);
            return DisplayName;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GetItemDisplayName: FAILED to get item data for '%s'"), *ItemId);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GetItemDisplayName: ItemDataManager is NULL"));
    }

    // 取得できない場合はIDをそのまま返す
    UE_LOG(LogTemp, Error, TEXT("GetItemDisplayName: FALLBACK - returning ItemId '%s'"), *ItemId);
    return ItemId;
}

void UC_GlobalTaskCard::UpdateButtonStates()
{
    if (!TaskManager)
    {
        return;
    }

    // 優先度上げボタン（優先度1が最高なので無効化）
    if (PriorityUpButton)
    {
        PriorityUpButton->SetIsEnabled(TaskData.Priority > 1);
    }

    // 優先度下げボタン（最大優先度まで下げられる）
    if (PriorityDownButton)
    {
        int32 MaxPriority = TaskManager->GetGlobalTasks().Num();
        PriorityDownButton->SetIsEnabled(TaskData.Priority < MaxPriority);
    }
}

void UC_GlobalTaskCard::OnPriorityUpClicked()
{
    if (!TaskManager)
    {
        return;
    }

    // 優先度を上げる = 数値を下げる
    if (TaskManager->MoveTaskUp(TaskIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_GlobalTaskCard: Moved task %s up"), *TaskData.TaskId);
    }
}

void UC_GlobalTaskCard::OnPriorityDownClicked()
{
    if (!TaskManager)
    {
        return;
    }

    // 優先度を下げる = 数値を上げる
    if (TaskManager->MoveTaskDown(TaskIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_GlobalTaskCard: Moved task %s down"), *TaskData.TaskId);
    }
}

void UC_GlobalTaskCard::OnDeleteClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_GlobalTaskCard::OnDeleteClicked - Task %s (Index: %d)"), *TaskData.TaskId, TaskIndex);
    
    // 削除確認ダイアログを表示（Blueprint実装）
    ShowDeleteConfirmDialog();
    
    // Blueprint実装がない場合の直接削除（テスト用）
    // 実際のプロジェクトでは削除確認ダイアログを実装することを推奨
    ConfirmDelete();
}

void UC_GlobalTaskCard::ConfirmDelete()
{
    if (!TaskManager)
    {
        return;
    }

    if (TaskManager->RemoveGlobalTask(TaskIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_GlobalTaskCard: Deleted task %s"), *TaskData.TaskId);
    }
}
