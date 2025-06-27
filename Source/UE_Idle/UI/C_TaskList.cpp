#include "C_TaskList.h"
#include "../Components/TaskManagerComponent.h"
#include "C_GlobalTaskCard.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBoxSlot.h"

void UC_TaskList::NativeConstruct()
{
    Super::NativeConstruct();

}

void UC_TaskList::NativeDestruct()
{
    // イベントのアンバインド
    UnbindTaskManagerEvents();

    Super::NativeDestruct();
}

void UC_TaskList::InitializeWithTaskManager(UTaskManagerComponent* InTaskManager)
{
    if (!InTaskManager)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TaskList::InitializeWithTaskManager - TaskManager is null"));
        return;
    }

    // 既存のタスクマネージャーがある場合はアンバインド
    if (TaskManager)
    {
        UnbindTaskManagerEvents();
    }

    TaskManager = InTaskManager;
    BindTaskManagerEvents();

    // 初期タスクリストを表示
    RefreshTaskList();

    UE_LOG(LogTemp, Log, TEXT("UC_TaskList::InitializeWithTaskManager - Initialized with TaskManager"));
}

void UC_TaskList::RefreshTaskList()
{
    if (!TaskManager || !TaskScrollBox)
    {
        return;
    }

    // 既存のカードをクリア
    TaskScrollBox->ClearChildren();
    TaskCards.Empty();

    // タスクを優先度順に取得
    TArray<FGlobalTask> SortedTasks = TaskManager->GetGlobalTasksByPriority();

    // 各タスクのカードを作成
    for (int32 i = 0; i < SortedTasks.Num(); i++)
    {
        // 元のインデックスを探す
        int32 OriginalIndex = TaskManager->FindTaskByID(SortedTasks[i].TaskId);
        if (OriginalIndex >= 0)
        {
            UC_GlobalTaskCard* NewCard = CreateTaskCard(SortedTasks[i], OriginalIndex);
            if (NewCard)
            {
                TaskCards.Add(NewCard);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UC_TaskList::RefreshTaskList - Created %d task cards"), TaskCards.Num());
}

UC_GlobalTaskCard* UC_TaskList::CreateTaskCard(const FGlobalTask& Task, int32 TaskIndex)
{
    if (!GlobalTaskCardClass || !TaskScrollBox)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TaskList::CreateTaskCard - GlobalTaskCardClass or TaskScrollBox is null"));
        return nullptr;
    }

    // タスクカードを作成
    UC_GlobalTaskCard* NewCard = CreateWidget<UC_GlobalTaskCard>(this, GlobalTaskCardClass);
    if (NewCard)
    {
        NewCard->InitializeWithTask(Task, TaskIndex, TaskManager);
        TaskScrollBox->AddChild(NewCard);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TaskList::CreateTaskCard - Created card for task %s"), *Task.TaskId);
    }

    return NewCard;
}


void UC_TaskList::OnGlobalTaskAdded(const FGlobalTask& NewTask)
{
    // タスクリストを再構築
    RefreshTaskList();
}

void UC_TaskList::OnGlobalTaskRemoved(int32 TaskIndex)
{
    // タスクリストを再構築
    RefreshTaskList();
}

void UC_TaskList::OnTaskPriorityChanged(int32 TaskIndex, int32 NewPriority)
{
    // 優先度が変更されたらリストを再ソート
    RefreshTaskList();
}

void UC_TaskList::SortTaskCardsByPriority()
{
    // RefreshTaskListで優先度順に再構築するため、この関数は現在未使用
}

void UC_TaskList::BindTaskManagerEvents()
{
    if (!TaskManager)
    {
        return;
    }

    TaskManager->OnGlobalTaskAdded.AddDynamic(this, &UC_TaskList::OnGlobalTaskAdded);
    TaskManager->OnGlobalTaskRemoved.AddDynamic(this, &UC_TaskList::OnGlobalTaskRemoved);
    TaskManager->OnTaskPriorityChanged.AddDynamic(this, &UC_TaskList::OnTaskPriorityChanged);
}

void UC_TaskList::UnbindTaskManagerEvents()
{
    if (!TaskManager)
    {
        return;
    }

    TaskManager->OnGlobalTaskAdded.RemoveDynamic(this, &UC_TaskList::OnGlobalTaskAdded);
    TaskManager->OnGlobalTaskRemoved.RemoveDynamic(this, &UC_TaskList::OnGlobalTaskRemoved);
    TaskManager->OnTaskPriorityChanged.RemoveDynamic(this, &UC_TaskList::OnTaskPriorityChanged);
}
