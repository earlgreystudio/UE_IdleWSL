#include "C_TeamTaskMakeSheet.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "../Components/TeamComponent.h"
#include "../Types/TaskTypes.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"

void UC_TeamTaskMakeSheet::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamTaskMakeSheet::NativeConstruct - TASK MAKE SHEET CONSTRUCTED ====="));
    
    // クラス情報をログ出力
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskMakeSheet class: %s"), *GetClass()->GetName());
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskMakeSheet parent class: %s"), GetClass()->GetSuperClass() ? *GetClass()->GetSuperClass()->GetName() : TEXT("NULL"));
    
    // 全ての子ウィジェットをリスト
    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);
    UE_LOG(LogTemp, Warning, TEXT("Total child widgets: %d"), AllWidgets.Num());
    for (UWidget* Widget : AllWidgets)
    {
        if (Widget)
        {
            UE_LOG(LogTemp, Warning, TEXT("  - Widget: %s (Type: %s)"), *Widget->GetName(), *Widget->GetClass()->GetName());
        }
    }
    
    // ウィジェットバインディングの確認
    if (CreateTaskButton)
    {
        CreateTaskButton->OnClicked.AddDynamic(this, &UC_TeamTaskMakeSheet::OnCreateTaskClicked);
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskMakeSheet::NativeConstruct - CreateTaskButton found and bound"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskMakeSheet::NativeConstruct - CreateTaskButton is NULL! Check Blueprint binding"));
    }
    
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UC_TeamTaskMakeSheet::OnCancelClicked);
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskMakeSheet::NativeConstruct - CancelButton found and bound"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskMakeSheet::NativeConstruct - CancelButton is NULL! Check Blueprint binding"));
    }

    if (TaskTypeComboBox)
    {
        TaskTypeComboBox->OnSelectionChanged.AddDynamic(this, &UC_TeamTaskMakeSheet::OnTaskTypeComboBoxChanged);
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskMakeSheet::NativeConstruct - TaskTypeComboBox found and bound"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskMakeSheet::NativeConstruct - TaskTypeComboBox is NULL! Check Blueprint binding"));
    }

    // コンボボックスの初期化
    InitializeTaskTypeComboBox();
    InitializeDefaultValues();
    
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamTaskMakeSheet::NativeConstruct - CONSTRUCTION COMPLETED ====="));
}

void UC_TeamTaskMakeSheet::NativeDestruct()
{
    // イベントのアンバインド
    if (CreateTaskButton)
    {
        CreateTaskButton->OnClicked.RemoveAll(this);
    }
    
    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveAll(this);
    }

    if (TaskTypeComboBox)
    {
        TaskTypeComboBox->OnSelectionChanged.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UC_TeamTaskMakeSheet::InitializeWithTeam(int32 InTeamIndex, UTeamComponent* InTeamComponent)
{
    TeamIndex = InTeamIndex;
    TeamComponent = InTeamComponent;

    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamTaskMakeSheet: TeamComponent is null"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UC_TeamTaskMakeSheet: Initialized with Team %d"), TeamIndex);
}

void UC_TeamTaskMakeSheet::OnTaskTypeSelectionChanged()
{
    if (!TaskTypeComboBox)
    {
        return;
    }

    FString SelectedTaskType = TaskTypeComboBox->GetSelectedOption();
    ETaskType TaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskType);
    
    // タスクタイプに応じてUI表示を更新
    UpdateUIVisibilityForTaskType(TaskType);
    
    // 入力状態チェック
    UpdateCreateButtonState();
}

void UC_TeamTaskMakeSheet::OnCreateTaskClicked()
{
    FString ErrorMessage;
    if (!ValidateInput(ErrorMessage))
    {
        ShowValidationError(ErrorMessage);
        return;
    }

    FTeamTask NewTask = CreateTeamTaskFromInput();
    
    if (TeamComponent)
    {
        // TeamComponentにタスクを追加
        bool bSuccess = TeamComponent->AddTeamTask(TeamIndex, NewTask);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("UC_TeamTaskMakeSheet: Successfully created team task for Team %d"), TeamIndex);
            OnTeamTaskCreatedSuccessfully(NewTask);
            CloseTaskMakeSheet();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamTaskMakeSheet: Failed to create team task"));
            ShowValidationError(TEXT("タスクの作成に失敗しました（タスクスロットが満杯の可能性があります）"));
        }
    }
}

void UC_TeamTaskMakeSheet::OnCancelClicked()
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamTaskMakeSheet: Task creation cancelled"));
    OnTeamTaskCreationCancelled();
    CloseTaskMakeSheet();
}

void UC_TeamTaskMakeSheet::OnTaskTypeComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    OnTaskTypeSelectionChanged();
}

void UC_TeamTaskMakeSheet::InitializeTaskTypeComboBox()
{
    if (!TaskTypeComboBox)
    {
        return;
    }

    TaskTypeComboBox->ClearOptions();

    // チーム用に利用可能なタスクタイプを追加
    TArray<ETaskType> AvailableTaskTypes = {
        ETaskType::Gathering,    // 採集
        ETaskType::Cooking,      // 料理
        ETaskType::Crafting,     // 製作
        ETaskType::Construction, // 建築
        ETaskType::Adventure     // 冒険
    };

    for (ETaskType TaskType : AvailableTaskTypes)
    {
        FString TaskTypeName = UTaskTypeUtils::GetTaskTypeDisplayName(TaskType);
        TaskTypeComboBox->AddOption(TaskTypeName);
    }

    // デフォルト選択
    if (TaskTypeComboBox->GetOptionCount() > 0)
    {
        TaskTypeComboBox->SetSelectedIndex(0);
    }
}

