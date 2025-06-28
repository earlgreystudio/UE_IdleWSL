#include "C_TeamCard.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "../Components/TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "C_CharacterCard.h"
#include "C_TeamTaskCard.h"
#include "Framework/Application/SlateApplication.h"

UC_TeamCard::UC_TeamCard(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    TeamIndex = -1;
    TeamComponent = nullptr;
    bIsInitialized = false;
    
    UE_LOG(LogTemp, Warning, TEXT("===== UC_TeamCard CREATED ====="));
}

void UC_TeamCard::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamCard::NativeConstruct - TEAM CARD CONSTRUCTED ====="));
    
    if (CharacterCardsContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CharacterCardsContainer found"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - CharacterCardsContainer is NULL! Check Blueprint binding"));
    }
    
    // チームタスク作成ボタンのバインド
    if (CreateTaskButton)
    {
        CreateTaskButton->OnClicked.AddDynamic(this, &UC_TeamCard::OnCreateTaskClicked);
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CreateTaskButton bound"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CreateTaskButton is NULL (optional)"));
    }
    
    // CharacterCardClassが設定されていない場合は自動設定を試行
    if (!CharacterCardClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CharacterCardClass not set, trying to auto-detect"));
        CharacterCardClass = UC_CharacterCard::StaticClass();
        if (CharacterCardClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - Auto-set CharacterCardClass to UC_CharacterCard"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - Failed to auto-set CharacterCardClass"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CharacterCardClass is already set"));
    }
    
    // TeamTaskMakeSheetClassが設定されていない場合は自動設定を試行
    if (!TeamTaskMakeSheetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - TeamTaskMakeSheetClass not set, trying to auto-detect"));
        TeamTaskMakeSheetClass = UC_TeamTaskMakeSheet::StaticClass();
        if (TeamTaskMakeSheetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - Auto-set TeamTaskMakeSheetClass to UC_TeamTaskMakeSheet"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - Failed to auto-set TeamTaskMakeSheetClass"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - TeamTaskMakeSheetClass is already set"));
    }
    
    // TeamTaskCardClassが設定されていない場合は自動設定を試行
    if (!TeamTaskCardClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - TeamTaskCardClass not set, trying to auto-detect"));
        TeamTaskCardClass = UC_TeamTaskCard::StaticClass();
        if (TeamTaskCardClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - Auto-set TeamTaskCardClass to UC_TeamTaskCard"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - Failed to auto-set TeamTaskCardClass"));
        }
    }
}

void UC_TeamCard::NativeDestruct()
{
    UnbindTeamEvents();
    ClearCharacterCards();
    ClearTaskCards();

    // ボタンイベントのアンバインド
    if (CreateTaskButton)
    {
        CreateTaskButton->OnClicked.RemoveDynamic(this, &UC_TeamCard::OnCreateTaskClicked);
    }

    Super::NativeDestruct();
}

void UC_TeamCard::InitializeWithTeam(int32 InTeamIndex, UTeamComponent* InTeamComponent)
{
    if (!InTeamComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::InitializeWithTeam - InTeamComponent is null"));
        return;
    }

    if (!InTeamComponent->IsValidTeamIndex(InTeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::InitializeWithTeam - Invalid team index: %d"), InTeamIndex);
        return;
    }

    // 既存のイベントバインドを解除
    UnbindTeamEvents();

    TeamIndex = InTeamIndex;
    TeamComponent = InTeamComponent;
    bIsInitialized = true;

    // 新しいTeamComponentのイベントにバインド
    BindTeamEvents();

    // 初回表示更新
    UpdateDisplay();
}

void UC_TeamCard::UpdateDisplay()
{
    if (!bIsInitialized || !IsValidTeamCard())
    {
        return;
    }

    UpdateTeamInfo();
    UpdateCharacterCards();
    UpdateTaskCards();
}

void UC_TeamCard::UpdateTeamInfo()
{
    if (!IsValidTeamCard())
    {
        return;
    }

    UpdateTeamNameDisplay();
    UpdateCurrentTaskDisplay();
    UpdateTeamStatusDisplay();
    UpdateMemberCountDisplay();
}

void UC_TeamCard::UpdateCharacterCards()
{
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamCard::UpdateCharacterCards - START ====="));
    
    if (!IsValidTeamCard())
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateCharacterCards - Invalid team card"));
        return;
    }
    
    if (!CharacterCardsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateCharacterCards - CharacterCardsContainer is NULL"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Clearing existing cards"));
    // 既存のキャラクターカードをクリア
    ClearCharacterCards();

    // チームメンバーのカードを作成
    FTeam TeamData = GetTeamData();
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Team has %d members"), TeamData.Members.Num());
    
    for (int32 i = 0; i < TeamData.Members.Num(); ++i)
    {
        AC_IdleCharacter* Character = TeamData.Members[i];
        if (Character)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Creating card for member %d"), i);
            UC_CharacterCard* CharacterCard = CreateCharacterCard(Character);
            if (CharacterCard)
            {
                CharacterCards.Add(CharacterCard);
                CharacterCardsContainer->AddChildToWrapBox(CharacterCard);
                UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Successfully added character card %d to container"), i);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateCharacterCards - Failed to create character card for member %d"), i);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateCharacterCards - Member %d is NULL"), i);
        }
    }
    
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamCard::UpdateCharacterCards - END (Total cards: %d) ====="), CharacterCards.Num());
}

