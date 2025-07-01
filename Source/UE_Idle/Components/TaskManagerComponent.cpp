#include "TaskManagerComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Managers/LocationDataTableManager.h"
#include "../Types/LocationTypes.h"
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

bool UTaskManagerComponent::UpdateTaskTargetQuantity(int32 TaskIndex, int32 NewTargetQuantity)
{
    if (bIsProcessingTasks)
    {
        LogError(TEXT("UpdateTaskTargetQuantity: Cannot update quantity while processing"));
        return false;
    }
    
    if (!GlobalTasks.IsValidIndex(TaskIndex))
    {
        LogError(FString::Printf(TEXT("UpdateTaskTargetQuantity: Invalid task index %d"), TaskIndex));
        return false;
    }
    
    int32 OldQuantity = GlobalTasks[TaskIndex].TargetQuantity;
    GlobalTasks[TaskIndex].TargetQuantity = NewTargetQuantity;
    
    UE_LOG(LogTemp, Warning, TEXT("UpdateTaskTargetQuantity: Task %s quantity changed from %d to %d"),
           *GlobalTasks[TaskIndex].TaskId, OldQuantity, NewTargetQuantity);
    
    // UI更新のためイベントを発行
    OnTaskQuantityUpdated.Broadcast(TaskIndex, OldQuantity, NewTargetQuantity);
    
    return true;
}

