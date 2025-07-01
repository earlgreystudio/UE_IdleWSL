#include "C_TeamCard.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "../Components/TeamComponent.h"
#include "../Components/LocationMovementComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Components/TaskManagerComponent.h"
#include "../Components/TimeManagerComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../C_PlayerController.h"
#include "C_CharacterCard.h"
#include "C_TeamTaskCard.h"
#include "Framework/Application/SlateApplication.h"

UC_TeamCard::UC_TeamCard(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    TeamIndex = -1;
    TeamComponent = nullptr;
    bIsInitialized = false;
    
    // Team card created
}

void UC_TeamCard::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Team card constructed
    
    if (CharacterCardsContainer)
    {
        // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CharacterCardsContainer found"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - CharacterCardsContainer is NULL! Check Blueprint binding"));
    }
    
    // チームタスク作成ボタンのバインド
    if (CreateTaskButton)
    {
        CreateTaskButton->OnClicked.AddDynamic(this, &UC_TeamCard::OnCreateTaskClicked);
        // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CreateTaskButton bound"));
    }
    else
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - CreateTaskButton is NULL (optional)"));
    }
    
    // CharacterCardClassが設定されていない場合は自動設定を試行
    if (!CharacterCardClass)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - CharacterCardClass not set, trying to auto-detect"));
        CharacterCardClass = UC_CharacterCard::StaticClass();
        if (CharacterCardClass)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - Auto-set CharacterCardClass to UC_CharacterCard"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - Failed to auto-set CharacterCardClass"));
        }
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - CharacterCardClass is already set"));
    }
    
    // TeamTaskMakeSheetClassが設定されていない場合は自動設定を試行
    if (!TeamTaskMakeSheetClass)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - TeamTaskMakeSheetClass not set, trying to auto-detect"));
        TeamTaskMakeSheetClass = UC_TeamTaskMakeSheet::StaticClass();
        if (TeamTaskMakeSheetClass)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - Auto-set TeamTaskMakeSheetClass to UC_TeamTaskMakeSheet"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::NativeConstruct - Failed to auto-set TeamTaskMakeSheetClass"));
        }
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::NativeConstruct - TeamTaskMakeSheetClass is already set"));
    }
    
    // TeamTaskCardClassが設定されていない場合は自動設定を試行
    if (!TeamTaskCardClass)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - TeamTaskCardClass not set, trying to auto-detect"));
        TeamTaskCardClass = UC_TeamTaskCard::StaticClass();
        if (TeamTaskCardClass)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::NativeConstruct - Auto-set TeamTaskCardClass to UC_TeamTaskCard"));
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
    UpdateDistanceFromBaseDisplay();
}

