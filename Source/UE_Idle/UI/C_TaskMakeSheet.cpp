#include "C_TaskMakeSheet.h"
#include "C_GlobalTaskCard.h"
#include "../Components/TaskManagerComponent.h"
#include "../Components/CraftingComponent.h"
#include "../C_PlayerController.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Engine/Engine.h"

void UC_TaskMakeSheet::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("=== UC_TaskMakeSheet: NativeConstruct START ==="));
    UE_LOG(LogTemp, Warning, TEXT("TaskTypeComboBox: %s"), TaskTypeComboBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("TargetItemComboBox: %s"), TargetItemComboBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("LocationComboBox: %s"), LocationComboBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("TargetQuantitySpinBox: %s"), TargetQuantitySpinBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("KeepQuantityCheckBox: %s"), KeepQuantityCheckBox ? TEXT("OK") : TEXT("NULL"));

    // PlayerControllerから必要なコンポーネントを自動取得
    AutoInitializeFromPlayerController();

    InitializeTaskTypeComboBox();
    InitializeDefaultValues();
    BindEvents();
    
    UE_LOG(LogTemp, Warning, TEXT("=== UC_TaskMakeSheet: NativeConstruct COMPLETED ==="));
}

void UC_TaskMakeSheet::NativeDestruct()
{
    UnbindEvents();
    Super::NativeDestruct();
}

void UC_TaskMakeSheet::SetTaskManagerComponent(UTaskManagerComponent* InTaskManager)
{
    if (TaskManagerComponent != InTaskManager)
    {
        // 既存のイベントバインディングを解除
        UnbindEvents();
        
        TaskManagerComponent = InTaskManager;
        
        // 新しいTaskManagerComponentにイベントをバインド
        BindEvents();
        
        // タスクリストを更新
        RefreshTaskList();
        
        UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: TaskManagerComponent set"));
    }
}

void UC_TaskMakeSheet::SetCraftingComponent(UCraftingComponent* InCraftingComponent)
{
    UE_LOG(LogTemp, Warning, TEXT("=== SetCraftingComponent CALLED ==="));
    UE_LOG(LogTemp, Warning, TEXT("InCraftingComponent: %s"), InCraftingComponent ? TEXT("OK") : TEXT("NULL"));
    
    CraftingComponent = InCraftingComponent;
    
    if (CraftingComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("CraftingComponent successfully set"));
        
        // アイテムリストを更新
        UE_LOG(LogTemp, Warning, TEXT("Calling UpdateTargetItemComboBox from SetCraftingComponent..."));
        UpdateTargetItemComboBox();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CraftingComponent is NULL!"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== SetCraftingComponent COMPLETED ==="));
}

