#include "TaskManagerComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Components/TeamComponent.h"
#include "Engine/World.h"

UTaskManagerComponent::UTaskManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bProcessingTasks = false;
    MaxGlobalTasks = 20;
}

void UTaskManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("TaskManagerComponent: BeginPlay - Initialized"));
}

void UTaskManagerComponent::BeginDestroy()
{
    // 処理中フラグをクリア
    bProcessingTasks = false;
    
    // 参照をクリア
    GlobalInventoryRef = nullptr;
    TeamComponentRef = nullptr;
    
    // タイマーをクリア
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }
    
    UE_LOG(LogTemp, Log, TEXT("TaskManagerComponent: BeginDestroy - Cleaned up"));
    
    Super::BeginDestroy();
}

// === タスク管理機能 ===

int32 UTaskManagerComponent::AddGlobalTask(const FGlobalTask& NewTask)
{
    if (bProcessingTasks)
    {
        LogError(TEXT("AddGlobalTask: Cannot add task while processing"));
        return -1;
    }

    if (!ValidateTask(NewTask))
    {
        LogError(TEXT("AddGlobalTask: Invalid task provided"));
        return -1;
    }

    if (GlobalTasks.Num() >= MaxGlobalTasks)
    {
        LogError(FString::Printf(TEXT("AddGlobalTask: Maximum task limit (%d) reached"), MaxGlobalTasks));
        return -1;
    }

    // 優先度の重複チェック
    if (HasDuplicatePriority(NewTask.Priority))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddGlobalTask: Priority %d already exists, adjusting priorities"), NewTask.Priority);
        
        // 既存タスクの優先度を調整
        for (FGlobalTask& ExistingTask : GlobalTasks)
        {
            if (ExistingTask.Priority >= NewTask.Priority)
            {
                ExistingTask.Priority++;
            }
        }
    }

    FGlobalTask TaskToAdd = NewTask;
    TaskToAdd.CreatedTime = FDateTime::Now();
    
    // タスクタイプに基づいて関連スキルを自動設定
    if (!TaskToAdd.RelatedSkills.HasSkills())
    {
        TaskToAdd.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(TaskToAdd.TaskType);
    }
    
    int32 NewIndex = GlobalTasks.Add(TaskToAdd);
    
    LogTaskOperation(TEXT("Added"), TaskToAdd);
    OnGlobalTaskAdded.Broadcast(TaskToAdd);
    
    return NewIndex;
}

bool UTaskManagerComponent::RemoveGlobalTask(int32 TaskIndex)
{
    if (bProcessingTasks)
    {
        LogError(TEXT("RemoveGlobalTask: Cannot remove task while processing"));
        return false;
    }

    if (!IsValidTaskIndex(TaskIndex))
    {
        LogError(FString::Printf(TEXT("RemoveGlobalTask: Invalid task index %d"), TaskIndex));
        return false;
    }

    FGlobalTask RemovedTask = GlobalTasks[TaskIndex];
    GlobalTasks.RemoveAt(TaskIndex);
    
    LogTaskOperation(TEXT("Removed"), RemovedTask);
    OnGlobalTaskRemoved.Broadcast(TaskIndex);
    
    // 優先度を再計算
    RecalculatePriorities();
    
    return true;
}

bool UTaskManagerComponent::UpdateTaskPriority(int32 TaskIndex, int32 NewPriority)
{
    if (bProcessingTasks)
    {
        LogError(TEXT("UpdateTaskPriority: Cannot update priority while processing"));
        return false;
    }

    if (!IsValidTaskIndex(TaskIndex))
    {
        LogError(FString::Printf(TEXT("UpdateTaskPriority: Invalid task index %d"), TaskIndex));
        return false;
    }

    if (NewPriority < 1 || NewPriority > MaxGlobalTasks)
    {
        LogError(FString::Printf(TEXT("UpdateTaskPriority: Invalid priority %d (valid range: 1-%d)"), NewPriority, MaxGlobalTasks));
        return false;
    }

    int32 OldPriority = GlobalTasks[TaskIndex].Priority;
    GlobalTasks[TaskIndex].Priority = NewPriority;
    
    UE_LOG(LogTemp, Log, TEXT("UpdateTaskPriority: Task %s priority changed from %d to %d"), 
           *GlobalTasks[TaskIndex].TaskId, OldPriority, NewPriority);
    
    OnTaskPriorityChanged.Broadcast(TaskIndex, NewPriority);
    
    return true;
}

TArray<FGlobalTask> UTaskManagerComponent::GetGlobalTasksByPriority() const
{
    TArray<FGlobalTask> SortedTasks = GlobalTasks;
    
    // 優先度順にソート（1が最高優先度）
    SortedTasks.Sort([](const FGlobalTask& A, const FGlobalTask& B) {
        return A.Priority < B.Priority;
    });
    
    return SortedTasks;
}

