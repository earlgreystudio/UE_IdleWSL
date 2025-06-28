#include "C_TeamTaskCard.h"
#include "../Components/TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UC_TeamTaskCard::NativeConstruct()
{
    Super::NativeConstruct();

    // ボタンイベントのバインド
    if (PriorityUpButton)
    {
        PriorityUpButton->OnClicked.AddDynamic(this, &UC_TeamTaskCard::OnPriorityUpClicked);
    }

    if (PriorityDownButton)
    {
        PriorityDownButton->OnClicked.AddDynamic(this, &UC_TeamTaskCard::OnPriorityDownClicked);
    }

    if (DeleteButton)
    {
        DeleteButton->OnClicked.AddDynamic(this, &UC_TeamTaskCard::OnDeleteClicked);
    }
}

void UC_TeamTaskCard::NativeDestruct()
{
    Super::NativeDestruct();
}

void UC_TeamTaskCard::InitializeWithTeamTask(const FTeamTask& InTask, int32 InTeamIndex, int32 InTaskPriority, UTeamComponent* InTeamComponent)
{
    TaskData = InTask;
    TeamIndex = InTeamIndex;
    TaskPriority = InTaskPriority;
    TeamComponent = InTeamComponent;

    UpdateDisplay();
}

void UC_TeamTaskCard::UpdateDisplay()
{
    UpdateTaskTypeDisplay();
    UpdatePriorityDisplay();
    UpdateSkillsDisplay();
    UpdateStatusDisplay();
    UpdateButtonStates();
}

void UC_TeamTaskCard::OnTeamMembersChanged()
{
    // チームメンバーが変更されたらスキル表示を更新
    UpdateSkillsDisplay();
}

void UC_TeamTaskCard::UpdateTaskTypeDisplay()
{
    if (TaskTypeText)
    {
        FString TypeName = UTaskTypeUtils::GetTaskTypeDisplayName(TaskData.TaskType);
        TaskTypeText->SetText(FText::FromString(TypeName));
    }
}

void UC_TeamTaskCard::UpdatePriorityDisplay()
{
    if (PriorityText)
    {
        // 表示は1から始めるため、インデックスに1を加える
        FString PriorityStr = FString::Printf(TEXT("優先度: %d"), TaskPriority + 1);
        PriorityText->SetText(FText::FromString(PriorityStr));
    }
}

void UC_TeamTaskCard::UpdateSkillsDisplay()
{
    if (!SkillsBox)
    {
        return;
    }

    // 既存のスキル表示をクリア
    SkillsBox->ClearChildren();

    // タスクに関連するスキルを取得
    FTaskRelatedSkills RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(TaskData.TaskType);
    
    // 主要スキル表示
    if (!RelatedSkills.PrimarySkill.IsEmpty())
    {
        float TotalValue = CalculateTeamSkillTotal(RelatedSkills.PrimarySkill);
        FString DisplayName = UTaskTypeUtils::ConvertSkillNameToPropertyName(RelatedSkills.PrimarySkill);
        FString SkillText = FString::Printf(TEXT("%s: %.0f (重要度: %.0f%%)"), 
            *DisplayName, TotalValue, RelatedSkills.PrimaryWeight * 100.0f);
        
        if (UTextBlock* SkillTextBlock = NewObject<UTextBlock>(this))
        {
            SkillTextBlock->SetText(FText::FromString(SkillText));
            SkillsBox->AddChildToVerticalBox(SkillTextBlock);
        }
    }

    // 副次スキル表示
    if (!RelatedSkills.SecondarySkill.IsEmpty())
    {
        float TotalValue = CalculateTeamSkillTotal(RelatedSkills.SecondarySkill);
        FString DisplayName = UTaskTypeUtils::ConvertSkillNameToPropertyName(RelatedSkills.SecondarySkill);
        FString SkillText = FString::Printf(TEXT("%s: %.0f (重要度: %.0f%%)"), 
            *DisplayName, TotalValue, RelatedSkills.SecondaryWeight * 100.0f);
        
        if (UTextBlock* SkillTextBlock = NewObject<UTextBlock>(this))
        {
            SkillTextBlock->SetText(FText::FromString(SkillText));
            SkillsBox->AddChildToVerticalBox(SkillTextBlock);
        }
    }

    // 補助スキル表示
    if (!RelatedSkills.TertiarySkill.IsEmpty())
    {
        float TotalValue = CalculateTeamSkillTotal(RelatedSkills.TertiarySkill);
        FString DisplayName = UTaskTypeUtils::ConvertSkillNameToPropertyName(RelatedSkills.TertiarySkill);
        FString SkillText = FString::Printf(TEXT("%s: %.0f (重要度: %.0f%%)"), 
            *DisplayName, TotalValue, RelatedSkills.TertiaryWeight * 100.0f);
        
        if (UTextBlock* SkillTextBlock = NewObject<UTextBlock>(this))
        {
            SkillTextBlock->SetText(FText::FromString(SkillText));
            SkillsBox->AddChildToVerticalBox(SkillTextBlock);
        }
    }

    // スキルがない場合
    if (SkillsBox->GetChildrenCount() == 0)
    {
        if (UTextBlock* NoSkillText = NewObject<UTextBlock>(this))
        {
            NoSkillText->SetText(FText::FromString(TEXT("スキル不要")));
            SkillsBox->AddChildToVerticalBox(NoSkillText);
        }
    }
}