void UC_TeamCard::UpdateCharacterCards()
{
    // Character cards update start
    
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

    // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Clearing existing cards"));
    // 既存のキャラクターカードをクリア
    ClearCharacterCards();

    // チームメンバーのカードを作成
    FTeam TeamData = GetTeamData();
    UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::UpdateCharacterCards - Team has %d members"), TeamData.Members.Num());
    
    for (int32 i = 0; i < TeamData.Members.Num(); ++i)
    {
        AC_IdleCharacter* Character = TeamData.Members[i];
        if (Character)
        {
            // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Creating card for member %d"), i);
            UC_CharacterCard* CharacterCard = CreateCharacterCard(Character);
            if (CharacterCard)
            {
                CharacterCards.Add(CharacterCard);
                CharacterCardsContainer->AddChildToWrapBox(CharacterCard);
                // UE_LOG(LogTemp, Warning, TEXT("UC_TeamCard::UpdateCharacterCards - Successfully added character card %d to container"), i);
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
    
    // Character cards update end
}

void UC_TeamCard::UpdateTaskCards()
{
    // Updating task cards
    
    if (!IsValidTeamCard() || !TeamTaskCardsContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateTaskCards - Invalid team card or container"));
        return;
    }

    // 既存のタスクカードをクリア
    ClearTaskCards();

    // チームタスクのカードを作成
    TArray<FTeamTask> TeamTasks = TeamComponent->GetTeamTasks(TeamIndex);
    // Found tasks
    
    for (int32 i = 0; i < TeamTasks.Num(); ++i)
    {
        UC_TeamTaskCard* TaskCard = CreateTaskCard(TeamTasks[i], i);
        if (TaskCard)
        {
            TeamTaskCards.Add(TaskCard);
            TeamTaskCardsContainer->AddChildToVerticalBox(TaskCard);
            // Task card added
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::UpdateTaskCards - Failed to create task card %d"), i);
        }
    }
    
    // Task cards update completed
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
        UE_LOG(LogTemp, Error, TEXT("BindTeamEvents: TeamComponent is NULL for TeamCard %d"), TeamIndex);
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
    TeamComponent->OnTeamsUpdated.AddDynamic(this, &UC_TeamCard::OnTeamsUpdated);
    
    // MovementComponentのイベントもバインド
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (ULocationMovementComponent* MovementComp = IdlePC->MovementComponent)
                {
                    MovementComp->OnMovementProgressUpdated.AddDynamic(this, &UC_TeamCard::OnMovementProgressUpdated);
                }
            }
        }
    }
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
    TeamComponent->OnTeamsUpdated.RemoveDynamic(this, &UC_TeamCard::OnTeamsUpdated);
    
    // MovementComponentのイベントもアンバインド
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (ULocationMovementComponent* MovementComp = IdlePC->MovementComponent)
                {
                    MovementComp->OnMovementProgressUpdated.RemoveDynamic(this, &UC_TeamCard::OnMovementProgressUpdated);
                }
            }
        }
    }
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
        UpdateTeamStatusDisplay();
    }
}

void UC_TeamCard::OnTeamTaskStarted(int32 InTeamIndex, const FTeamTask& StartedTask)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::OnTeamTaskStarted - Called for Team %d, My Team: %d"), InTeamIndex, TeamIndex);
    
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::OnTeamTaskStarted - Updating task cards for Team %d"), TeamIndex);
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

void UC_TeamCard::OnTeamsUpdated()
{
    // アイテム採集等でチーム状態が変わった可能性があるため、全ての表示を更新
    UpdateTeamStatusDisplay();
    UpdateCurrentTaskDisplay();
    UpdateCharacterCards();
}

void UC_TeamCard::OnMovementProgressUpdated(int32 InTeamIndex, const FMovementInfo& MovementInfo)
{
    // 自分のチームの移動進捗のみ処理
    if (InTeamIndex == TeamIndex)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("C_TeamCard: Movement progress update for team %d - distance: %.1fm, remaining: %.1fs"), 
            InTeamIndex, MovementInfo.CurrentDistanceFromBase, MovementInfo.RemainingTime);
        
        // ステータスと距離表示を更新
        UpdateTeamStatusDisplay();
        UpdateDistanceFromBaseDisplay();
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
    UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::CreateCharacterCard - START"));
    
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

    UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::CreateCharacterCard - Creating widget with class"));
    UC_CharacterCard* NewCard = CreateWidget<UC_CharacterCard>(this, CharacterCardClass);
    
    if (NewCard)
    {
        // Widget created successfully
        NewCard->InitializeWithCharacter(Character);
        // Character card initialized
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
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::CreateTaskCard - TeamTaskCardClass is null"));
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
        return UTaskTypeUtils::GetTaskTypeDisplayName(CurrentTask.TaskType);
    }

    // 割り当てられたタスクを表示
    if (TeamData.AssignedTask != ETaskType::Idle)
    {
        return UTaskTypeUtils::GetTaskTypeDisplayName(TeamData.AssignedTask);
    }

    return TEXT("待機中");
}