FGlobalTask UTaskManagerComponent::GetGlobalTask(int32 TaskIndex) const
{
    if (IsValidTaskIndex(TaskIndex))
    {
        return GlobalTasks[TaskIndex];
    }
    
    UE_LOG(LogTemp, Warning, TEXT("GetGlobalTask: Invalid task index %d, returning default task"), TaskIndex);
    return FGlobalTask(); // デフォルトタスク
}

// === 優先度管理機能 ===

bool UTaskManagerComponent::SwapTaskPriority(int32 TaskIndex1, int32 TaskIndex2)
{
    if (!IsValidTaskIndex(TaskIndex1) || !IsValidTaskIndex(TaskIndex2))
    {
        LogError(FString::Printf(TEXT("SwapTaskPriority: Invalid task indices %d, %d"), TaskIndex1, TaskIndex2));
        return false;
    }

    if (TaskIndex1 == TaskIndex2)
    {
        return true; // 同じインデックスなので何もしない
    }

    int32 TempPriority = GlobalTasks[TaskIndex1].Priority;
    GlobalTasks[TaskIndex1].Priority = GlobalTasks[TaskIndex2].Priority;
    GlobalTasks[TaskIndex2].Priority = TempPriority;
    
    UE_LOG(LogTemp, Log, TEXT("SwapTaskPriority: Swapped priorities between tasks %d and %d"), TaskIndex1, TaskIndex2);
    
    OnTaskPriorityChanged.Broadcast(TaskIndex1, GlobalTasks[TaskIndex1].Priority);
    OnTaskPriorityChanged.Broadcast(TaskIndex2, GlobalTasks[TaskIndex2].Priority);
    
    return true;
}

bool UTaskManagerComponent::MoveTaskUp(int32 TaskIndex)
{
    if (!IsValidTaskIndex(TaskIndex))
    {
        return false;
    }

    TArray<FGlobalTask> SortedTasks = GetGlobalTasksByPriority();
    
    // 現在のタスクを探す
    int32 CurrentPosition = -1;
    for (int32 i = 0; i < SortedTasks.Num(); i++)
    {
        if (SortedTasks[i].TaskId == GlobalTasks[TaskIndex].TaskId)
        {
            CurrentPosition = i;
            break;
        }
    }

    if (CurrentPosition <= 0)
    {
        return false; // 既に最高優先度または見つからない
    }

    // 1つ上のタスクと優先度を交換
    for (int32 i = 0; i < GlobalTasks.Num(); i++)
    {
        if (GlobalTasks[i].TaskId == SortedTasks[CurrentPosition - 1].TaskId)
        {
            return SwapTaskPriority(TaskIndex, i);
        }
    }

    return false;
}

bool UTaskManagerComponent::MoveTaskDown(int32 TaskIndex)
{
    if (!IsValidTaskIndex(TaskIndex))
    {
        return false;
    }

    TArray<FGlobalTask> SortedTasks = GetGlobalTasksByPriority();
    
    // 現在のタスクを探す
    int32 CurrentPosition = -1;
    for (int32 i = 0; i < SortedTasks.Num(); i++)
    {
        if (SortedTasks[i].TaskId == GlobalTasks[TaskIndex].TaskId)
        {
            CurrentPosition = i;
            break;
        }
    }

    if (CurrentPosition < 0 || CurrentPosition >= SortedTasks.Num() - 1)
    {
        return false; // 既に最低優先度または見つからない
    }

    // 1つ下のタスクと優先度を交換
    for (int32 i = 0; i < GlobalTasks.Num(); i++)
    {
        if (GlobalTasks[i].TaskId == SortedTasks[CurrentPosition + 1].TaskId)
        {
            return SwapTaskPriority(TaskIndex, i);
        }
    }

    return false;
}