void UC_TeamTaskCard::UpdateStatusDisplay()
{
    if (StatusText)
    {
        FString StatusStr;
        
        if (TaskData.bIsActive)
        {
            StatusStr = TEXT("実行中");
            
            // 進行状況を追加
            if (TaskData.EstimatedCompletionTime > 0.0f)
            {
                float ElapsedTime = TaskData.GetElapsedTime(GetWorld()->GetTimeSeconds());
                float Progress = (ElapsedTime / (TaskData.EstimatedCompletionTime * 3600.0f)) * 100.0f;
                StatusStr += FString::Printf(TEXT(" (%.1f%%)"), FMath::Clamp(Progress, 0.0f, 100.0f));
            }
        }
        else
        {
            StatusStr = TEXT("待機中");
        }
        
        StatusText->SetText(FText::FromString(StatusStr));
    }
}

void UC_TeamTaskCard::UpdateButtonStates()
{
    if (!TeamComponent)
    {
        return;
    }

    TArray<FTeamTask> TeamTasks = TeamComponent->GetTeamTasks(TeamIndex);
    
    // 優先度上げボタン（インデックス0は最高優先度なので上げられない）
    if (PriorityUpButton)
    {
        bool bCanMoveUp = TaskPriority > 0;
        PriorityUpButton->SetIsEnabled(bCanMoveUp);
        UE_LOG(LogTemp, Warning, TEXT("UpdateButtonStates: PriorityUp enabled=%s (TaskPriority=%d)"), 
               bCanMoveUp ? TEXT("true") : TEXT("false"), TaskPriority);
    }

    // 優先度下げボタン（最後のインデックスは下げられない）
    if (PriorityDownButton)
    {
        bool bCanMoveDown = TaskPriority < TeamTasks.Num() - 1;
        PriorityDownButton->SetIsEnabled(bCanMoveDown);
        UE_LOG(LogTemp, Warning, TEXT("UpdateButtonStates: PriorityDown enabled=%s (TaskPriority=%d, Total=%d)"), 
               bCanMoveDown ? TEXT("true") : TEXT("false"), TaskPriority, TeamTasks.Num());
    }
}

float UC_TeamTaskCard::CalculateTeamSkillTotal(const FString& SkillPropertyName) const
{
    TArray<AC_IdleCharacter*> Members = GetTeamMembers();
    float TotalValue = 0.0f;

    for (AC_IdleCharacter* Member : Members)
    {
        if (Member)
        {
            FDerivedStats Stats = Member->GetDerivedStats();
            TotalValue += GetSkillValueFromStats(Stats, SkillPropertyName);
        }
    }

    return TotalValue;
}

float UC_TeamTaskCard::GetSkillValueFromStats(const FDerivedStats& Stats, const FString& PropertyName) const
{
    // プロパティ名から対応する値を取得
    if (PropertyName == TEXT("ConstructionPower"))
    {
        return Stats.ConstructionPower;
    }
    else if (PropertyName == TEXT("ProductionPower"))
    {
        return Stats.ProductionPower;
    }
    else if (PropertyName == TEXT("GatheringPower"))
    {
        return Stats.GatheringPower;
    }
    else if (PropertyName == TEXT("CookingPower"))
    {
        return Stats.CookingPower;
    }
    else if (PropertyName == TEXT("CraftingPower"))
    {
        return Stats.CraftingPower;
    }
    else if (PropertyName == TEXT("CombatPower"))
    {
        return Stats.CombatPower;
    }
    else if (PropertyName == TEXT("AttackSpeed"))
    {
        return Stats.AttackSpeed;
    }
    else if (PropertyName == TEXT("DefenseValue"))
    {
        return static_cast<float>(Stats.DefenseValue);
    }
    else if (PropertyName == TEXT("HitChance"))
    {
        return Stats.HitChance;
    }
    else if (PropertyName == TEXT("CriticalChance"))
    {
        return Stats.CriticalChance;
    }
    else if (PropertyName == TEXT("DodgeChance"))
    {
        return Stats.DodgeChance;
    }
    else if (PropertyName == TEXT("WorkPower"))
    {
        return Stats.WorkPower;
    }

    // デフォルト値
    return 0.0f;
}