void UC_TaskMakeSheet::AutoInitializeFromPlayerController()
{
    UE_LOG(LogTemp, Warning, TEXT("=== AutoInitializeFromPlayerController START ==="));
    
    // PlayerControllerを取得
    if (APlayerController* PC = GetOwningPlayer())
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerController found: %s"), PC ? TEXT("OK") : TEXT("NULL"));
        
        // カスタムPlayerControllerにキャスト
        if (AC_PlayerController* CustomPC = Cast<AC_PlayerController>(PC))
        {
            UE_LOG(LogTemp, Warning, TEXT("Cast to AC_PlayerController: SUCCESS"));
            
            // TaskManagerComponentを取得・設定
            if (CustomPC->TaskManager)
            {
                SetTaskManagerComponent(CustomPC->TaskManager);
                UE_LOG(LogTemp, Warning, TEXT("TaskManagerComponent set from PlayerController"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("TaskManager is NULL in PlayerController"));
            }
            
            // CraftingComponentを取得・設定
            if (CustomPC->CraftingComponent)
            {
                SetCraftingComponent(CustomPC->CraftingComponent);
                UE_LOG(LogTemp, Warning, TEXT("CraftingComponent set from PlayerController"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CraftingComponent is NULL in PlayerController"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to cast to AC_PlayerController"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get OwningPlayer"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== AutoInitializeFromPlayerController COMPLETED ==="));
}

void UC_TaskMakeSheet::RefreshTaskList()
{
    if (!TaskScrollBox || !TaskManagerComponent)
    {
        return;
    }

    // 既存のタスクカードをクリア
    TaskScrollBox->ClearChildren();

    // 全てのグローバルタスクを取得
    TArray<FGlobalTask> GlobalTasks = TaskManagerComponent->GetGlobalTasks();

    // 優先度順にソート
    GlobalTasks.Sort([](const FGlobalTask& A, const FGlobalTask& B) {
        return A.Priority < B.Priority;
    });

    // タスクカードを作成して追加
    for (int32 i = 0; i < GlobalTasks.Num(); i++)
    {
        if (UC_GlobalTaskCard* TaskCard = CreateTaskCard(GlobalTasks[i], i))
        {
            TaskScrollBox->AddChild(TaskCard);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: Refreshed task list with %d tasks"), GlobalTasks.Num());
}

void UC_TaskMakeSheet::OnTaskTypeSelectionChanged()
{
    UE_LOG(LogTemp, Warning, TEXT("=== OnTaskTypeSelectionChanged CALLED ==="));

    if (!TaskTypeComboBox)
    {
        UE_LOG(LogTemp, Error, TEXT("TaskTypeComboBox is NULL!"));
        return;
    }

    // 現在選択されているタスクタイプを取得
    FString SelectedTaskTypeName = TaskTypeComboBox->GetSelectedOption();
    UE_LOG(LogTemp, Warning, TEXT("Selected Task Type: %s"), *SelectedTaskTypeName);
    
    ETaskType SelectedTaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskTypeName);
    UE_LOG(LogTemp, Warning, TEXT("Converted to ETaskType: %d"), (int32)SelectedTaskType);

    // タスクタイプに応じたUI制御
    UE_LOG(LogTemp, Warning, TEXT("Calling UpdateUIVisibilityForTaskType..."));
    UpdateUIVisibilityForTaskType(SelectedTaskType);

    // 対象アイテムリストを更新
    UE_LOG(LogTemp, Warning, TEXT("Calling UpdateTargetItemComboBox..."));
    UpdateTargetItemComboBox();

    // 場所リストを更新（冒険の場合のみ）
    if (SelectedTaskType == ETaskType::Adventure)
    {
        UE_LOG(LogTemp, Warning, TEXT("Adventure selected, calling UpdateLocationComboBox..."));
        UpdateLocationComboBox();
    }

    UE_LOG(LogTemp, Warning, TEXT("Calling UpdateCreateButtonState..."));
    UpdateCreateButtonState();
    
    UE_LOG(LogTemp, Warning, TEXT("=== OnTaskTypeSelectionChanged COMPLETED ==="));
}

void UC_TaskMakeSheet::OnTaskTypeComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    UE_LOG(LogTemp, Warning, TEXT("ComboBox Selection Changed: %s (Type: %d)"), *SelectedItem, (int32)SelectionType);
    
    // 実際の処理は既存の関数に委譲
    OnTaskTypeSelectionChanged();
}

void UC_TaskMakeSheet::OnCreateTaskClicked()
{
    FString ErrorMessage;
    if (!ValidateInput(ErrorMessage))
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TaskMakeSheet: Validation failed: %s"), *ErrorMessage);
        
        // エラーメッセージを表示（Blueprint側でダイアログ実装）
        ShowValidationError(ErrorMessage);
        return;
    }

    // 入力からタスクを作成
    FGlobalTask NewTask = CreateTaskFromInput();

    // 指定優先度に挿入（既存タスクは自動で調整される）
    if (TaskManagerComponent && InsertTaskAtPriority(NewTask))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: Successfully created task: %s"), *NewTask.DisplayName);
        
        // 入力フィールドをクリア
        ClearInputFields();
        
        // タスクリストを更新
        RefreshTaskList();
        
        // 成功通知（Blueprint側で実装）
        OnTaskCreatedSuccessfully(NewTask);
        
        // タスク作成成功後、Widgetを自動で閉じる
        CloseTaskMakeSheet();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TaskMakeSheet: Failed to create task"));
        ShowValidationError(TEXT("タスクの作成に失敗しました"));
    }
}

void UC_TaskMakeSheet::OnCancelClicked()
{
    // 入力フィールドをクリア
    ClearInputFields();
    
    // キャンセル通知（Blueprint側で実装）
    OnTaskCreationCancelled();
    
    // Widgetを閉じる
    CloseTaskMakeSheet();
}

void UC_TaskMakeSheet::CloseTaskMakeSheet()
{
    UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: Closing TaskMakeSheet"));
    
    // ViewportからWidgetを削除
    if (IsInViewport())
    {
        RemoveFromParent();
        UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: Removed from viewport"));
    }
    
    // Blueprint側での追加処理があれば実行
    OnTaskMakeSheetClosed();
}

void UC_TaskMakeSheet::InitializeTaskTypeComboBox()
{
    if (!TaskTypeComboBox)
    {
        return;
    }

    TaskTypeComboBox->ClearOptions();

    // 利用可能なタスクタイプを追加
    TArray<ETaskType> TaskTypes = UTaskTypeUtils::GetAllTaskTypes();
    for (ETaskType TaskType : TaskTypes)
    {
        // IdleとAllは除外（ユーザーが作成するタスクではない）
        if (TaskType != ETaskType::Idle && TaskType != ETaskType::All)
        {
            FString TaskTypeName = UTaskTypeUtils::GetTaskTypeDisplayName(TaskType);
            TaskTypeComboBox->AddOption(TaskTypeName);
        }
    }

    // デフォルト選択
    if (TaskTypeComboBox->GetOptionCount() > 0)
    {
        TaskTypeComboBox->SetSelectedIndex(0);
    }
}

void UC_TaskMakeSheet::InitializeDefaultValues()
{
    if (TargetItemComboBox)
    {
        TargetItemComboBox->ClearOptions();
    }

    if (LocationComboBox)
    {
        LocationComboBox->ClearOptions();
    }

    if (TargetQuantitySpinBox)
    {
        TargetQuantitySpinBox->SetValue(1.0f);
        TargetQuantitySpinBox->SetMinValue(1.0f);
        TargetQuantitySpinBox->SetMaxValue(99999.0f);
        TargetQuantitySpinBox->SetDelta(1.0f);
    }

    if (PrioritySpinBox)
    {
        PrioritySpinBox->SetValue(1.0f);
        PrioritySpinBox->SetMinValue(1.0f);
        PrioritySpinBox->SetMaxValue(20.0f);
        PrioritySpinBox->SetDelta(1.0f);
    }

    if (KeepQuantityCheckBox)
    {
        KeepQuantityCheckBox->SetIsChecked(false);
    }

    // 初期化時にタスクタイプ選択変更処理を呼び出してUI状態を適切に設定
    UE_LOG(LogTemp, Warning, TEXT("InitializeDefaultValues: Calling OnTaskTypeSelectionChanged for initial setup"));
    OnTaskTypeSelectionChanged();

    UpdateCreateButtonState();
}

void UC_TaskMakeSheet::UpdateTargetItemComboBox()
{
    UE_LOG(LogTemp, Warning, TEXT("=== UpdateTargetItemComboBox START ==="));
    UE_LOG(LogTemp, Warning, TEXT("TargetItemComboBox: %s"), TargetItemComboBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("CraftingComponent: %s"), CraftingComponent ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("TaskTypeComboBox: %s"), TaskTypeComboBox ? TEXT("OK") : TEXT("NULL"));

    if (!TargetItemComboBox || !CraftingComponent || !TaskTypeComboBox)
    {
        UE_LOG(LogTemp, Error, TEXT("Required components are NULL - Cannot update target item list"));
        return;
    }

    // 現在選択されているタスクタイプを取得
    FString SelectedTaskTypeName = TaskTypeComboBox->GetSelectedOption();
    UE_LOG(LogTemp, Warning, TEXT("Selected Task Type Name: %s"), *SelectedTaskTypeName);
    
    ETaskType SelectedTaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskTypeName);
    UE_LOG(LogTemp, Warning, TEXT("Selected Task Type Enum: %d"), (int32)SelectedTaskType);

    // コンボボックスをクリア
    TargetItemComboBox->ClearOptions();
    UE_LOG(LogTemp, Warning, TEXT("Cleared TargetItemComboBox options"));

    // 選択されたタスクタイプに対応するレシピを取得
    TArray<FString> DisplayNames = CraftingComponent->GetDisplayNamesForCategory(SelectedTaskType);
    UE_LOG(LogTemp, Warning, TEXT("Retrieved %d display names from CraftingComponent"), DisplayNames.Num());

    // コンボボックスに追加
    for (int32 i = 0; i < DisplayNames.Num(); i++)
    {
        const FString& DisplayName = DisplayNames[i];
        TargetItemComboBox->AddOption(DisplayName);
        UE_LOG(LogTemp, Warning, TEXT("Added option %d: %s"), i, *DisplayName);
    }

    // デフォルト選択
    if (TargetItemComboBox->GetOptionCount() > 0)
    {
        TargetItemComboBox->SetSelectedIndex(0);
        UE_LOG(LogTemp, Warning, TEXT("Set default selection to index 0"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No options available - ComboBox remains empty"));
    }

    UE_LOG(LogTemp, Warning, TEXT("=== UpdateTargetItemComboBox COMPLETED ==="));
}

void UC_TaskMakeSheet::UpdateLocationComboBox()
{
    if (!LocationComboBox)
    {
        return;
    }

    // コンボボックスをクリア
    LocationComboBox->ClearOptions();

    // 冒険用の場所リストを追加（仮のデータ）
    TArray<FString> AdventureLocations = {
        TEXT("森の奥地"),
        TEXT("洞窟"),
        TEXT("山頂"),
        TEXT("遺跡"),
        TEXT("魔の森")
    };

    // コンボボックスに追加
    for (const FString& Location : AdventureLocations)
    {
        LocationComboBox->AddOption(Location);
    }

    // デフォルト選択
    if (LocationComboBox->GetOptionCount() > 0)
    {
        LocationComboBox->SetSelectedIndex(0);
    }

    UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: Updated location list (%d locations)"), AdventureLocations.Num());
}

void UC_TaskMakeSheet::UpdateUIVisibilityForTaskType(ETaskType TaskType)
{
    UE_LOG(LogTemp, Warning, TEXT("=== UpdateUIVisibilityForTaskType START ==="));
    UE_LOG(LogTemp, Warning, TEXT("TaskType: %d"), (int32)TaskType);
    UE_LOG(LogTemp, Warning, TEXT("TargetQuantitySpinBox: %s"), TargetQuantitySpinBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("KeepQuantityCheckBox: %s"), KeepQuantityCheckBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("TargetItemComboBox: %s"), TargetItemComboBox ? TEXT("OK") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("LocationComboBox: %s"), LocationComboBox ? TEXT("OK") : TEXT("NULL"));

    if (!TargetQuantitySpinBox || !KeepQuantityCheckBox || !TargetItemComboBox || !LocationComboBox)
    {
        UE_LOG(LogTemp, Error, TEXT("Required UI elements are NULL - Cannot update visibility"));
        return;
    }

    // デフォルトは全て表示
    TargetQuantitySpinBox->SetVisibility(ESlateVisibility::Visible);
    KeepQuantityCheckBox->SetVisibility(ESlateVisibility::Visible);
    TargetItemComboBox->SetVisibility(ESlateVisibility::Visible);
    LocationComboBox->SetVisibility(ESlateVisibility::Collapsed);
    UE_LOG(LogTemp, Warning, TEXT("Set default visibility: All visible except LocationComboBox"));

    switch (TaskType)
    {
        case ETaskType::Adventure:
            // 冒険：場所選択のみ、数量とKeepは非表示
            TargetQuantitySpinBox->SetVisibility(ESlateVisibility::Collapsed);
            KeepQuantityCheckBox->SetVisibility(ESlateVisibility::Collapsed);
            TargetItemComboBox->SetVisibility(ESlateVisibility::Collapsed);
            LocationComboBox->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Warning, TEXT("Adventure mode: Location visible, others collapsed"));
            break;

        case ETaskType::Construction:
            // 建築：アイテム選択のみ、数量とKeepは非表示（常に1個）
            TargetQuantitySpinBox->SetVisibility(ESlateVisibility::Collapsed);
            KeepQuantityCheckBox->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Warning, TEXT("Construction mode: Item visible, quantity/keep collapsed"));
            break;

        case ETaskType::Gathering:
            // 採集：数量とKeepは非表示
            TargetQuantitySpinBox->SetVisibility(ESlateVisibility::Collapsed);
            KeepQuantityCheckBox->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Warning, TEXT("Gathering mode: Item visible, quantity/keep collapsed"));
            break;

        case ETaskType::Cooking:
        case ETaskType::Crafting:
            // 料理・製作：全て表示（デフォルト）
            UE_LOG(LogTemp, Warning, TEXT("Cooking/Crafting mode: All elements visible"));
            break;

        default:
            // その他：数量とKeepは非表示
            TargetQuantitySpinBox->SetVisibility(ESlateVisibility::Collapsed);
            KeepQuantityCheckBox->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Warning, TEXT("Default mode: Item visible, quantity/keep collapsed"));
            break;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== UpdateUIVisibilityForTaskType COMPLETED ==="));
}