bool UTaskManagerComponent::CompleteTask(int32 TaskIndex)
{
    if (bIsProcessingTasks)
    {
        LogError(TEXT("CompleteTask: Cannot complete task while processing"));
        return false;
    }
    
    if (!GlobalTasks.IsValidIndex(TaskIndex))
    {
        LogError(FString::Printf(TEXT("CompleteTask: Invalid task index %d"), TaskIndex));
        return false;
    }
    
    GlobalTasks[TaskIndex].bIsCompleted = true;
    
    UE_LOG(LogTemp, Warning, TEXT("CompleteTask: Task %s marked as completed"),
           *GlobalTasks[TaskIndex].TaskId);
    
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

    // 全チームメンバーの個人インベントリから取得
    if (IsValid(TeamComponentRef))
    {
        const TArray<FTeam>& Teams = TeamComponentRef->GetTeams();
        for (int32 i = 0; i < Teams.Num(); i++)
        {
            const FTeam& Team = Teams[i];
            for (AC_IdleCharacter* Member : Team.Members)
            {
                if (IsValid(Member))
                {
                    if (UInventoryComponent* MemberInventory = Member->GetInventoryComponent())
                    {
                        TotalAmount += MemberInventory->GetItemCount(ResourceId);
                    }
                }
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
            
        case ETaskType::Adventure:
            // 冒険タスクは常に実行可能（戦闘装備は後で実装）
            return true;
            
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

    int32 OldProgress = Task.CurrentProgress;
    Task.CurrentProgress = FMath::Max(0, Task.CurrentProgress + ProgressAmount);
    
    // 進捗ログは重要な場合のみ
    if (Task.CurrentProgress >= Task.TargetQuantity)
    {
        UE_LOG(LogTemp, Warning, TEXT("PROGRESS: %s completed %d/%d"), 
            *TaskId, Task.CurrentProgress, Task.TargetQuantity);
    }
    
    // 採集タスクの数量タイプ別完了判定
    if (Task.TaskType == ETaskType::Gathering)
    {
        switch (Task.GatheringQuantityType)
        {
            case EGatheringQuantityType::Unlimited:
                // 無制限採集は永続的に実行される（完了しない）
                UE_LOG(LogTemp, VeryVerbose, TEXT("Unlimited gathering task %s continues"), *TaskId);
                return true;
                
            case EGatheringQuantityType::Keep:
                // キープ型は永続的に実行される（完了しない）
                UE_LOG(LogTemp, VeryVerbose, TEXT("Keep quantity task %s continues"), *TaskId);
                return true;
                
            case EGatheringQuantityType::Specified:
                // 個数指定型のみ完了判定を行う
                if (Task.CurrentProgress >= Task.TargetQuantity && Task.TargetQuantity > 0)
                {
                    Task.bIsCompleted = true;
                    UE_LOG(LogTemp, Warning, TEXT("TASK COMPLETED: %s reached %d/%d - removing from list"), 
                        *TaskId, Task.CurrentProgress, Task.TargetQuantity);
                    OnGlobalTaskCompleted.Broadcast(Task);
                    
                    // 完了タスクを即座に削除
                    GlobalTasks.RemoveAt(TaskIndex);
                    RecalculatePriorities();
                    
                    // UI更新通知
                    OnGlobalTaskRemoved.Broadcast(TaskIndex);
                    
                    UE_LOG(LogTemp, Warning, TEXT("✅ Task %s auto-removed. Remaining: %d"), *TaskId, GlobalTasks.Num());
                }
                return true;
                
            default:
                // フォールバック：従来のbIsKeepQuantityロジック
                if (Task.bIsKeepQuantity)
                {
                    // キープ型は完了しない
                    return true;
                }
                else
                {
                    // 通常タスクは完了判定
                    if (Task.CurrentProgress >= Task.TargetQuantity && Task.TargetQuantity > 0)
                    {
                        Task.bIsCompleted = true;
                        UE_LOG(LogTemp, Warning, TEXT("TASK COMPLETED: %s reached %d/%d - removing from list"), 
                            *TaskId, Task.CurrentProgress, Task.TargetQuantity);
                        OnGlobalTaskCompleted.Broadcast(Task);
                        
                        GlobalTasks.RemoveAt(TaskIndex);
                        RecalculatePriorities();
                        OnGlobalTaskRemoved.Broadcast(TaskIndex);
                        
                        UE_LOG(LogTemp, Warning, TEXT("✅ Task %s auto-removed. Remaining: %d"), *TaskId, GlobalTasks.Num());
                    }
                }
                return true;
        }
    }
    
    // 他のタスクタイプの完了判定
    if (Task.CurrentProgress >= Task.TargetQuantity && Task.TargetQuantity > 0)
    {
        Task.bIsCompleted = true;
        UE_LOG(LogTemp, Warning, TEXT("TASK COMPLETED: %s reached %d/%d - removing from list"), 
            *TaskId, Task.CurrentProgress, Task.TargetQuantity);
        OnGlobalTaskCompleted.Broadcast(Task);
        
        // 完了タスクを即座に削除
        GlobalTasks.RemoveAt(TaskIndex);
        RecalculatePriorities();
        
        // UI更新通知
        OnGlobalTaskRemoved.Broadcast(TaskIndex);
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Task %s auto-removed. Remaining: %d"), *TaskId, GlobalTasks.Num());
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

// === 採集継続判定機能 ===

bool UTaskManagerComponent::ShouldContinueGathering(int32 TeamIndex, const FString& ItemId) const
{
    if (!IsValid(TeamComponentRef))
    {
        LogError(TEXT("ShouldContinueGathering: TeamComponent reference not set"));
        return false;
    }

    // 1. 該当アイテムのアクティブなタスクを検索
    FGlobalTask GatheringTask = FindActiveGatheringTask(ItemId);
    if (GatheringTask.TaskId.IsEmpty())
    {
        return false;
    }

    // 2. 現在の利用可能アイテム数を取得
    int32 CurrentAvailable = GetCurrentItemAvailability(TeamIndex, ItemId);
    
    // 3. 新しいGatheringQuantityTypeに基づく判定
    switch (GatheringTask.GatheringQuantityType)
    {
        case EGatheringQuantityType::Unlimited:
            // 無制限：常に継続
            return true;
            
        case EGatheringQuantityType::Keep:
            // キープ型：目標数量を下回っている場合は継続
            return CurrentAvailable < GatheringTask.TargetQuantity;
            
        case EGatheringQuantityType::Specified:
            // 個数指定型：目標数量に達していない場合は継続
            return CurrentAvailable < GatheringTask.TargetQuantity && !GatheringTask.bIsCompleted;
            
        default:
            // フォールバック（従来のbIsKeepQuantityロジック）
            if (GatheringTask.bIsKeepQuantity)
            {
                return CurrentAvailable < GatheringTask.TargetQuantity;
            }
            else
            {
                return CurrentAvailable < GatheringTask.TargetQuantity && !GatheringTask.bIsCompleted;
            }
    }
}

int32 UTaskManagerComponent::GetCurrentItemAvailability(int32 TeamIndex, const FString& ItemId) const
{
    int32 TotalAvailable = 0;
    
    // 1. 拠点倉庫の数量を取得
    if (IsValid(GlobalInventoryRef))
    {
        TotalAvailable += GlobalInventoryRef->GetItemCount(ItemId);
    }
    
    // 2. 該当チームのメンバーが持つ数量を取得
    if (IsValid(TeamComponentRef))
    {
        FTeam Team = TeamComponentRef->GetTeam(TeamIndex);
        for (AC_IdleCharacter* Member : Team.Members)
        {
            if (IsValid(Member))
            {
                if (UInventoryComponent* MemberInventory = Member->GetInventoryComponent())
                {
                    TotalAvailable += MemberInventory->GetItemCount(ItemId);
                }
            }
        }
    }
    
    return TotalAvailable;
}

FGlobalTask UTaskManagerComponent::FindActiveGatheringTask(const FString& ItemId) const
{
    // 優先度順にソート
    TArray<FGlobalTask> SortedTasks = GlobalTasks;
    SortedTasks.Sort([](const FGlobalTask& A, const FGlobalTask& B) {
        return A.Priority < B.Priority;
    });
    
    // 該当アイテムの未完了タスクを検索
    for (const FGlobalTask& Task : SortedTasks)
    {
        if (Task.TargetItemId == ItemId && 
            !Task.bIsCompleted && 
            (Task.TaskType == ETaskType::Gathering || Task.TaskType == ETaskType::All))
        {
            return Task;
        }
    }
    
    // 見つからない場合は空のタスクを返す
    return FGlobalTask();
}

FString UTaskManagerComponent::GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId) const
{
    UE_LOG(LogTemp, Warning, TEXT("📋🎯 GetTargetItemForTeam: Team %d, Location %s"), TeamIndex, *LocationId);
    
    if (!IsValid(TeamComponentRef))
    {
        UE_LOG(LogTemp, Error, TEXT("GetTargetItemForTeam: TeamComponent unavailable"));
        return FString();
    }
    
    // 1. チームタスクを優先度順で取得
    TArray<FTeamTask> TeamTasks = TeamComponentRef->GetTeamTasks(TeamIndex);
    
    UE_LOG(LogTemp, Warning, TEXT("📋📝 GetTargetItemForTeam: Found %d team tasks"), TeamTasks.Num());
    
    if (TeamTasks.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⚠️ GetTargetItemForTeam: No team tasks, falling back to global tasks"));
        // チームタスクなし → グローバルタスクにフォールバック
        return GetTargetItemFromGlobalTasks(LocationId);
    }
    
    // 2. チームタスクの優先度順でマッチング検索
    for (int32 i = 0; i < TeamTasks.Num(); i++)
    {
        const FTeamTask& TeamTask = TeamTasks[i];
        
        UE_LOG(LogTemp, Warning, TEXT("📋🔍 GetTargetItemForTeam: Checking team task %d - Type: %d, Priority: %d"), 
            i, (int32)TeamTask.TaskType, TeamTask.Priority);
        
        // 3. このチームタスクに対応するグローバルタスクを探す
        FString MatchedTarget = FindMatchingGlobalTask(TeamTask, TeamIndex, LocationId);
        
        UE_LOG(LogTemp, Warning, TEXT("📋📦 GetTargetItemForTeam: FindMatchingGlobalTask returned: '%s'"), *MatchedTarget);
        
        if (!MatchedTarget.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("📋✅ GetTargetItemForTeam: Returning matched target: '%s'"), *MatchedTarget);
            return MatchedTarget; // 最初にマッチしたものを返す
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("📋❌ GetTargetItemForTeam: No matching tasks found, returning empty string"));
    return FString(); // 全てのチームタスクでマッチしない → 拠点帰還
}

FString UTaskManagerComponent::GetTargetItemFromGlobalTasks(const FString& LocationId) const
{
    
    // LocationDataTableManager を取得
    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetTargetItemFromGlobalTasks: GameInstance not found"));
        return FString();
    }
    
    ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>();
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetTargetItemFromGlobalTasks: LocationManager not found"));
        return FString();
    }
    
    // グローバルタスクリストから採集タスクを優先度順で探す
    for (const FGlobalTask& GlobalTask : GlobalTasks)
    {
        // 採集タスクのみチェック
        if (GlobalTask.TaskType != ETaskType::Gathering)
        {
            continue;
        }
        
        // タスクが完了済みならスキップ
        if (GlobalTask.bIsCompleted)
        {
            continue;
        }
        
        // LocationDataからアイテムが採取可能かチェック
        if (!LocationId.IsEmpty())
        {
            FLocationDataRow LocationData;
            if (LocationManager->GetLocationData(LocationId, LocationData))
            {
                // この場所で採取可能なアイテムリストに含まれているかチェック
                TArray<FString> GatherableItems;
                LocationData.GetGatherableItemIds(GatherableItems);
                
                if (GatherableItems.Contains(GlobalTask.TargetItemId))
                {
                    return GlobalTask.TargetItemId;
                }
            }
        }
    }
    
    return FString(); // グローバルタスクでもマッチしない
}

bool UTaskManagerComponent::IsTaskCompleted(const FString& TaskId) const
{
    if (TaskId.IsEmpty())
    {
        return true; // 空のタスクIDは完了とみなす
    }
    
    for (const FGlobalTask& Task : GlobalTasks)
    {
        if (Task.TaskId == TaskId)
        {
            // 完了フラグまたは目標数量達成をチェック
            if (Task.bIsCompleted)
            {
                return true;
            }
            
            // 現在の進捗が目標に達しているかチェック
            if (Task.TargetQuantity > 0 && Task.CurrentProgress >= Task.TargetQuantity)
            {
                UE_LOG(LogTemp, Warning, TEXT("TASK COMPLETED: %s reached target (%d/%d)"), 
                    *TaskId, Task.CurrentProgress, Task.TargetQuantity);
                return true;
            }
            
            // 重要：進捗状況（目標に達していない場合のみ表示）
            UE_LOG(LogTemp, Warning, TEXT("TASK PROGRESS: %s (%d/%d)"), 
                *TaskId, Task.CurrentProgress, Task.TargetQuantity);
            
            return false;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("IsTaskCompleted: Task %s not found"), *TaskId);
    return true; // 見つからないタスクは完了とみなす
}

TArray<FGlobalTask> UTaskManagerComponent::GetExecutableGatheringTasksAtLocation(int32 TeamIndex, const FString& LocationId) const
{
    TArray<FGlobalTask> ExecutableTasks;
    
    // LocationDataTableManager を取得して場所の採集可能アイテムリストを確認
    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance)
    {
        return ExecutableTasks;
    }
    
    ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>();
    if (!LocationManager)
    {
        return ExecutableTasks;
    }
    
    // 場所データを取得
    FLocationDataRow LocationData;
    if (!LocationManager->GetLocationData(LocationId, LocationData))
    {
        return ExecutableTasks;
    }
    
    // その場所の採集可能アイテムリストを取得
    TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
    
    // 優先度順にソートされた全体タスクを取得
    TArray<FGlobalTask> SortedTasks = GlobalTasks;
    SortedTasks.Sort([](const FGlobalTask& A, const FGlobalTask& B) {
        return A.Priority < B.Priority; // 優先度が高い（数値が小さい）順
    });
    
    // 採集タスクの中で、その場所で実行可能なものを抽出
    for (const FGlobalTask& Task : SortedTasks)
    {
        // 未完了の採集タスクのみ対象
        if (Task.bIsCompleted || Task.TaskType != ETaskType::Gathering)
        {
            continue;
        }
        
        // Keepタスクの満足度チェック：拠点全体の在庫量で判定
        if (Task.GatheringQuantityType == EGatheringQuantityType::Keep && Task.TargetQuantity > 0)
        {
            // 拠点全体の現在在庫量を取得（拠点倉庫 + 全チーム所持）
            int32 TotalCurrentAmount = GetTotalResourceAmount(Task.TargetItemId);
            
            if (TotalCurrentAmount >= Task.TargetQuantity)
            {
                continue; // Keep task is satisfied, skip
            }
        }
        
        // その場所で採集可能なアイテムかチェック
        bool bCanGatherAtLocation = false;
        for (const FGatherableItemInfo& ItemInfo : GatherableItems)
        {
            if (ItemInfo.ItemId == Task.TargetItemId)
            {
                bCanGatherAtLocation = true;
                break;
            }
        }
        
        if (!bCanGatherAtLocation)
        {
            continue;
        }
        
        // チームがこのタスクを実行可能かチェック
        if (CanTeamExecuteTask(TeamIndex, Task))
        {
            ExecutableTasks.Add(Task);
        }
    }
    
    return ExecutableTasks;
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

// === タスクマッチング関数群 ===

FString UTaskManagerComponent::FindMatchingGlobalTask(const FTeamTask& TeamTask, int32 TeamIndex, const FString& LocationId) const
{
    switch (TeamTask.TaskType)
    {
        case ETaskType::Gathering:
            return FindMatchingGatheringTask(TeamIndex, LocationId);
            
        case ETaskType::Adventure:
            return FindMatchingAdventureTask(TeamIndex, LocationId);
            
        case ETaskType::Construction:
            return FindMatchingConstructionTask(TeamIndex);
            
        case ETaskType::Cooking:
            return FindMatchingCookingTask(TeamIndex);
            
        case ETaskType::Crafting:
            return FindMatchingCraftingTask(TeamIndex);
            
        default:
            UE_LOG(LogTemp, Warning, TEXT("⚠️ Unhandled team task type: %d"), (int32)TeamTask.TaskType);
            return FString();
    }
}

FString UTaskManagerComponent::FindMatchingGatheringTask(int32 TeamIndex, const FString& LocationId) const
{
    UE_LOG(LogTemp, Warning, TEXT("📋🌱 FindMatchingGatheringTask: Team %d, Location %s"), TeamIndex, *LocationId);
    
    // その場所で採集可能なグローバルタスクを優先度順で取得
    UE_LOG(LogTemp, Warning, TEXT("📋🔍 FindMatchingGatheringTask: Checking global tasks count: %d"), GlobalTasks.Num());
    
    // 現在のグローバルタスクをログ出力
    for (int32 i = 0; i < GlobalTasks.Num(); i++)
    {
        const FGlobalTask& Task = GlobalTasks[i];
        UE_LOG(LogTemp, Warning, TEXT("📋📝 Global Task %d: ID=%s, Type=%d, TargetItem=%s, Completed=%s"), 
            i, *Task.TaskId, (int32)Task.TaskType, *Task.TargetItemId, Task.bIsCompleted ? TEXT("YES") : TEXT("NO"));
    }
    TArray<FGlobalTask> ExecutableTasks = GetExecutableGatheringTasksAtLocation(TeamIndex, LocationId);
    
    if (ExecutableTasks.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("📋❌ FindMatchingGatheringTask: No executable tasks at location %s"), *LocationId);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("📋📊 FindMatchingGatheringTask: Found %d executable tasks at location %s"), 
            ExecutableTasks.Num(), *LocationId);
    }
    
    if (ExecutableTasks.Num() > 0)
    {
        const FGlobalTask& SelectedTask = ExecutableTasks[0]; // 最優先
        UE_LOG(LogTemp, Warning, TEXT("📋✅ FindMatchingGatheringTask: Selected task target item: %s"), 
            *SelectedTask.TargetItemId);
        return SelectedTask.TargetItemId;
    }
    
    // This was already changed above
    return FString();
}

FString UTaskManagerComponent::FindMatchingAdventureTask(int32 TeamIndex, const FString& LocationId) const
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("  🏔️ Searching adventure tasks for location %s"), *LocationId);
    
    // 冒険タスクを優先度順で取得
    TArray<FGlobalTask> AdventureTasks = GetGlobalTasksByPriority();
    
    for (const FGlobalTask& Task : AdventureTasks)
    {
        // 冒険タスクかつ未完了のタスクのみ対象
        if (Task.TaskType == ETaskType::Adventure && !Task.bIsCompleted)
        {
            // TargetItemIdを場所IDとして使用（例: "plains", "forest"など）
            if (Task.TargetItemId == LocationId || Task.TargetItemId.IsEmpty())
            {
                // チームがタスクを実行可能かチェック
                if (CanTeamExecuteTask(TeamIndex, Task))
                {
                    UE_LOG(LogTemp, VeryVerbose, TEXT("  ✅ Selected adventure task: %s at %s"), 
                        *Task.DisplayName, *LocationId);
                    return Task.TaskId;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("  ❌ No adventure tasks available for %s"), *LocationId);
    return FString();
}

FString UTaskManagerComponent::FindMatchingConstructionTask(int32 TeamIndex) const
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("  🏗️ Construction task matching (not implemented)"));
    // TODO: 建築システム実装時に追加
    return FString();
}