FString UC_TeamCard::GetTeamStatusDisplayText() const
{
    if (!IsValidTeamCard())
    {
        return TEXT("無効");
    }

    // === 毎ターン判定ロジック（状態管理なし） ===
    
    // 1. 現在地を取得
    FString CurrentLocation = GetCurrentLocation();
    
    // 2. TaskManagerから実行可能タスクを確認
    FString TargetItem = GetCurrentTargetItem();
    
    // 3. 移動中かチェック
    bool bIsMoving = IsTeamMoving();
    
    // 4. ロジック分岐
    if (CurrentLocation == TEXT("base"))
    {
        // 拠点にいる場合
        if (bIsMoving)
        {
            // 移動準備中または移動開始
            return GetMovementStatusText(false); // 移動
        }
        else if (ShouldUnloadItems())
        {
            return TEXT("荷下ろし");
        }
        else if (!TargetItem.IsEmpty())
        {
            // 新しいタスクがある場合
            return GetMovementStatusText(false); // 移動
        }
        else
        {
            return TEXT("待機");
        }
    }
    else
    {
        // 拠点以外にいる場合
        if (bIsMoving)
        {
            // 移動中
            if (TargetItem.IsEmpty())
            {
                return GetMovementStatusText(true); // 帰還
            }
            else
            {
                return GetMovementStatusText(false); // 目的地に向かって移動中
            }
        }
        else
        {
            // 拠点以外で移動していない場合：作業中または帰還待機
            int32 CurrentResourceCount = GetCurrentResourceCount();
            
            // チームが実際にアイテムを持っているかチェック
            bool bHasItems = ShouldUnloadItems();
            
            if (!TargetItem.IsEmpty())
            {
                // タスクがある場合は作業中（採集中）
                if (CurrentResourceCount >= 0)
                {
                    return FString::Printf(TEXT("採集（%d個）"), CurrentResourceCount);
                }
                else
                {
                    return TEXT("採集");
                }
            }
            else if (bHasItems)
            {
                // タスクはないがアイテムを持っている場合
                if (CurrentResourceCount >= 0)
                {
                    return FString::Printf(TEXT("作業完了（%d個）"), CurrentResourceCount);
                }
                else
                {
                    return TEXT("作業完了");
                }
            }
            else
            {
                // タスクもアイテムもない場合は帰還
                return GetMovementStatusText(true); // 帰還
            }
        }
    }
}

FString UC_TeamCard::GetDistanceFromBaseDisplayText() const
{
    if (!IsValidTeamCard())
    {
        return TEXT("0");
    }

    // PlayerControllerからMovementComponentを取得
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            // AC_PlayerControllerにキャスト
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (ULocationMovementComponent* MovementComp = IdlePC->MovementComponent)
                {
                    float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
                    return FString::Printf(TEXT("%.0f"), CurrentDistance);
                }
            }
        }
    }

    // MovementComponentが取得できない場合のフォールバック
    return TEXT("0");
}

int32 UC_TeamCard::GetCurrentResourceCount() const
{
    if (!IsValidTeamCard())
    {
        return -1;
    }

    FTeam TeamData = GetTeamData();
    
    // 採集タスクでない場合は-1を返す
    if (TeamData.AssignedTask != ETaskType::Gathering)
    {
        return -1;
    }
    
    // TaskManagerComponentから目標アイテムIDを取得
    FString TargetItemId;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (UTaskManagerComponent* TaskManagerComp = IdlePC->GetTaskManager())
                {
                    // 現在の採集場所での目標アイテムを取得
                    TargetItemId = TaskManagerComp->GetTargetItemForTeam(TeamIndex, TeamData.GatheringLocationId);
                }
            }
        }
    }
    
    if (TargetItemId.IsEmpty())
    {
        return -1; // 目標アイテムが設定されていない
    }
    
    // チーム内の全キャラクターが持つ目標アイテムの個数を合計
    int32 TotalCount = 0;
    for (AC_IdleCharacter* Member : TeamData.Members)
    {
        if (Member && Member->GetInventoryComponent())
        {
            UInventoryComponent* CharInventory = Member->GetInventoryComponent();
            int32 MemberCount = CharInventory->GetItemCount(TargetItemId);
            TotalCount += MemberCount;
        }
    }
    
    return TotalCount;
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
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateTeamStatusDisplay: Team %d TeamStatusText is NULL"), TeamIndex);
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