bool UC_TaskMakeSheet::ValidateInput(FString& OutErrorMessage) const
{
    // タスクタイプチェック
    if (!TaskTypeComboBox || TaskTypeComboBox->GetSelectedOption().IsEmpty())
    {
        OutErrorMessage = TEXT("タスクタイプを選択してください");
        return false;
    }

    // 現在選択されているタスクタイプを取得
    FString SelectedTaskTypeName = TaskTypeComboBox->GetSelectedOption();
    ETaskType SelectedTaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskTypeName);

    // タスクタイプ別の検証
    switch (SelectedTaskType)
    {
        case ETaskType::Adventure:
            // 冒険：場所選択のみチェック
            if (!LocationComboBox || LocationComboBox->GetSelectedOption().IsEmpty())
            {
                OutErrorMessage = TEXT("冒険場所を選択してください");
                return false;
            }
            break;

        case ETaskType::Construction:
        case ETaskType::Gathering:
            // 建築・採集：アイテム選択のみチェック
            if (!TargetItemComboBox || TargetItemComboBox->GetSelectedOption().IsEmpty())
            {
                OutErrorMessage = TEXT("対象アイテムを選択してください");
                return false;
            }
            break;

        case ETaskType::Cooking:
        case ETaskType::Crafting:
            // 料理・製作：アイテムと数量をチェック
            if (!TargetItemComboBox || TargetItemComboBox->GetSelectedOption().IsEmpty())
            {
                OutErrorMessage = TEXT("対象アイテムを選択してください");
                return false;
            }
            if (!TargetQuantitySpinBox || TargetQuantitySpinBox->GetValue() < 1.0f)
            {
                OutErrorMessage = TEXT("目標数量は1以上である必要があります");
                return false;
            }
            break;

        default:
            // その他：基本的なアイテム選択チェック
            if (!TargetItemComboBox || TargetItemComboBox->GetSelectedOption().IsEmpty())
            {
                OutErrorMessage = TEXT("対象アイテムを選択してください");
                return false;
            }
            break;
    }

    // 優先度チェック
    if (!PrioritySpinBox)
    {
        OutErrorMessage = TEXT("優先度が設定されていません");
        return false;
    }

    int32 InputPriority = static_cast<int32>(PrioritySpinBox->GetValue());
    if (InputPriority < 1 || InputPriority > 20)
    {
        OutErrorMessage = TEXT("優先度は1-20の範囲で設定してください");
        return false;
    }

    // 優先度重複時は挿入方式で自動調整するため、チェック不要

    return true;
}

