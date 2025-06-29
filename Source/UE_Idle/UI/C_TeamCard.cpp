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
        UE_LOG(LogTemp, VeryVerbose, TEXT("UC_TeamCard::OnTeamActionStateChanged - Team %d"), TeamIndex);
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

    FTeam TeamData = GetTeamData();
    
    // 移動中の場合は残り時間を表示
    if (TeamData.ActionState == ETeamActionState::Moving)
    {
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
                        // ターンベース設計：残り距離から移動時間を計算
                        float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
                        int32 CurrentDistanceInt = FMath::RoundToInt(CurrentDistance);
                        
                        // 目標距離を計算
                        int32 TargetDistance = 0;
                        if (TeamData.GatheringLocationId == TEXT("base"))
                        {
                            TargetDistance = 0;
                        }
                        else
                        {
                            // 既知の距離
                            if (TeamData.GatheringLocationId == TEXT("plains")) TargetDistance = 100;
                            else if (TeamData.GatheringLocationId == TEXT("forest")) TargetDistance = 200;
                            else if (TeamData.GatheringLocationId == TEXT("mountain")) TargetDistance = 800;
                            else if (TeamData.GatheringLocationId == TEXT("swamp")) TargetDistance = 500;
                            else TargetDistance = 100; // デフォルト
                        }
                        
                        int32 RemainingDistance = FMath::Abs(TargetDistance - CurrentDistanceInt);
                        
                        // 目標地で移動種別を判定
                        FString MovementText = (TeamData.GatheringLocationId == TEXT("base")) ? TEXT("帰還中") : TEXT("移動中");
                        
                        if (RemainingDistance > 0)
                        {
                            // 移動速度30m/turnでターン数を計算 → 秒数に変換（1ターン=1秒と仮定）
                            int32 MovementSpeed = 30; // m/turn
                            int32 RemainingTurns = FMath::CeilToInt(static_cast<float>(RemainingDistance) / MovementSpeed);
                            int32 RemainingSeconds = RemainingTurns; // 1ターン=1秒
                            
                            int32 Minutes = RemainingSeconds / 60;
                            int32 Seconds = RemainingSeconds % 60;
                            
                            FString TimeText = FString::Printf(TEXT("%02d：%02d"), Minutes, Seconds);
                            FString Result = FString::Printf(TEXT("%s（残り%s）"), *MovementText, *TimeText);
                            return Result;
                        }
                        else
                        {
                            return MovementText;
                        }
                    }
                }
            }
        }
        
        // MovementComponentが取得できない場合のフォールバック
        return TEXT("移動中");
    }
    
    // 作業中の場合は目標資源の個数を表示
    if (TeamData.ActionState == ETeamActionState::Working)
    {
        int32 CurrentResourceCount = GetCurrentResourceCount();
        if (CurrentResourceCount >= 0)
        {
            return FString::Printf(TEXT("作業中（%d個）"), CurrentResourceCount);
        }
    }
    
    // その他の状態は通常の状態表示
    return TeamData.GetActionStateDisplayName();
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
            TotalCount += CharInventory->GetItemCount(TargetItemId);
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