FString UTaskManagerComponent::FindMatchingCookingTask(int32 TeamIndex) const
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("  🍳 Cooking task matching (not implemented)"));
    // TODO: 料理システム実装時に追加
    return FString();
}

FString UTaskManagerComponent::FindMatchingCraftingTask(int32 TeamIndex) const
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("  ⚒️ Crafting task matching (not implemented)"));
    // TODO: 製作システム実装時に追加
    return FString();
}

// === タスク実行計画システム実装 ===

FTaskExecutionPlan UTaskManagerComponent::CreateExecutionPlanForTeam(int32 TeamIndex, const FString& CurrentLocation, ETaskType CurrentTask)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("📋 Creating execution plan for Team %d at %s (Current Task: %s)"), 
        TeamIndex, *CurrentLocation, *UTaskTypeUtils::GetTaskTypeDisplayName(CurrentTask));
    
    // タスクタイプに応じて専門メソッドに委譲
    switch (CurrentTask)
    {
        case ETaskType::Gathering:
            return CreateGatheringExecutionPlan(TeamIndex, CurrentLocation);
            
        case ETaskType::Adventure:
            return CreateAdventureExecutionPlan(TeamIndex, CurrentLocation);
            
        case ETaskType::All:
            return CreateAllModeExecutionPlan(TeamIndex, CurrentLocation);
            
        default:
            UE_LOG(LogTemp, Warning, TEXT("⚠️ Unsupported task type for execution plan: %s"), 
                *UTaskTypeUtils::GetTaskTypeDisplayName(CurrentTask));
            
            // 無効な計画を返す
            FTaskExecutionPlan InvalidPlan;
            InvalidPlan.ExecutionAction = ETaskExecutionAction::WaitIdle;
            InvalidPlan.ExecutionReason = TEXT("Unsupported task type");
            InvalidPlan.bIsValid = false;
            return InvalidPlan;
    }
}