void UC_TeamCard::UpdateTaskCards()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateTaskCards - START for Team %d"), TeamIndex);
    
    if (!IsValidTeamCard() || !TeamTaskCardsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateTaskCards - Invalid team card or container"));
        return;
    }

    // 既存のタスクカードをクリア
    ClearTaskCards();

    // チームタスクのカードを作成
    TArray<FTeamTask> TeamTasks = TeamComponent->GetTeamTasks(TeamIndex);
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateTaskCards - Found %d tasks for Team %d"), TeamTasks.Num(), TeamIndex);
    
    for (int32 i = 0; i < TeamTasks.Num(); ++i)
    {
        UC_TeamTaskCard* TaskCard = CreateTaskCard(TeamTasks[i], i);
        if (TaskCard)
        {
            TeamTaskCards.Add(TaskCard);
            TeamTaskCardsContainer->AddChildToVerticalBox(TaskCard);
            UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateTaskCards - Added task card %d"), i);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateTaskCards - Failed to create task card %d"), i);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateTaskCards - COMPLETED"));
}

FTeam UC_TeamCard::GetTeamData() const
{
    if (IsValidTeamCard())
    {
        return TeamComponent->GetTeam(TeamIndex);
    }
    return FTeam();
}

bool UC_TeamCard::IsValidTeamCard() const
{
    return bIsInitialized && 
           TeamComponent && 
           TeamComponent->IsValidTeamIndex(TeamIndex);
}

void UC_TeamCard::BindTeamEvents()
{
    if (!TeamComponent)
    {
        return;
    }

    TeamComponent->OnMemberAssigned.AddDynamic(this, &UC_TeamCard::OnMemberAssigned);
    TeamComponent->OnMemberRemoved.AddDynamic(this, &UC_TeamCard::OnMemberRemoved);
    TeamComponent->OnTaskChanged.AddDynamic(this, &UC_TeamCard::OnTaskChanged);
    TeamComponent->OnTeamNameChanged.AddDynamic(this, &UC_TeamCard::OnTeamNameChanged);
    TeamComponent->OnTeamActionStateChanged.AddDynamic(this, &UC_TeamCard::OnTeamActionStateChanged);
    TeamComponent->OnTeamTaskStarted.AddDynamic(this, &UC_TeamCard::OnTeamTaskStarted);
    TeamComponent->OnTeamTaskCompleted.AddDynamic(this, &UC_TeamCard::OnTeamTaskCompleted);
    TeamComponent->OnCharacterDataChanged.AddDynamic(this, &UC_TeamCard::OnCharacterDataChanged);
}

void UC_TeamCard::UnbindTeamEvents()
{
    if (!TeamComponent)
    {
        return;
    }

    TeamComponent->OnMemberAssigned.RemoveDynamic(this, &UC_TeamCard::OnMemberAssigned);
    TeamComponent->OnMemberRemoved.RemoveDynamic(this, &UC_TeamCard::OnMemberRemoved);
    TeamComponent->OnTaskChanged.RemoveDynamic(this, &UC_TeamCard::OnTaskChanged);
    TeamComponent->OnTeamNameChanged.RemoveDynamic(this, &UC_TeamCard::OnTeamNameChanged);
    TeamComponent->OnTeamActionStateChanged.RemoveDynamic(this, &UC_TeamCard::OnTeamActionStateChanged);
    TeamComponent->OnTeamTaskStarted.RemoveDynamic(this, &UC_TeamCard::OnTeamTaskStarted);
    TeamComponent->OnTeamTaskCompleted.RemoveDynamic(this, &UC_TeamCard::OnTeamTaskCompleted);
    TeamComponent->OnCharacterDataChanged.RemoveDynamic(this, &UC_TeamCard::OnCharacterDataChanged);
}