FGlobalTask UC_TaskMakeSheet::CreateTaskFromInput() const
{
    FGlobalTask NewTask;

    // ユニークなタスクID生成
    NewTask.TaskId = GenerateUniqueTaskId();

    // 優先度
    NewTask.Priority = static_cast<int32>(PrioritySpinBox->GetValue());

    // タスクタイプを先に設定
    FString SelectedTaskTypeName = TaskTypeComboBox->GetSelectedOption();
    NewTask.TaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskTypeName);

    // タスクタイプ別の設定
    switch (NewTask.TaskType)
    {
        case ETaskType::Adventure:
            // 冒険：場所情報を設定
            NewTask.TargetItemId = LocationComboBox->GetSelectedOption();
            NewTask.TargetQuantity = 1; // 冒険は常に1
            break;

        case ETaskType::Construction:
            // 建築：アイテムID設定、数量は常に1
            {
                FString SelectedDisplayName = TargetItemComboBox->GetSelectedOption();
                if (CraftingComponent)
                {
                    NewTask.TargetItemId = CraftingComponent->GetRecipeIdFromDisplayName(SelectedDisplayName, NewTask.TaskType);
                }
                else
                {
                    NewTask.TargetItemId = SelectedDisplayName;
                }
                NewTask.TargetQuantity = 1; // 建築は常に1個
            }
            break;

        case ETaskType::Gathering:
            // 採集：アイテムID設定、数量は1固定
            {
                FString SelectedDisplayName = TargetItemComboBox->GetSelectedOption();
                if (CraftingComponent)
                {
                    NewTask.TargetItemId = CraftingComponent->GetRecipeIdFromDisplayName(SelectedDisplayName, NewTask.TaskType);
                }
                else
                {
                    NewTask.TargetItemId = SelectedDisplayName;
                }
                NewTask.TargetQuantity = 1; // 採集は1固定
            }
            break;

        case ETaskType::Cooking:
        case ETaskType::Crafting:
        default:
            // 料理・製作：通常の処理
            {
                FString SelectedDisplayName = TargetItemComboBox->GetSelectedOption();
                if (CraftingComponent)
                {
                    NewTask.TargetItemId = CraftingComponent->GetRecipeIdFromDisplayName(SelectedDisplayName, NewTask.TaskType);
                }
                else
                {
                    NewTask.TargetItemId = SelectedDisplayName;
                }
                NewTask.TargetQuantity = static_cast<int32>(TargetQuantitySpinBox->GetValue());
            }
            break;
    }

    // 数量キープ型（冒険は常にfalse）
    if (NewTask.TaskType == ETaskType::Adventure)
    {
        NewTask.bIsKeepQuantity = false;
    }
    else
    {
        NewTask.bIsKeepQuantity = KeepQuantityCheckBox->IsChecked();
    }

    // 表示名を生成（タスクタイプとTargetItemIdが設定された後）
    FString TaskTypeName = UTaskTypeUtils::GetTaskTypeDisplayName(NewTask.TaskType);
    if (NewTask.TaskType == ETaskType::Adventure)
    {
        // 冒険の場合は場所名を表示
        NewTask.DisplayName = FString::Printf(TEXT("%s: %s"), 
            *TaskTypeName, *NewTask.TargetItemId);
    }
    else
    {
        // その他は数量も表示
        NewTask.DisplayName = FString::Printf(TEXT("%s: %s x%d"), 
            *TaskTypeName, *NewTask.TargetItemId, NewTask.TargetQuantity);
    }

    // 関連スキル情報を自動設定
    NewTask.RelatedSkills = UTaskTypeUtils::GetTaskRelatedSkills(NewTask.TaskType);

    // その他の初期値
    NewTask.CurrentProgress = 0;
    NewTask.bIsCompleted = false;
    NewTask.CreatedTime = FDateTime::Now();

    UE_LOG(LogTemp, Warning, TEXT("Created task: %s (Type: %d, Target: %s, Quantity: %d)"), 
        *NewTask.DisplayName, (int32)NewTask.TaskType, *NewTask.TargetItemId, NewTask.TargetQuantity);

    return NewTask;
}