FTaskExecutionPlan UTaskManagerComponent::CreateGatheringExecutionPlan(int32 TeamIndex, const FString& CurrentLocation)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("🌾 Creating gathering execution plan for Team %d at %s"), TeamIndex, *CurrentLocation);
    
    FTaskExecutionPlan Plan;
    Plan.ExecutionAction = ETaskExecutionAction::None;
    Plan.bIsValid = false;
    
    // 1. 拠点にいる場合の処理
    if (CurrentLocation == TEXT("base"))
    {
        // 自動荷下ろしが必要
        Plan.ExecutionAction = ETaskExecutionAction::UnloadItems;
        Plan.ExecutionReason = TEXT("Auto-unload at base before next gathering task");
        Plan.bIsValid = true;
        
        // 次の採集先を探す
        TArray<FString> LocationsToCheck = {TEXT("plains"), TEXT("forest"), TEXT("swamp"), TEXT("mountain")};
        for (const FString& LocationId : LocationsToCheck)
        {
            TArray<FGlobalTask> GatheringTasks = GetExecutableGatheringTasksAtLocation(TeamIndex, LocationId);
            if (GatheringTasks.Num() > 0)
            {
                // 最優先の採集タスクを選択
                const FGlobalTask& SelectedTask = GatheringTasks[0];
                Plan.ExecutionAction = ETaskExecutionAction::MoveToLocation;
                Plan.TaskId = SelectedTask.TaskId;
                Plan.TargetLocation = LocationId;
                Plan.TargetItem = SelectedTask.TargetItemId;
                Plan.ExecutionReason = FString::Printf(TEXT("Moving to %s for %s gathering"), 
                    *LocationId, *SelectedTask.TargetItemId);
                Plan.bIsValid = true;
                
                UE_LOG(LogTemp, Log, TEXT("📋 Gathering Plan: Move to %s for %s"), 
                    *LocationId, *SelectedTask.TargetItemId);
                return Plan;
            }
        }
        
        // 利用可能なタスクがない場合
        Plan.ExecutionAction = ETaskExecutionAction::WaitIdle;
        Plan.ExecutionReason = TEXT("No gathering tasks available");
        Plan.bIsValid = true;
        return Plan;
    }
    
    // 2. 拠点以外にいる場合の処理
    FString TargetItemId = GetTargetItemForTeam(TeamIndex, CurrentLocation);
    
    // 2.1 タスク完了チェック
    if (!TargetItemId.IsEmpty())
    {
        FGlobalTask ActiveTask = FindActiveGatheringTask(TargetItemId);
        if (!ActiveTask.TaskId.IsEmpty() && IsTaskCompleted(ActiveTask.TaskId))
        {
            Plan.ExecutionAction = ETaskExecutionAction::ReturnToBase;
            Plan.ExecutionReason = FString::Printf(TEXT("Task %s completed, returning to base"), *ActiveTask.TaskId);
            Plan.bIsValid = true;
            
            UE_LOG(LogTemp, Log, TEXT("📋 Gathering Plan: Task completed, returning to base"));
            return Plan;
        }
    }
    
    // 2.2 アイテム満タン判定（TODO: チーム容量チェック実装）
    // 現在は簡易実装
    
    // 2.3 タスク存在判定
    if (TargetItemId.IsEmpty())
    {
        Plan.ExecutionAction = ETaskExecutionAction::ReturnToBase;
        Plan.ExecutionReason = TEXT("No target item for current location");
        Plan.bIsValid = true;
        
        UE_LOG(LogTemp, Log, TEXT("📋 Gathering Plan: No target item, returning to base"));
        return Plan;
    }
    
    // 2.4 採集実行
    Plan.ExecutionAction = ETaskExecutionAction::ExecuteGathering;
    Plan.TargetItem = TargetItemId;
    Plan.TargetLocation = CurrentLocation;
    Plan.ExecutionReason = FString::Printf(TEXT("Execute gathering for %s at %s"), 
        *TargetItemId, *CurrentLocation);
    Plan.bIsValid = true;
    
    UE_LOG(LogTemp, Log, TEXT("📋 Gathering Plan: Execute gathering for %s"), *TargetItemId);
    return Plan;
}