void UTaskManagerComponent::RecalculatePriorities()
{
    if (GlobalTasks.Num() == 0)
    {
        return;
    }

    // 優先度順にソートしてから連番を振り直す
    TArray<FGlobalTask> SortedTasks = GetGlobalTasksByPriority();
    
    for (int32 i = 0; i < SortedTasks.Num(); i++)
    {
        // 元の配列で該当タスクを見つけて更新
        for (FGlobalTask& Task : GlobalTasks)
        {
            if (Task.TaskId == SortedTasks[i].TaskId)
            {
                Task.Priority = i + 1; // 1から開始
                break;
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("RecalculatePriorities: Recalculated priorities for %d tasks"), GlobalTasks.Num());
}

// === 「全て」モード用タスク選択 ===

FGlobalTask UTaskManagerComponent::GetNextAvailableTask(int32 TeamIndex) const
{
    if (!IsValid(TeamComponentRef))
    {
        LogError(TEXT("GetNextAvailableTask: TeamComponent reference is null"));
        return FGlobalTask();
    }

    TArray<FGlobalTask> SortedTasks = GetGlobalTasksByPriority();
    
    for (const FGlobalTask& Task : SortedTasks)
    {
        if (Task.bIsCompleted)
        {
            continue; // 完了済みタスクはスキップ
        }

        if (CanTeamExecuteTask(TeamIndex, Task))
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("GetNextAvailableTask: Found available task %s for team %d"), 
                   *Task.TaskId, TeamIndex);
            return Task;
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("GetNextAvailableTask: No available tasks found for team %d"), TeamIndex);
    return FGlobalTask(); // 実行可能なタスクなし
}

bool UTaskManagerComponent::CanTeamExecuteTask(int32 TeamIndex, const FGlobalTask& Task) const
{
    if (!IsValid(TeamComponentRef))
    {
        return false;
    }

    // チームの有効性チェック
    if (!TeamComponentRef->GetTeam(TeamIndex).IsValidTeam())
    {
        return false;
    }

    // タスクの有効性チェック
    if (!Task.IsValid() || Task.bIsCompleted)
    {
        return false;
    }

    // リソース要件チェック
    if (!CheckGlobalTaskRequirements(Task, TeamIndex))
    {
        return false;
    }

    // チームサイズチェック（最低1人必要）
    if (TeamComponentRef->GetTeam(TeamIndex).Members.Num() == 0)
    {
        return false;
    }

    return true;
}

// === リソース監視・判定 ===

bool UTaskManagerComponent::CheckResourceRequirements(const FTeamTask& Task) const
{
    if (!IsValid(GlobalInventoryRef))
    {
        LogError(TEXT("CheckResourceRequirements: GlobalInventory reference is null"));
        return false;
    }

    for (const auto& Requirement : Task.RequiredResources)
    {
        int32 TotalAmount = GetTotalResourceAmount(Requirement.Key);
        if (TotalAmount < Requirement.Value)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("CheckResourceRequirements: Insufficient %s (have %d, need %d)"), 
                   *Requirement.Key, TotalAmount, Requirement.Value);
            return false;
        }
    }

    return true;
}

int32 UTaskManagerComponent::GetTotalResourceAmount(const FString& ResourceId) const
{
    int32 TotalAmount = 0;

    // グローバルインベントリから取得
    if (IsValid(GlobalInventoryRef))
    {
        TotalAmount += GlobalInventoryRef->GetItemCount(ResourceId);
    }

    // 全チームのインベントリから取得
    if (IsValid(TeamComponentRef))
    {
        for (int32 i = 0; i < TeamComponentRef->GetTeams().Num(); i++)
        {
            if (UInventoryComponent* TeamInventory = TeamComponentRef->GetTeamInventoryComponent(i))
            {
                TotalAmount += TeamInventory->GetItemCount(ResourceId);
            }
        }
    }

    return TotalAmount;
}

bool UTaskManagerComponent::CheckGlobalTaskRequirements(const FGlobalTask& Task, int32 TeamIndex) const
{
    // 基本的な要件チェック（将来的にタスクタイプ別の詳細チェックを追加）
    switch (Task.TaskType)
    {
        case ETaskType::Construction:
            // 建築タスクは木材と石材が必要（例）
            return GetTotalResourceAmount(TEXT("wood")) >= 10 && 
                   GetTotalResourceAmount(TEXT("stone")) >= 5;
            
        case ETaskType::Cooking:
            // 料理タスクは食材が必要（例）
            return GetTotalResourceAmount(TEXT("ingredient")) >= 1;
            
        case ETaskType::Gathering:
            // 採集は常に実行可能
            return true;
            
        case ETaskType::Crafting:
            // 製作は材料が必要（例）
            return GetTotalResourceAmount(TEXT("material")) >= 1;
            
        default:
            return true;
    }
}

// === タスク完了処理 ===

void UTaskManagerComponent::ProcessTaskCompletion(const FString& TaskId, int32 CompletedAmount)
{
    if (bProcessingTasks)
    {
        return; // 重複処理防止
    }

    bProcessingTasks = true;

    for (FGlobalTask& Task : GlobalTasks)
    {
        if (Task.TaskId == TaskId && !Task.bIsCompleted)
        {
            Task.CurrentProgress += CompletedAmount;
            
            if (Task.CurrentProgress >= Task.TargetQuantity)
            {
                Task.bIsCompleted = true;
                LogTaskOperation(TEXT("Completed"), Task);
                OnGlobalTaskCompleted.Broadcast(Task);
            }
            else
            {
                UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessTaskCompletion: Task %s progress updated to %d/%d"), 
                       *TaskId, Task.CurrentProgress, Task.TargetQuantity);
            }
            break;
        }
    }

    bProcessingTasks = false;
}