FString UC_TaskMakeSheet::GenerateUniqueTaskId() const
{
    // タイムスタンプ + ランダム値でユニークなIDを生成
    FDateTime Now = FDateTime::Now();
    int32 RandomValue = FMath::RandRange(1000, 9999);
    
    return FString::Printf(TEXT("task_%d%02d%02d_%02d%02d%02d_%d"),
        Now.GetYear(),
        Now.GetMonth(),
        Now.GetDay(),
        Now.GetHour(),
        Now.GetMinute(),
        Now.GetSecond(),
        RandomValue
    );
}

UC_GlobalTaskCard* UC_TaskMakeSheet::CreateTaskCard(const FGlobalTask& Task, int32 TaskIndex)
{
    if (!GlobalTaskCardClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TaskMakeSheet: GlobalTaskCardClass not set"));
        return nullptr;
    }

    UC_GlobalTaskCard* TaskCard = CreateWidget<UC_GlobalTaskCard>(this, GlobalTaskCardClass);
    if (TaskCard)
    {
        TaskCard->InitializeWithTask(Task, TaskIndex, TaskManagerComponent);
    }

    return TaskCard;
}

void UC_TaskMakeSheet::UpdateCreateButtonState()
{
    if (!CreateTaskButton)
    {
        return;
    }

    // 基本的な入力項目が埋まっているかチェック
    bool bCanCreate = true;

    if (!TaskTypeComboBox || TaskTypeComboBox->GetSelectedOption().IsEmpty())
    {
        bCanCreate = false;
    }
    else
    {
        // タスクタイプ別の必須項目チェック
        FString SelectedTaskTypeName = TaskTypeComboBox->GetSelectedOption();
        ETaskType SelectedTaskType = UTaskTypeUtils::GetTaskTypeFromString(SelectedTaskTypeName);
        
        switch (SelectedTaskType)
        {
            case ETaskType::Adventure:
                // 冒険：場所選択のみチェック
                if (!LocationComboBox || LocationComboBox->GetSelectedOption().IsEmpty())
                {
                    bCanCreate = false;
                }
                break;

            case ETaskType::Construction:
            case ETaskType::Gathering:
            case ETaskType::Cooking:
            case ETaskType::Crafting:
            default:
                // その他：アイテム選択チェック
                if (!TargetItemComboBox || TargetItemComboBox->GetSelectedOption().IsEmpty())
                {
                    bCanCreate = false;
                }
                break;
        }
    }

    CreateTaskButton->SetIsEnabled(bCanCreate);
}