FTaskExecutionPlan UTaskManagerComponent::CreateAdventureExecutionPlan(int32 TeamIndex, const FString& CurrentLocation)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("⚔️ Creating adventure execution plan for Team %d at %s"), TeamIndex, *CurrentLocation);
    
    FTaskExecutionPlan Plan;
    Plan.ExecutionAction = ETaskExecutionAction::None;
    Plan.bIsValid = false;
    
    // 1. 拠点にいる場合 - 冒険先を決定
    if (CurrentLocation == TEXT("base"))
    {
        // 利用可能な冒険タスクから目的地を決定（既存のロジックを統合）
        TArray<FGlobalTask> AdventureTasks = GetGlobalTasksByPriority();
        FString TargetLocation;
        FString SelectedTaskId;
        
        for (const FGlobalTask& Task : AdventureTasks)
        {
            if (Task.TaskType == ETaskType::Adventure && !Task.bIsCompleted)
            {
                // TargetItemIdを場所として使用
                if (!Task.TargetItemId.IsEmpty())
                {
                    TargetLocation = Task.TargetItemId;
                    SelectedTaskId = Task.TaskId;
                    break;
                }
                else
                {
                    // 場所が指定されていない場合はランダムに選択
                    TArray<FString> PossibleLocations = {TEXT("plains"), TEXT("forest"), TEXT("swamp"), TEXT("cave")};
                    TargetLocation = PossibleLocations[FMath::RandRange(0, PossibleLocations.Num() - 1)];
                    SelectedTaskId = Task.TaskId;
                    break;
                }
            }
        }
        
        if (TargetLocation.IsEmpty())
        {
            Plan.ExecutionAction = ETaskExecutionAction::WaitIdle;
            Plan.ExecutionReason = TEXT("No adventure tasks available");
            Plan.bIsValid = true;
            
            UE_LOG(LogTemp, Log, TEXT("📋 Adventure Plan: No adventure tasks available"));
            return Plan;
        }
        
        // 冒険先への移動
        Plan.ExecutionAction = ETaskExecutionAction::MoveToLocation;
        Plan.TaskId = SelectedTaskId;
        Plan.TargetLocation = TargetLocation;
        Plan.ExecutionReason = FString::Printf(TEXT("Moving to %s for adventure"), *TargetLocation);
        Plan.bIsValid = true;
        
        UE_LOG(LogTemp, Log, TEXT("📋 Adventure Plan: Move to %s for adventure"), *TargetLocation);
        return Plan;
    }
    
    // 2. 目的地に到着している場合 - 戦闘実行
    // マッチする冒険タスクがあるかチェック
    FString MatchingTaskId = FindMatchingAdventureTask(TeamIndex, CurrentLocation);
    
    if (!MatchingTaskId.IsEmpty())
    {
        Plan.ExecutionAction = ETaskExecutionAction::ExecuteCombat;
        Plan.TaskId = MatchingTaskId;
        Plan.TargetLocation = CurrentLocation;
        Plan.ExecutionReason = FString::Printf(TEXT("Execute combat at %s"), *CurrentLocation);
        Plan.bIsValid = true;
        
        UE_LOG(LogTemp, Log, TEXT("📋 Adventure Plan: Execute combat at %s"), *CurrentLocation);
        return Plan;
    }
    
    // 3. 該当する冒険タスクがない場合 - 拠点に帰還
    Plan.ExecutionAction = ETaskExecutionAction::ReturnToBase;
    Plan.ExecutionReason = FString::Printf(TEXT("No matching adventure task at %s, returning to base"), *CurrentLocation);
    Plan.bIsValid = true;
    
    UE_LOG(LogTemp, Log, TEXT("📋 Adventure Plan: No matching task at %s, returning to base"), *CurrentLocation);
    return Plan;
}