void UC_TeamCard::UpdateDistanceFromBaseDisplay()
{
    if (DistanceFromBaseText)
    {
        FString DistanceText = GetDistanceFromBaseDisplayText();
        DistanceFromBaseText->SetText(FText::FromString(DistanceText));
    }
}

// ======== チームタスク作成ボタンハンドラー ========

void UC_TeamCard::OnCreateTaskClicked()
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::OnCreateTaskClicked - Team %d"), TeamIndex);
    
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
    UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Creating task make sheet for Team %d"), TeamIndex);
    
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
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Widget created successfully"));
        
        // チーム情報で初期化
        TaskMakeSheet->InitializeWithTeam(TeamIndex, TeamComponent);
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Widget initialized"));
        
        // 画面に表示
        TaskMakeSheet->AddToViewport();
        
        // UIインタラクション用の入力モード設定
        OwningPlayer->SetInputMode(FInputModeGameAndUI());
        OwningPlayer->bShowMouseCursor = true;
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - TeamTaskMakeSheet added to viewport with input mode"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamCard::ShowTeamTaskMakeSheet - Failed to create TaskMakeSheet widget"));
    }
}

// === 毎ターン判定用ヘルパー関数 ===

FString UC_TeamCard::GetCurrentLocation() const
{
    // MovementComponentから現在地を取得
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (ULocationMovementComponent* MovementComp = IdlePC->MovementComponent)
                {
                    float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
                    
                    // 距離から場所を判定
                    if (CurrentDistance <= 0.0f)
                    {
                        return TEXT("base");
                    }
                    else if (FMath::IsNearlyEqual(CurrentDistance, 100.0f, 5.0f))
                    {
                        return TEXT("plains");
                    }
                    else if (FMath::IsNearlyEqual(CurrentDistance, 200.0f, 5.0f))
                    {
                        return TEXT("forest");
                    }
                    else if (FMath::IsNearlyEqual(CurrentDistance, 500.0f, 5.0f))
                    {
                        return TEXT("swamp");
                    }
                    else if (FMath::IsNearlyEqual(CurrentDistance, 800.0f, 5.0f))
                    {
                        return TEXT("mountain");
                    }
                    else
                    {
                        // 移動中の場合は最寄りの場所を返す
                        if (CurrentDistance < 50.0f) return TEXT("base");
                        else if (CurrentDistance < 150.0f) return TEXT("plains");
                        else if (CurrentDistance < 350.0f) return TEXT("forest");
                        else if (CurrentDistance < 650.0f) return TEXT("swamp");
                        else return TEXT("mountain");
                    }
                }
            }
        }
    }
    
    return TEXT("base"); // デフォルト
}

FString UC_TeamCard::GetCurrentTargetItem() const
{
    // TaskManagerから実行可能タスクを取得
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (UTaskManagerComponent* TaskManager = IdlePC->FindComponentByClass<UTaskManagerComponent>())
                {
                    FString CurrentLocation = GetCurrentLocation();
                    return TaskManager->GetTargetItemForTeam(TeamIndex, CurrentLocation);
                }
            }
        }
    }
    
    return FString(); // タスクなし
}

bool UC_TeamCard::IsTeamMoving() const
{
    // MovementComponentから移動状態を確認
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (ULocationMovementComponent* MovementComp = IdlePC->MovementComponent)
                {
                    EMovementState State = MovementComp->GetMovementState(TeamIndex);
                    
                    // デバッグ：移動状態をログ出力
                    UE_LOG(LogTemp, Warning, TEXT("🎮🔍 TeamCard %d: MovementState = %d"), 
                        TeamIndex, (int32)State);
                    
                    return (State == EMovementState::MovingToDestination || State == EMovementState::MovingToBase);
                }
            }
        }
    }
    
    return false;
}