bool UTaskManagerComponent::UpdateTaskProgress(const FString& TaskId, int32 ProgressAmount)
{
    int32 TaskIndex = FindTaskByID(TaskId);
    if (TaskIndex < 0)
    {
        return false;
    }

    FGlobalTask& Task = GlobalTasks[TaskIndex];
    if (Task.bIsCompleted)
    {
        return false;
    }

    Task.CurrentProgress = FMath::Max(0, Task.CurrentProgress + ProgressAmount);
    
    if (Task.CurrentProgress >= Task.TargetQuantity)
    {
        Task.bIsCompleted = true;
        OnGlobalTaskCompleted.Broadcast(Task);
    }

    return true;
}

void UTaskManagerComponent::ClearCompletedTasks()
{
    int32 RemovedCount = GlobalTasks.RemoveAll([](const FGlobalTask& Task) {
        return Task.bIsCompleted;
    });

    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("ClearCompletedTasks: Removed %d completed tasks"), RemovedCount);
        RecalculatePriorities();
    }
}

// === 参照設定 ===

void UTaskManagerComponent::SetGlobalInventoryReference(UInventoryComponent* InventoryComponent)
{
    if (IsValid(InventoryComponent))
    {
        GlobalInventoryRef = InventoryComponent;
        UE_LOG(LogTemp, Log, TEXT("TaskManagerComponent: GlobalInventory reference set"));
    }
    else
    {
        LogError(TEXT("SetGlobalInventoryReference: Invalid inventory component"));
    }
}

void UTaskManagerComponent::SetTeamComponentReference(UTeamComponent* TeamComponent)
{
    if (IsValid(TeamComponent))
    {
        TeamComponentRef = TeamComponent;
        UE_LOG(LogTemp, Log, TEXT("TaskManagerComponent: TeamComponent reference set"));
    }
    else
    {
        LogError(TEXT("SetTeamComponentReference: Invalid team component"));
    }
}

// === ユーティリティ ===

bool UTaskManagerComponent::IsValidTaskIndex(int32 TaskIndex) const
{
    return TaskIndex >= 0 && TaskIndex < GlobalTasks.Num();
}

int32 UTaskManagerComponent::FindTaskByID(const FString& TaskId) const
{
    for (int32 i = 0; i < GlobalTasks.Num(); i++)
    {
        if (GlobalTasks[i].TaskId == TaskId)
        {
            return i;
        }
    }
    return -1;
}

int32 UTaskManagerComponent::GetIncompleteTaskCount() const
{
    return GlobalTasks.FilterByPredicate([](const FGlobalTask& Task) {
        return !Task.bIsCompleted;
    }).Num();
}

int32 UTaskManagerComponent::GetCompletedTaskCount() const
{
    return GlobalTasks.FilterByPredicate([](const FGlobalTask& Task) {
        return Task.bIsCompleted;
    }).Num();
}

// === 内部ヘルパー関数 ===

bool UTaskManagerComponent::ValidateTask(const FGlobalTask& Task) const
{
    if (!Task.IsValid())
    {
        return false;
    }

    // 重複チェック
    if (FindTaskByID(Task.TaskId) >= 0)
    {
        LogError(FString::Printf(TEXT("ValidateTask: Task ID '%s' already exists"), *Task.TaskId));
        return false;
    }

    return true;
}

bool UTaskManagerComponent::HasDuplicatePriority(int32 Priority, int32 ExcludeIndex) const
{
    for (int32 i = 0; i < GlobalTasks.Num(); i++)
    {
        if (i != ExcludeIndex && GlobalTasks[i].Priority == Priority)
        {
            return true;
        }
    }
    return false;
}

bool UTaskManagerComponent::IsValidArrayAccess(int32 Index) const
{
    return Index >= 0 && Index < GlobalTasks.Num();
}

void UTaskManagerComponent::LogTaskOperation(const FString& Operation, const FGlobalTask& Task) const
{
    UE_LOG(LogTemp, Log, TEXT("TaskManager %s: %s (ID: %s, Priority: %d, Type: %s)"),
           *Operation, *Task.DisplayName, *Task.TaskId, Task.Priority,
           *UTaskTypeUtils::GetTaskTypeDisplayName(Task.TaskType));
}

void UTaskManagerComponent::LogError(const FString& ErrorMessage) const
{
    UE_LOG(LogTemp, Error, TEXT("TaskManagerComponent Error: %s"), *ErrorMessage);
}