FTaskExecutionPlan UTaskManagerComponent::CreateAllModeExecutionPlan(int32 TeamIndex, const FString& CurrentLocation)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("🔄 Creating all-mode execution plan for Team %d at %s"), TeamIndex, *CurrentLocation);
    
    FTaskExecutionPlan Plan;
    Plan.ExecutionAction = ETaskExecutionAction::None;
    Plan.bIsValid = false;
    
    // 「全て」モードでは最優先のタスクを自動選択
    FGlobalTask NextTask = GetNextAvailableTask(TeamIndex);
    
    if (NextTask.TaskId.IsEmpty())
    {
        // 利用可能なタスクがない場合
        Plan.ExecutionAction = ETaskExecutionAction::WaitIdle;
        Plan.ExecutionReason = TEXT("No available tasks in all-mode");
        Plan.bIsValid = true;
        
        UE_LOG(LogTemp, Log, TEXT("📋 All-Mode Plan: No available tasks"));
        return Plan;
    }
    
    // タスクタイプに応じて適切な実行計画に委譲
    switch (NextTask.TaskType)
    {
        case ETaskType::Gathering:
            {
                FTaskExecutionPlan GatheringPlan = CreateGatheringExecutionPlan(TeamIndex, CurrentLocation);
                GatheringPlan.ExecutionReason = FString::Printf(TEXT("All-mode selected gathering: %s"), 
                    *GatheringPlan.ExecutionReason);
                UE_LOG(LogTemp, Log, TEXT("📋 All-Mode Plan: Delegated to gathering"));
                return GatheringPlan;
            }
            
        case ETaskType::Adventure:
            {
                FTaskExecutionPlan AdventurePlan = CreateAdventureExecutionPlan(TeamIndex, CurrentLocation);
                AdventurePlan.ExecutionReason = FString::Printf(TEXT("All-mode selected adventure: %s"), 
                    *AdventurePlan.ExecutionReason);
                UE_LOG(LogTemp, Log, TEXT("📋 All-Mode Plan: Delegated to adventure"));
                return AdventurePlan;
            }
            
        default:
            // サポートされていないタスクタイプ
            Plan.ExecutionAction = ETaskExecutionAction::WaitIdle;
            Plan.ExecutionReason = FString::Printf(TEXT("All-mode: Unsupported task type %s"), 
                *UTaskTypeUtils::GetTaskTypeDisplayName(NextTask.TaskType));
            Plan.bIsValid = true;
            
            UE_LOG(LogTemp, Warning, TEXT("📋 All-Mode Plan: Unsupported task type: %s"), 
                *UTaskTypeUtils::GetTaskTypeDisplayName(NextTask.TaskType));
            return Plan;
    }
}