bool UC_TeamCard::ShouldUnloadItems() const
{
    // チームメンバーがResourceカテゴリアイテムを持っているかチェック
    if (!IsValidTeamCard())
    {
        return false;
    }
    
    FTeam TeamData = GetTeamData();
    
    for (AC_IdleCharacter* Member : TeamData.Members)
    {
        if (IsValid(Member))
        {
            if (UInventoryComponent* MemberInventory = Member->GetInventoryComponent())
            {
                // インベントリにResourceカテゴリのアイテムがあるかチェック
                // 簡易判定：wood, stone, ironなどの基本資源があるかチェック
                TArray<FString> ResourceItems = {TEXT("wood"), TEXT("stone"), TEXT("iron"), TEXT("food"), TEXT("ingredient")};
                
                for (const FString& ResourceItem : ResourceItems)
                {
                    if (MemberInventory->GetItemCount(ResourceItem) > 0)
                    {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

FString UC_TeamCard::GetMovementStatusText(bool bReturning) const
{
    // 移動中の残り時間を計算
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                if (ULocationMovementComponent* MovementComp = IdlePC->MovementComponent)
                {
                    float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
                    int32 CurrentDistanceInt = FMath::RoundToInt(CurrentDistance);
                    
                    // 目標距離を取得
                    int32 TargetDistance = 0;
                    if (bReturning)
                    {
                        TargetDistance = 0; // 拠点
                    }
                    else
                    {
                        // 新しいタスクの目的地を取得
                        FTeam TeamData = GetTeamData();
                        FString GatheringLocationId = TeamData.GatheringLocationId;
                        
                        if (GatheringLocationId == TEXT("plains")) TargetDistance = 100;
                        else if (GatheringLocationId == TEXT("forest")) TargetDistance = 200;
                        else if (GatheringLocationId == TEXT("mountain")) TargetDistance = 800;
                        else if (GatheringLocationId == TEXT("swamp")) TargetDistance = 500;
                        else TargetDistance = 100; // デフォルト
                    }
                    
                    int32 RemainingDistance = FMath::Abs(TargetDistance - CurrentDistanceInt);
                    
                    // MovementComponentから実際の移動情報を取得
                    FMovementInfo MovementInfo = MovementComp->GetMovementInfo(TeamIndex);
                    
                    // State で移動中かチェック
                    if (MovementInfo.State == EMovementState::MovingToDestination || 
                        MovementInfo.State == EMovementState::MovingToBase)
                    {
                        // 実際の残り時間を使用
                        int32 RemainingSeconds = FMath::CeilToInt(MovementInfo.RemainingTime);
                        int32 Minutes = RemainingSeconds / 60;
                        int32 Seconds = RemainingSeconds % 60;
                        
                        FString TimeText = FString::Printf(TEXT("%02d：%02d"), Minutes, Seconds);
                        FString MovementText = bReturning ? TEXT("帰還") : TEXT("移動");
                        
                        UE_LOG(LogTemp, Warning, TEXT("🎮⏱️ TeamCard %d: Actual remaining time = %.1fs (displayed as %s)"), 
                            TeamIndex, MovementInfo.RemainingTime, *TimeText);
                        
                        return FString::Printf(TEXT("%s（残り%s）"), *MovementText, *TimeText);
                    }
                    else if (RemainingDistance > 0)
                    {
                        // フォールバック：簡易計算
                        int32 MovementSpeed = 30;
                        int32 RemainingTurns = FMath::CeilToInt(static_cast<float>(RemainingDistance) / MovementSpeed);
                        int32 RemainingSeconds = RemainingTurns;
                        
                        int32 Minutes = RemainingSeconds / 60;
                        int32 Seconds = RemainingSeconds % 60;
                        
                        FString TimeText = FString::Printf(TEXT("%02d：%02d"), Minutes, Seconds);
                        FString MovementText = bReturning ? TEXT("帰還") : TEXT("移動");
                        
                        return FString::Printf(TEXT("%s（残り%s）"), *MovementText, *TimeText);
                    }
                    else
                    {
                        return bReturning ? TEXT("帰還") : TEXT("移動");
                    }
                }
            }
        }
    }
    
    return bReturning ? TEXT("帰還中") : TEXT("移動中");
}