void UC_TaskMakeSheet::ClearInputFields()
{
    if (TargetItemComboBox && TargetItemComboBox->GetOptionCount() > 0)
    {
        TargetItemComboBox->SetSelectedIndex(0);
    }

    if (LocationComboBox && LocationComboBox->GetOptionCount() > 0)
    {
        LocationComboBox->SetSelectedIndex(0);
    }

    if (TargetQuantitySpinBox)
    {
        TargetQuantitySpinBox->SetValue(1.0f);
    }

    if (PrioritySpinBox)
    {
        PrioritySpinBox->SetValue(1.0f);
    }

    if (KeepQuantityCheckBox)
    {
        KeepQuantityCheckBox->SetIsChecked(false);
    }

    if (TaskTypeComboBox && TaskTypeComboBox->GetOptionCount() > 0)
    {
        TaskTypeComboBox->SetSelectedIndex(0);
    }

    UpdateCreateButtonState();
}

void UC_TaskMakeSheet::BindEvents()
{
    if (!TaskManagerComponent)
    {
        return;
    }

    // 既存のバインドを確実に解除してから再バインド
    TaskManagerComponent->OnGlobalTaskAdded.RemoveAll(this);
    TaskManagerComponent->OnGlobalTaskRemoved.RemoveAll(this);

    // TaskManagerComponentのイベントをバインド
    TaskManagerComponent->OnGlobalTaskAdded.AddDynamic(this, &UC_TaskMakeSheet::OnGlobalTaskAdded);
    TaskManagerComponent->OnGlobalTaskRemoved.AddDynamic(this, &UC_TaskMakeSheet::OnGlobalTaskRemoved);

    // UI要素のイベントをバインド（既存バインドを先に解除）
    if (CreateTaskButton)
    {
        CreateTaskButton->OnClicked.RemoveAll(this);
        CreateTaskButton->OnClicked.AddDynamic(this, &UC_TaskMakeSheet::OnCreateTaskClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveAll(this);
        CancelButton->OnClicked.AddDynamic(this, &UC_TaskMakeSheet::OnCancelClicked);
    }

    // ComboBoxのSelectionChangedイベントをバインド
    if (TaskTypeComboBox)
    {
        TaskTypeComboBox->OnSelectionChanged.RemoveAll(this);
        TaskTypeComboBox->OnSelectionChanged.AddDynamic(this, &UC_TaskMakeSheet::OnTaskTypeComboBoxChanged);
    }

    // UMGイベントは手動で呼び出すため、ここでは自動バインディングしない
    // Blueprint側でイベントを適切にバインドする
}

void UC_TaskMakeSheet::UnbindEvents()
{
    if (TaskManagerComponent)
    {
        TaskManagerComponent->OnGlobalTaskAdded.RemoveAll(this);
        TaskManagerComponent->OnGlobalTaskRemoved.RemoveAll(this);
    }

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

    // UMGイベントのアンバインドは不要（手動呼び出しのため）
}

void UC_TaskMakeSheet::OnGlobalTaskAdded(const FGlobalTask& NewTask)
{
    // タスクが追加されたらリストを更新
    RefreshTaskList();
}

void UC_TaskMakeSheet::OnGlobalTaskRemoved(int32 TaskIndex)
{
    // タスクが削除されたらリストを更新
    RefreshTaskList();
}

// 指定優先度にタスクを挿入（既存タスクの優先度を自動調整）
bool UC_TaskMakeSheet::InsertTaskAtPriority(const FGlobalTask& NewTask)
{
    if (!TaskManagerComponent)
    {
        return false;
    }

    int32 TargetPriority = NewTask.Priority;
    
    // 指定優先度以降の既存タスクの優先度を+1ずつ調整
    TArray<FGlobalTask> ExistingTasks = TaskManagerComponent->GetGlobalTasks();
    for (int32 i = 0; i < ExistingTasks.Num(); i++)
    {
        if (ExistingTasks[i].Priority >= TargetPriority)
        {
            // 優先度を+1する
            TaskManagerComponent->UpdateTaskPriority(i, ExistingTasks[i].Priority + 1);
        }
    }
    
    // 新しいタスクを追加
    int32 TaskIndex = TaskManagerComponent->AddGlobalTask(NewTask);
    bool bSuccess = (TaskIndex >= 0);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TaskMakeSheet: Inserted task at priority %d, adjusted %d existing tasks"), 
            TargetPriority, ExistingTasks.Num());
    }
    
    return bSuccess;
}

// テキスト変更時のハンドラー（ボタン状態更新用）
void UC_TaskMakeSheet::OnInputChanged()
{
    UpdateCreateButtonState();
}