// === 採集実行機能（GatheringService/GatheringComponentからの移行） ===

TArray<FString> UTaskManagerComponent::GetGatherableItemsAt(const FString& LocationId) const
{
    TArray<FString> GatherableItems;
    
    if (LocationId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⛏️ GetGatherableItemsAt: Empty location ID"));
        return GatherableItems;
    }
    
    // LocationDataTableManagerから採集可能アイテムを取得
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("📋⛏️ GetGatherableItemsAt: GameInstance not found"));
        return GatherableItems;
    }
    
    ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>();
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Error, TEXT("📋⛏️ GetGatherableItemsAt: LocationDataTableManager not found"));
        return GatherableItems;
    }
    
    FLocationDataRow LocationData;
    if (LocationManager->GetLocationData(LocationId, LocationData))
    {
        LocationData.GetGatherableItemIds(GatherableItems);
        UE_LOG(LogTemp, VeryVerbose, TEXT("📋⛏️ GetGatherableItemsAt: Found %d gatherable items at %s"), 
            GatherableItems.Num(), *LocationId);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⛏️ GetGatherableItemsAt: Location %s not found"), *LocationId);
    }
    
    return GatherableItems;
}

TArray<FString> UTaskManagerComponent::FindLocationsForItem(const FString& ItemId) const
{
    TArray<FString> Locations;
    
    if (ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("📋🔍 FindLocationsForItem: Empty item ID"));
        return Locations;
    }
    
    // 全場所をチェックして該当アイテムが採集可能な場所を探す
    TArray<FString> AllLocations = {TEXT("base"), TEXT("plains"), TEXT("forest"), TEXT("swamp"), TEXT("mountain"), TEXT("cave")};
    
    for (const FString& Location : AllLocations)
    {
        TArray<FString> GatherableItems = GetGatherableItemsAt(Location);
        if (GatherableItems.Contains(ItemId))
        {
            Locations.Add(Location);
            UE_LOG(LogTemp, VeryVerbose, TEXT("📋🔍 FindLocationsForItem: Item %s found at %s"), 
                *ItemId, *Location);
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("📋🔍 FindLocationsForItem: Item %s can be gathered at %d locations"), 
        *ItemId, Locations.Num());
    
    return Locations;
}