void UC_TeamTaskMakeSheet::InitializeDefaultValues()
{
    // 初期状態では場所選択を非表示
    if (LocationComboBox)
    {
        LocationComboBox->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 作成ボタンの状態を更新
    UpdateCreateButtonState();
}

void UC_TeamTaskMakeSheet::UpdateLocationComboBox()
{
    if (!LocationComboBox)
    {
        return;
    }

    LocationComboBox->ClearOptions();

    // 場所リストの手動追加（仮の実装）
    TArray<FString> LocationNames = {
        TEXT("森"),
        TEXT("山"),
        TEXT("川"),
        TEXT("洞窟"),
        TEXT("平原")
    };
    
    for (const FString& LocationName : LocationNames)
    {
        LocationComboBox->AddOption(LocationName);
    }

    // デフォルト選択
    if (LocationComboBox->GetOptionCount() > 0)
    {
        LocationComboBox->SetSelectedIndex(0);
    }
}

void UC_TeamTaskMakeSheet::UpdateUIVisibilityForTaskType(ETaskType TaskType)
{
    if (!LocationComboBox)
    {
        return;
    }

    if (TaskType == ETaskType::Adventure)
    {
        // 冒険の場合は場所選択を表示
        LocationComboBox->SetVisibility(ESlateVisibility::Visible);
        UpdateLocationComboBox();
    }
    else
    {
        // その他のタスクでは場所選択を非表示
        LocationComboBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UC_TeamTaskMakeSheet::CloseTaskMakeSheet()
{
    // 入力フィールドをクリア
    ClearInputFields();
    
    // Blueprint実装のクローズイベントを呼び出し
    OnTeamTaskMakeSheetClosed();
    
    // ウィジェットを非表示または削除
    SetVisibility(ESlateVisibility::Collapsed);
}

bool UC_TeamTaskMakeSheet::ValidateInput(FString& OutErrorMessage) const
{
    if (!TaskTypeComboBox)
    {
        OutErrorMessage = TEXT("タスクタイプが選択されていません");
        return false;
    }

    FString SelectedTaskType = TaskTypeComboBox->GetSelectedOption();
    if (SelectedTaskType.IsEmpty())
    {
        OutErrorMessage = TEXT("タスクタイプを選択してください");
        return false;
    }

    ETaskType TaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskType);
    
    // 冒険タスクの場合は場所選択が必要
    if (TaskType == ETaskType::Adventure)
    {
        if (!LocationComboBox || LocationComboBox->GetSelectedOption().IsEmpty())
        {
            OutErrorMessage = TEXT("冒険の場合は目的地を選択してください");
            return false;
        }
    }

    if (!TeamComponent)
    {
        OutErrorMessage = TEXT("チーム情報が見つかりません");
        return false;
    }

    return true;
}

FTeamTask UC_TeamTaskMakeSheet::CreateTeamTaskFromInput() const
{
    FTeamTask NewTask;

    if (TaskTypeComboBox)
    {
        FString SelectedTaskType = TaskTypeComboBox->GetSelectedOption();
        NewTask.TaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskType);
    }

    // 既存のタスクを取得して、使用されていない優先度を見つける
    int32 NewPriority = 1;
    if (TeamComponent && TeamComponent->IsValidTeamIndex(TeamIndex))
    {
        TArray<FTeamTask> ExistingTasks = TeamComponent->GetTeamTasks(TeamIndex);
        
        // 使用されている優先度を収集
        TArray<int32> UsedPriorities;
        for (const FTeamTask& Task : ExistingTasks)
        {
            UsedPriorities.Add(Task.Priority);
        }
        
        // 1から順番に未使用の優先度を探す
        while (UsedPriorities.Contains(NewPriority))
        {
            NewPriority++;
        }
    }

    // チーム用タスクの基本設定
    NewTask.Priority = NewPriority; // 自動割り当てされた優先度
    NewTask.MinTeamSize = 1; // 最小1人
    NewTask.EstimatedCompletionTime = 1.0f; // デフォルト1時間
    NewTask.bIsActive = false;
    NewTask.StartTime = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("UC_TeamTaskMakeSheet: Created team task - Type: %s, Priority: %d"), 
        *UTaskTypeUtils::GetTaskTypeDisplayName(NewTask.TaskType), NewTask.Priority);

    return NewTask;
}

void UC_TeamTaskMakeSheet::UpdateCreateButtonState()
{
    if (!CreateTaskButton)
    {
        return;
    }

    FString ErrorMessage;
    bool bCanCreate = ValidateInput(ErrorMessage);
    CreateTaskButton->SetIsEnabled(bCanCreate);
}

void UC_TeamTaskMakeSheet::ClearInputFields()
{
    if (TaskTypeComboBox && TaskTypeComboBox->GetOptionCount() > 0)
    {
        TaskTypeComboBox->SetSelectedIndex(0);
    }

    if (LocationComboBox)
    {
        LocationComboBox->ClearSelection();
        LocationComboBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UC_TeamTaskMakeSheet::OnInputChanged()
{
    UpdateCreateButtonState();
}