TArray<AC_IdleCharacter*> UC_TeamTaskCard::GetTeamMembers() const
{
    if (!TeamComponent)
    {
        return TArray<AC_IdleCharacter*>();
    }

    FTeam Team = TeamComponent->GetTeam(TeamIndex);
    return Team.Members;
}

void UC_TeamTaskCard::OnPriorityUpClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskCard::OnPriorityUpClicked - Team %d, Task Priority %d, Index %d"), TeamIndex, TaskData.Priority, TaskPriority);
    
    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskCard::OnPriorityUpClicked - TeamComponent is null"));
        return;
    }

    // 優先度を上げる = インデックスを下げる（0が最高優先度）
    if (TaskPriority > 0)
    {
        // 現在位置の1つ上（インデックスが小さい）のタスクと交換
        TArray<FTeamTask> TeamTasks = TeamComponent->GetTeamTasks(TeamIndex);
        
        if (TeamTasks.IsValidIndex(TaskPriority) && TeamTasks.IsValidIndex(TaskPriority - 1))
        {
            // 両方のタスクを削除
            FTeamTask CurrentTask = TeamTasks[TaskPriority];
            FTeamTask OtherTask = TeamTasks[TaskPriority - 1];
            
            // 優先度値を交換
            int32 TempPriority = CurrentTask.Priority;
            CurrentTask.Priority = OtherTask.Priority;
            OtherTask.Priority = TempPriority;
            
            // 両方削除してから追加し直す
            TeamComponent->RemoveTeamTask(TeamIndex, CurrentTask.Priority);
            TeamComponent->RemoveTeamTask(TeamIndex, OtherTask.Priority);
            TeamComponent->AddTeamTask(TeamIndex, OtherTask);
            TeamComponent->AddTeamTask(TeamIndex, CurrentTask);
            
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskCard: Swapped task priorities"));
        }
    }
}

void UC_TeamTaskCard::OnPriorityDownClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskCard::OnPriorityDownClicked - Team %d, Task Priority %d, Index %d"), TeamIndex, TaskData.Priority, TaskPriority);
    
    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskCard::OnPriorityDownClicked - TeamComponent is null"));
        return;
    }

    // 優先度を下げる = インデックスを上げる
    TArray<FTeamTask> TeamTasks = TeamComponent->GetTeamTasks(TeamIndex);
    
    if (TaskPriority < TeamTasks.Num() - 1)
    {
        // 現在位置の1つ下（インデックスが大きい）のタスクと交換
        if (TeamTasks.IsValidIndex(TaskPriority) && TeamTasks.IsValidIndex(TaskPriority + 1))
        {
            // 両方のタスクを削除
            FTeamTask CurrentTask = TeamTasks[TaskPriority];
            FTeamTask OtherTask = TeamTasks[TaskPriority + 1];
            
            // 優先度値を交換
            int32 TempPriority = CurrentTask.Priority;
            CurrentTask.Priority = OtherTask.Priority;
            OtherTask.Priority = TempPriority;
            
            // 両方削除してから追加し直す
            TeamComponent->RemoveTeamTask(TeamIndex, CurrentTask.Priority);
            TeamComponent->RemoveTeamTask(TeamIndex, OtherTask.Priority);
            TeamComponent->AddTeamTask(TeamIndex, OtherTask);
            TeamComponent->AddTeamTask(TeamIndex, CurrentTask);
            
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskCard: Swapped task priorities"));
        }
    }
}

void UC_TeamTaskCard::OnDeleteClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskCard::OnDeleteClicked - Team %d, Priority %d"), TeamIndex, TaskData.Priority);
    
    // 削除確認ダイアログを表示（Blueprint実装）
    // もしBlueprint実装がない場合は直接削除
    ShowDeleteConfirmDialog();
    
    // Blueprintイベントがない場合の直接削除（テスト用）
    // 実際のプロジェクトでは削除確認ダイアログを実装することを推奨
    ConfirmDelete();
}

void UC_TeamTaskCard::ConfirmDelete()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskCard::ConfirmDelete - Team %d, Priority %d"), TeamIndex, TaskData.Priority);
    
    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskCard::ConfirmDelete - TeamComponent is null"));
        return;
    }

    // TaskData.Priorityを使用（実際の優先度値）
    if (TeamComponent->RemoveTeamTask(TeamIndex, TaskData.Priority))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamTaskCard: Deleted team task with priority %d from team %d"), TaskData.Priority, TeamIndex);
    }
}