int32 UTaskManagerComponent::CalculateGatheringAmount(int32 TeamIndex, const FString& ItemId, const FString& LocationId) const
{
    if (!IsValid(TeamComponentRef))
    {
        UE_LOG(LogTemp, Error, TEXT("📋📊 CalculateGatheringAmount: TeamComponent reference not set"));
        return 0;
    }
    
    if (ItemId.IsEmpty() || LocationId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("📋📊 CalculateGatheringAmount: Empty ItemId or LocationId"));
        return 0;
    }
    
    // チーム情報を取得
    FTeam Team = TeamComponentRef->GetTeam(TeamIndex);
    if (!Team.IsValidTeam() || Team.Members.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("📋📊 CalculateGatheringAmount: Invalid team %d"), TeamIndex);
        return 0;
    }
    
    // チーム全体の採集力を計算
    float TotalGatheringPower = 0.0f;
    int32 ValidMembers = 0;
    
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (IsValid(Member))
        {
            // CharacterStatusComponentから採集能力を取得
            if (UCharacterStatusComponent* StatusComp = Member->GetStatusComponent())
            {
                FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
                TotalGatheringPower += DerivedStats.GatheringPower;
                ValidMembers++;
            }
        }
    }
    
    if (ValidMembers == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("📋📊 CalculateGatheringAmount: No valid team members"));
        return 0;
    }
    
    // 基本採集量計算（1ターンあたり）
    // 採集効率係数を適用（GatheringComponentと同じ値を使用）
    float GatheringEfficiencyMultiplier = 40.0f;
    int32 BaseAmount = FMath::FloorToInt(TotalGatheringPower / GatheringEfficiencyMultiplier);
    
    // 最低1個は採集できるようにする
    int32 FinalAmount = FMath::Max(BaseAmount, 1);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("📋📊 CalculateGatheringAmount: Team %d, Item %s, Amount %d (Power: %.1f, Members: %d)"), 
        TeamIndex, *ItemId, FinalAmount, TotalGatheringPower, ValidMembers);
    
    return FinalAmount;
}

bool UTaskManagerComponent::ExecuteGathering(int32 TeamIndex, const FString& ItemId, const FString& LocationId)
{
    if (!IsValid(TeamComponentRef))
    {
        UE_LOG(LogTemp, Error, TEXT("📋⚡ ExecuteGathering: TeamComponent reference not set"));
        return false;
    }
    
    if (ItemId.IsEmpty() || LocationId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⚡ ExecuteGathering: Empty ItemId or LocationId"));
        return false;
    }
    
    // チーム情報を取得
    FTeam Team = TeamComponentRef->GetTeam(TeamIndex);
    if (!Team.IsValidTeam() || Team.Members.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⚡ ExecuteGathering: Invalid team %d"), TeamIndex);
        return false;
    }
    
    // 場所で該当アイテムが採集可能かチェック
    TArray<FString> GatherableItems = GetGatherableItemsAt(LocationId);
    if (!GatherableItems.Contains(ItemId))
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⚡ ExecuteGathering: Item %s not gatherable at %s"), 
            *ItemId, *LocationId);
        return false;
    }
    
    // 採集量を計算
    int32 GatheringAmount = CalculateGatheringAmount(TeamIndex, ItemId, LocationId);
    if (GatheringAmount <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("📋⚡ ExecuteGathering: No gathering amount calculated"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("📋⚡🌱 ExecuteGathering: Team %d gathering %d x %s at %s"), 
        TeamIndex, GatheringAmount, *ItemId, *LocationId);
    
    // 各チームメンバーのインベントリに均等配分
    int32 AmountPerMember = FMath::Max(1, GatheringAmount / Team.Members.Num());
    int32 RemainingAmount = GatheringAmount;
    
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (IsValid(Member) && RemainingAmount > 0)
        {
            if (UInventoryComponent* MemberInventory = Member->GetInventoryComponent())
            {
                int32 AmountToAdd = FMath::Min(AmountPerMember, RemainingAmount);
                
                if (MemberInventory->AddItem(ItemId, AmountToAdd))
                {
                    RemainingAmount -= AmountToAdd;
                    
                    UE_LOG(LogTemp, Warning, TEXT("📋⚡✅ ExecuteGathering: Added %d %s to %s"), 
                        AmountToAdd, *ItemId, *Member->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("📋⚡ ExecuteGathering: Failed to add %s to %s inventory"), 
                        *ItemId, *Member->GetName());
                }
            }
        }
    }
    
    // タスク進捗を更新
    int32 ActualGathered = GatheringAmount - RemainingAmount;
    if (ActualGathered > 0)
    {
        // アクティブな採集タスクを見つけて進捗更新
        FGlobalTask ActiveTask = FindActiveGatheringTask(ItemId);
        if (!ActiveTask.TaskId.IsEmpty())
        {
            UpdateTaskProgress(ActiveTask.TaskId, ActualGathered);
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("📋⚡ ExecuteGathering: Successfully gathered %d %s at %s"), 
            ActualGathered, *ItemId, *LocationId);
        
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("📋⚡ ExecuteGathering: No items were actually gathered"));
    return false;
}