void UC_TeamCard::OnMemberAssigned(int32 InTeamIndex, AC_IdleCharacter* Character, const FString& TeamName)
{
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnMemberAssigned - Team %d"), TeamIndex);
        UpdateCharacterCards();
        UpdateMemberCountDisplay();
    }
}

void UC_TeamCard::OnMemberRemoved(int32 InTeamIndex, AC_IdleCharacter* Character)
{
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnMemberRemoved - Team %d"), TeamIndex);
        UpdateCharacterCards();
        UpdateMemberCountDisplay();
    }
}

void UC_TeamCard::OnTaskChanged(int32 InTeamIndex, ETaskType NewTask)
{
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnTaskChanged - Team %d"), TeamIndex);
        UpdateCurrentTaskDisplay();
        UpdateTeamStatusDisplay();
    }
}

void UC_TeamCard::OnTeamNameChanged(int32 InTeamIndex, const FString& NewName)
{
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnTeamNameChanged - Team %d: %s"), TeamIndex, *NewName);
        UpdateTeamNameDisplay();
    }
}

void UC_TeamCard::OnTeamActionStateChanged(int32 InTeamIndex, ETeamActionState NewState)
{
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnTeamActionStateChanged - Team %d"), TeamIndex);
        UpdateTeamStatusDisplay();
    }
}

void UC_TeamCard::OnTeamTaskStarted(int32 InTeamIndex, const FTeamTask& StartedTask)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::OnTeamTaskStarted - Called for Team %d, My Team: %d"), InTeamIndex, TeamIndex);
    
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::OnTeamTaskStarted - Updating task cards for Team %d"), TeamIndex);
        UpdateTaskCards();
        UpdateCurrentTaskDisplay();
    }
}

void UC_TeamCard::OnTeamTaskCompleted(int32 InTeamIndex, const FTeamTask& CompletedTask)
{
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnTeamTaskCompleted - Team %d"), TeamIndex);
        UpdateTaskCards();
        UpdateCurrentTaskDisplay();
    }
}

void UC_TeamCard::OnCharacterDataChanged(AC_IdleCharacter* Character)
{
    // このチームのメンバーかチェック
    FTeam TeamData = GetTeamData();
    if (TeamData.Members.Contains(Character))
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamCard::OnCharacterDataChanged - Team %d"), TeamIndex);
        UpdateCharacterCards();
    }
}

void UC_TeamCard::ClearCharacterCards()
{
    if (CharacterCardsContainer)
    {
        CharacterCardsContainer->ClearChildren();
    }

    for (UC_CharacterCard* Card : CharacterCards)
    {
        if (Card)
        {
            Card->RemoveFromParent();
        }
    }

    CharacterCards.Empty();
}

void UC_TeamCard::ClearTaskCards()
{
    if (TeamTaskCardsContainer)
    {
        TeamTaskCardsContainer->ClearChildren();
    }

    for (UC_TeamTaskCard* Card : TeamTaskCards)
    {
        if (Card)
        {
            Card->RemoveFromParent();
        }
    }

    TeamTaskCards.Empty();
}

UC_CharacterCard* UC_TeamCard::CreateCharacterCard(AC_IdleCharacter* Character)
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::CreateCharacterCard - START"));
    
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::CreateCharacterCard - Character is null"));
        return nullptr;
    }
    
    if (!CharacterCardClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::CreateCharacterCard - CharacterCardClass is null! Check Blueprint settings"));
        return nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::CreateCharacterCard - Creating widget with class"));
    UC_CharacterCard* NewCard = CreateWidget<UC_CharacterCard>(this, CharacterCardClass);
    
    if (NewCard)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::CreateCharacterCard - Widget created successfully, initializing"));
        NewCard->InitializeWithCharacter(Character);
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::CreateCharacterCard - Character card initialized successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::CreateCharacterCard - Failed to create widget"));
    }

    return NewCard;
}

UC_TeamTaskCard* UC_TeamCard::CreateTaskCard(const FTeamTask& TaskData, int32 TaskPriority)
{
    if (!TeamTaskCardClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::CreateTaskCard - TeamTaskCardClass is null"));
        return nullptr;
    }

    UC_TeamTaskCard* NewCard = CreateWidget<UC_TeamTaskCard>(this, TeamTaskCardClass);
    if (NewCard)
    {
        NewCard->InitializeWithTeamTask(TaskData, TeamIndex, TaskPriority, TeamComponent);
    }

    return NewCard;
}

FString UC_TeamCard::GetCurrentTaskDisplayText() const
{
    if (!IsValidTeamCard())
    {
        return TEXT("無効なチーム");
    }

    FTeam TeamData = GetTeamData();
    
    // 現在実行中のタスクを取得
    FTeamTask CurrentTask = TeamComponent->GetCurrentTeamTask(TeamIndex);
    if (CurrentTask.TaskType != ETaskType::Idle)
    {
        return UTeamComponent::GetTaskTypeDisplayName(CurrentTask.TaskType);
    }

    // 割り当てられたタスクを表示
    if (TeamData.AssignedTask != ETaskType::Idle)
    {
        return UTeamComponent::GetTaskTypeDisplayName(TeamData.AssignedTask);
    }

    return TEXT("待機中");
}

FString UC_TeamCard::GetTeamStatusDisplayText() const
{
    if (!IsValidTeamCard())
    {
        return TEXT("無効");
    }

    FTeam TeamData = GetTeamData();
    return TeamData.GetActionStateDisplayName();
}

void UC_TeamCard::UpdateTeamNameDisplay()
{
    if (TeamNameText)
    {
        FTeam TeamData = GetTeamData();
        TeamNameText->SetText(FText::FromString(TeamData.TeamName));
    }
}

void UC_TeamCard::UpdateCurrentTaskDisplay()
{
    if (CurrentTaskText)
    {
        FString TaskText = GetCurrentTaskDisplayText();
        CurrentTaskText->SetText(FText::FromString(TaskText));
    }
}

void UC_TeamCard::UpdateTeamStatusDisplay()
{
    if (TeamStatusText)
    {
        FString StatusText = GetTeamStatusDisplayText();
        TeamStatusText->SetText(FText::FromString(StatusText));
    }
}

void UC_TeamCard::UpdateMemberCountDisplay()
{
    if (MemberCountText)
    {
        FTeam TeamData = GetTeamData();
        FString CountText = FString::Printf(TEXT("メンバー: %d人"), TeamData.Members.Num());
        MemberCountText->SetText(FText::FromString(CountText));
    }
}

// ======== チームタスク作成ボタンハンドラー ========

void UC_TeamCard::OnCreateTaskClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::OnCreateTaskClicked - Team %d"), TeamIndex);
    
    if (!IsValidTeamCard())
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::OnCreateTaskClicked - Invalid team card"));
        return;
    }
    
    // TeamTaskMakeSheetを表示
    ShowTeamTaskMakeSheet();
}

void UC_TeamCard::ShowTeamTaskMakeSheet()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Creating task make sheet for Team %d"), TeamIndex);
    
    if (!IsValidTeamCard())
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Invalid team card"));
        return;
    }
    
    if (!TeamTaskMakeSheetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - TeamTaskMakeSheetClass is null! Check Blueprint settings"));
        return;
    }
    
    // PlayerControllerを取得してウィジェットの正しいオーナーとして使用
    APlayerController* OwningPlayer = GetOwningPlayer();
    if (!OwningPlayer)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - No owning player controller"));
        return;
    }
    
    // 正しいオーナー（PlayerController）でウィジェットを作成
    UC_TeamTaskMakeSheet* TaskMakeSheet = CreateWidget<UC_TeamTaskMakeSheet>(OwningPlayer, TeamTaskMakeSheetClass);
    if (TaskMakeSheet)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Widget created successfully"));
        
        // チーム情報で初期化
        TaskMakeSheet->InitializeWithTeam(TeamIndex, TeamComponent);
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Widget initialized"));
        
        // 画面に表示
        TaskMakeSheet->AddToViewport();
        
        // UIインタラクション用の入力モード設定
        OwningPlayer->SetInputMode(FInputModeGameAndUI());
        OwningPlayer->bShowMouseCursor = true;
        
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - TeamTaskMakeSheet added to viewport with input mode"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Failed to create TaskMakeSheet widget"));
    }
}