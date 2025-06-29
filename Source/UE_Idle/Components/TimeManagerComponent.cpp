#include "TimeManagerComponent.h"
#include "TaskManagerComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/GatheringComponent.h"
#include "../Components/LocationMovementComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Managers/LocationDataTableManager.h"
#include "../Managers/ItemDataTableManager.h"
#include "../Types/ItemTypes.h"
#include "../Actor/C_IdleCharacter.h"
#include "../C_PlayerController.h"
#include "../Interfaces/PlayerControllerInterface.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UTimeManagerComponent::UTimeManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    bTimeSystemActive = false;
    bProcessingTimeUpdate = false;
    TimeUpdateInterval = 1.0f;
    bFastProcessingMode = false;
    IdleThreshold = 300.0f;
    bEnableDefensiveProgramming = true;
    MaxProcessingRetries = 3;
    CurrentGameTime = 0.0f;
    LastProcessTime = 0.0f;
    
    TotalUpdatesProcessed = 0;
    TaskSwitchesPerformed = 0;
    TotalProcessingTime = 0.0f;
}

void UTimeManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    CurrentGameTime = GetWorld()->GetTimeSeconds();
    LastProcessTime = CurrentGameTime;
    
    UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: BeginPlay - Initialized at time %.2f"), CurrentGameTime);
}

void UTimeManagerComponent::BeginDestroy()
{
    // システム停止
    StopTimeSystem();
    
    // 参照をクリア
    TaskManager = nullptr;
    TeamComponents.Empty();
    PendingTaskSwitches.Empty();
    
    // 処理中フラグをクリア
    bProcessingTimeUpdate = false;
    
    UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: BeginDestroy - Cleaned up"));
    
    Super::BeginDestroy();
}

// === 時間システム制御 ===

void UTimeManagerComponent::StartTimeSystem()
{
    if (bTimeSystemActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartTimeSystem: Time system is already active"));
        return;
    }

    if (!AreReferencesValid())
    {
        LogTimeError(TEXT("StartTimeSystem: Cannot start - missing component references"));
        UE_LOG(LogTemp, Error, TEXT("StartTimeSystem: TaskManager=%s, TeamComponents=%d, GatheringComponent=%s"), 
            TaskManager ? TEXT("Valid") : TEXT("NULL"), 
            TeamComponents.Num(), 
            GatheringComponent ? TEXT("Valid") : TEXT("NULL"));
        return;
    }

    bTimeSystemActive = true;
    SetupTimer();
    
    LogTimeOperation(TEXT("Started"));
    OnTimeSystemStarted.Broadcast();
}

void UTimeManagerComponent::StopTimeSystem()
{
    if (!bTimeSystemActive)
    {
        return;
    }

    bTimeSystemActive = false;
    ClearTimer();
    
    // 進行中の処理をクリア
    bProcessingTimeUpdate = false;
    PendingTaskSwitches.Empty();
    
    LogTimeOperation(TEXT("Stopped"));
    OnTimeSystemStopped.Broadcast();
}

void UTimeManagerComponent::PauseTimeSystem()
{
    if (bTimeSystemActive)
    {
        ClearTimer();
        LogTimeOperation(TEXT("Paused"));
    }
}

void UTimeManagerComponent::ResumeTimeSystem()
{
    if (bTimeSystemActive && !GetWorld()->GetTimerManager().IsTimerActive(TimeUpdateTimerHandle))
    {
        SetupTimer();
        LogTimeOperation(TEXT("Resumed"));
    }
}

// === 時間進行処理 ===

void UTimeManagerComponent::ProcessTimeUpdate()
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("TimeManager Update: Active=%s, Processing teams..."), bTimeSystemActive ? TEXT("Yes") : TEXT("No"));
    
    if (!bTimeSystemActive)
    {
        return;
    }

    // 重複実行防止
    if (bProcessingTimeUpdate)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeManager: Skipping update, already processing"));
        return;
    }

    float StartTime = GetWorld()->GetTimeSeconds();
    bProcessingTimeUpdate = true;

    // 有効性チェック付き処理
    if (bEnableDefensiveProgramming)
    {
        ProcessTimeUpdateSafe();
    }
    else
    {
        // 1. 全体タスクの進行チェック
        ProcessGlobalTasks();
        
        // 2. 全チームのタスク進行チェック
        ProcessTeamTasks();
        
        // 3. リソース条件変化の監視
        CheckResourceConditions();
        
        // 4. タスク完了・切り替えの判定
        ProcessTaskSwitching();
        
        // 5. 時間イベント発信
        BroadcastTimeEvents();
    }

    // 統計更新
    TotalUpdatesProcessed++;
    TotalProcessingTime += GetWorld()->GetTimeSeconds() - StartTime;
    CurrentGameTime = GetWorld()->GetTimeSeconds();

    bProcessingTimeUpdate = false;
}

void UTimeManagerComponent::ProcessGlobalTasks()
{
    if (!IsValid(TaskManager))
    {
        return;
    }

    const TArray<FGlobalTask>& GlobalTasks = TaskManager->GetGlobalTasks();
    
    for (const FGlobalTask& Task : GlobalTasks)
    {
        if (!Task.bIsCompleted)
        {
            // タスクの進行状況をチェック（将来的に詳細実装）
            UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGlobalTasks: Monitoring task %s (Progress: %d/%d)"), 
                   *Task.TaskId, Task.CurrentProgress, Task.TargetQuantity);
        }
    }
}

void UTimeManagerComponent::ProcessTeamTasks()
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessTeamTasks: Processing %d teams"), TeamComponents.Num());
    
    for (int32 TeamIndex = 0; TeamIndex < TeamComponents.Num(); TeamIndex++)
    {
        if (bEnableDefensiveProgramming)
        {
            ProcessTeamTaskSafe(TeamIndex);
        }
        else
        {
            if (IsValid(TeamComponents[TeamIndex]))
            {
                FTeam Team = TeamComponents[TeamIndex]->GetTeam(TeamIndex);
                
                UE_LOG(LogTemp, Log, TEXT("ProcessTeamTasks: Team %d - State: %s, Task: %s, GatheringLocation: %s"), 
                    TeamIndex, 
                    *UEnum::GetValueAsString(Team.ActionState),
                    *UTaskTypeUtils::GetTaskTypeDisplayName(Team.AssignedTask),
                    *Team.GatheringLocationId);
                
                switch (Team.ActionState)
                {
                    case ETeamActionState::Idle:
                    case ETeamActionState::Working:
                        ProcessNormalTaskSafe(TeamIndex);
                        break;
                        
                    case ETeamActionState::Moving:
                        // 移動中も処理を継続（MovementComponentで移動処理）
                        UE_LOG(LogTemp, Warning, TEXT("ProcessTeamTasks: Team %d is moving, continuing task processing"), TeamIndex);
                        ProcessNormalTaskSafe(TeamIndex);
                        break;
                        
                    case ETeamActionState::InCombat:
                    case ETeamActionState::Locked:
                        MonitorLockedAction(TeamIndex);
                        break;
                }
            }
        }
    }
}

void UTimeManagerComponent::CheckResourceConditions()
{
    if (!IsValid(TaskManager))
    {
        return;
    }

    // 全チームの現在のタスクをチェック
    for (int32 TeamIndex = 0; TeamIndex < TeamComponents.Num(); TeamIndex++)
    {
        if (!IsValidTeam(TeamIndex))
        {
            continue;
        }

        UTeamComponent* TeamComp = TeamComponents[TeamIndex];
        FTeam Team = TeamComp->GetTeam(TeamIndex);
        
        if (Team.ActionState == ETeamActionState::Working)
        {
            ETaskType CurrentTaskType = Team.AssignedTask;
            
            // 「全て」モードの場合、リソース条件変化で即座に切り替え
            if (CurrentTaskType == ETaskType::All)
            {
                FGlobalTask AvailableTask = TaskManager->GetNextAvailableTask(TeamIndex);
                
                // 現在実行中のタスクと異なる場合は切り替え
                if (!AvailableTask.TaskId.IsEmpty())
                {
                    // タスク切り替えをキューに追加
                    FDelayedTaskSwitch DelayedSwitch;
                    DelayedSwitch.TeamIndex = TeamIndex;
                    DelayedSwitch.SwitchType = ETaskSwitchType::ResourceChange;
                    DelayedSwitch.Timestamp = CurrentGameTime;
                    
                    PendingTaskSwitches.Add(DelayedSwitch);
                }
            }
        }
    }
}

void UTimeManagerComponent::ProcessTaskSwitching()
{
    ProcessPendingTaskSwitches();
}

void UTimeManagerComponent::BroadcastTimeEvents()
{
    float ElapsedTime = CurrentGameTime - LastProcessTime;
    
    // 1時間経過チェック（3600秒 = 1時間、デバッグモードでは短縮）
    float HourThreshold = bFastProcessingMode ? 60.0f : 3600.0f;
    
    if (ElapsedTime >= HourThreshold)
    {
        float CurrentHour = CurrentGameTime / HourThreshold;
        OnHourPassed.Broadcast(CurrentHour);
        LastProcessTime = CurrentGameTime;
        
        UE_LOG(LogTemp, Log, TEXT("BroadcastTimeEvents: Hour passed - Game Hour %.2f"), CurrentHour);
    }
}

// === 個別タスク処理 ===

void UTimeManagerComponent::ProcessAllModeTask(int32 TeamIndex)
{
    if (!IsValid(TaskManager) || !IsValidTeam(TeamIndex))
    {
        return;
    }

    // 優先度順に全体タスクをチェック
    FGlobalTask NextTask = TaskManager->GetNextAvailableTask(TeamIndex);
    
    if (!NextTask.TaskId.IsEmpty())
    {
        StartTaskExecution(TeamIndex, NextTask);
    }
    else
    {
        SetTeamToIdle(TeamIndex);
    }
}

void UTimeManagerComponent::ProcessSpecificTask(int32 TeamIndex, ETaskType TaskType)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    // 特定タスクタイプ別の処理
    switch (TaskType)
    {
        case ETaskType::Idle:
        {
            // アイドル状態 - 何もしない
            UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessSpecificTask: Team %d is idle"), TeamIndex);
            break;
        }
        case ETaskType::Gathering:
        {
            ProcessGatheringTask(TeamIndex);
            break;
        }
        case ETaskType::Adventure:
        case ETaskType::Cooking:
        case ETaskType::Construction:
        case ETaskType::Crafting:
        {
            // 他のタスクタイプは将来的に実装
            UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessSpecificTask: Team %d processing %s (not yet implemented)"), 
                   TeamIndex, *UTaskTypeUtils::GetTaskTypeDisplayName(TaskType));
            break;
        }
        default:
        {
            UE_LOG(LogTemp, Warning, TEXT("ProcessSpecificTask: Unknown task type for team %d"), TeamIndex);
            break;
        }
    }
}

void UTimeManagerComponent::ProcessGatheringTask(int32 TeamIndex)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringTask: Processing team %d with turn-based logic"), TeamIndex);
    
    if (!IsValidTeam(TeamIndex) || !TaskManager || !MovementComponent)
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);

    // 1. 現在位置を取得
    FString CurrentLocation = GetCurrentLocation(TeamIndex);
    
    // 2. 拠点にいる場合は自動荷下ろしと次の目標設定
    if (CurrentLocation == TEXT("base"))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessGatheringTask: Team %d at base, checking for auto-unload and next task"), TeamIndex);
        
        // 自動荷下ろし処理（簡易実装）
        AutoUnloadResourceItems(TeamIndex);
        
        // 次の採集場所を探す
        FString NextTargetItemId;
        FString NextTargetLocation;
        
        TArray<FString> LocationsToCheck = {TEXT("plains"), TEXT("forest"), TEXT("swamp"), TEXT("mountain")};
        for (const FString& LocationId : LocationsToCheck)
        {
            FString TestItemId = TaskManager->GetTargetItemForTeam(TeamIndex, LocationId);
            if (!TestItemId.IsEmpty())
            {
                NextTargetItemId = TestItemId;
                NextTargetLocation = LocationId;
                break;
            }
        }
        
        if (NextTargetItemId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ NO TASKS: Team %d going idle"), TeamIndex);
            TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Idle);
            return;
        }
        
        // 新しい採集場所を設定
        TeamComp->SetTeamGatheringLocation(TeamIndex, NextTargetLocation);
        UE_LOG(LogTemp, Warning, TEXT("→ NEW TARGET: Team %d going to %s for %s"), 
            TeamIndex, *NextTargetLocation, *NextTargetItemId);
        
        // 移動開始
        ExecuteMovementStep(TeamIndex, NextTargetLocation);
        TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
        return;
    }
    
    // 3. 拠点以外の場所での処理
    FString TargetItemId = TaskManager->GetTargetItemForTeam(TeamIndex, Team.GatheringLocationId);
    FString TargetLocation = Team.GatheringLocationId;
    
    // 3.5. タスク完了チェック（目標数量達成）
    if (!TargetItemId.IsEmpty())
    {
        FGlobalTask ActiveTask = TaskManager->FindActiveGatheringTask(TargetItemId);
        if (!ActiveTask.TaskId.IsEmpty())
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringTask: Checking task completion for %s"), *ActiveTask.TaskId);
            if (TaskManager->IsTaskCompleted(ActiveTask.TaskId))
            {
                UE_LOG(LogTemp, Warning, TEXT("✅ TASK DONE: %s - Team %d returning home"), 
                    *ActiveTask.TaskId, TeamIndex);
                ExecuteMovementStep(TeamIndex, TEXT("base"));
                TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
                return;
            }
            UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringTask: Task %s not yet completed"), *ActiveTask.TaskId);
        }
    }
    
    // 4. アイテム満タン判定（拠点以外で実行）
    if (!TargetItemId.IsEmpty() && !CanTeamCarryNextGather(TeamIndex, TargetItemId))
    {
        UE_LOG(LogTemp, Warning, TEXT("📦 FULL: Team %d returning to base"), TeamIndex);
        // 拠点を採集場所に設定して帰還
        TeamComp->SetTeamGatheringLocation(TeamIndex, TEXT("base"));
        ExecuteMovementStep(TeamIndex, TEXT("base"));
        TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
        return;
    }
    
    // 5. タスク存在判定
    if (TargetItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("❓ NO TARGET: Team %d at %s - returning"), 
            TeamIndex, *Team.GatheringLocationId);
        ExecuteMovementStep(TeamIndex, TEXT("base"));
        TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
        return;
    }
    
    // 6. 位置判定と実行
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringTask: Team %d at %s, target %s for %s"), 
        TeamIndex, *CurrentLocation, *TargetLocation, *TargetItemId);
    
    if (CurrentLocation == TargetLocation)
    {
        // 目標地にいる → 採集実行
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringTask: Team %d executing gathering for %s at %s"), 
            TeamIndex, *TargetItemId, *CurrentLocation);
        ExecuteGathering(TeamIndex, TargetItemId);
        TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Working);
    }
    else
    {
        // 目標地にいない → 1ターン分移動実行
        UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessGatheringTask: Team %d executing movement step from %s to %s"), 
            TeamIndex, *CurrentLocation, *TargetLocation);
        ExecuteMovementStep(TeamIndex, TargetLocation);
        TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Moving);
    }
}

void UTimeManagerComponent::MonitorLockedAction(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    if (Team.ActionState == ETeamActionState::InCombat)
    {
        MonitorCombatSafe(TeamIndex);
    }
    else if (Team.ActionState == ETeamActionState::Locked)
    {
        // その他の中断不可アクションの監視
        float RemainingTime = Team.GetRemainingActionTime(CurrentGameTime);
        
        if (RemainingTime <= 0.0f)
        {
            UE_LOG(LogTemp, Log, TEXT("MonitorLockedAction: Team %d action completed"), TeamIndex);
            SetTeamToIdle(TeamIndex);
        }
    }
}

void UTimeManagerComponent::ProcessPostCombatTask(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    ETaskType TaskMode = TeamComp->GetTeam(TeamIndex).AssignedTask;
    
    if (TaskMode == ETaskType::All)
    {
        // 「全て」モード - 戦闘終了後に優先度再チェック
        ProcessAllModeTask(TeamIndex);
    }
    else if (TaskMode == ETaskType::Adventure)
    {
        // 冒険モード - 継続判定
        // 将来的に詳細実装
        UE_LOG(LogTemp, Log, TEXT("ProcessPostCombatTask: Adventure mode post-combat processing for team %d"), TeamIndex);
    }
    
    TaskSwitchesPerformed++;
}

// === タスク切り替えロジック ===

bool UTimeManagerComponent::InterruptAndSwitchTask(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return false;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    if (!Team.CanInterruptAction())
    {
        return false; // 中断不可
    }

    ETaskType CurrentMode = Team.AssignedTask;
    
    if (CurrentMode == ETaskType::All)
    {
        // 「全て」モードなら次の実行可能タスクを検索
        FGlobalTask NextTask = TaskManager->GetNextAvailableTask(TeamIndex);
        if (!NextTask.TaskId.IsEmpty())
        {
            StartTaskExecution(TeamIndex, NextTask);
            return true;
        }
    }
    else
    {
        // 特定タスクモードなら優先順位の次のタスクへ
        // 将来的に詳細実装
    }
    
    // 切り替え先がない場合は待機
    SetTeamToIdle(TeamIndex);
    return false;
}

void UTimeManagerComponent::StartTaskExecution(int32 TeamIndex, const FGlobalTask& Task)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    
    // チームの状態を作業中に設定
    FTeam& Team = TeamComp->GetTeams()[TeamIndex];
    Team.ActionState = ETeamActionState::Working;
    Team.ActionStartTime = CurrentGameTime;
    Team.EstimatedCompletionTime = 3600.0f; // 1時間（例）
    
    UE_LOG(LogTemp, Log, TEXT("StartTaskExecution: Team %d started task %s"), TeamIndex, *Task.TaskId);
    
    TaskSwitchesPerformed++;
}

void UTimeManagerComponent::SetTeamToIdle(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam& Team = TeamComp->GetTeams()[TeamIndex];
    Team.ActionState = ETeamActionState::Idle;
    Team.ActionStartTime = 0.0f;
    Team.EstimatedCompletionTime = 0.0f;
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("SetTeamToIdle: Team %d set to idle"), TeamIndex);
}

void UTimeManagerComponent::ProcessPendingTaskSwitches()
{
    if (PendingTaskSwitches.Num() == 0)
    {
        return;
    }

    TArray<FDelayedTaskSwitch> SwitchesToProcess = PendingTaskSwitches;
    PendingTaskSwitches.Empty();

    for (const FDelayedTaskSwitch& DelayedSwitch : SwitchesToProcess)
    {
        if (DelayedSwitch.IsValid() && IsValidTeam(DelayedSwitch.TeamIndex))
        {
            switch (DelayedSwitch.SwitchType)
            {
                case ETaskSwitchType::PostCombat:
                    ProcessPostCombatTask(DelayedSwitch.TeamIndex);
                    break;
                    
                case ETaskSwitchType::ResourceChange:
                    InterruptAndSwitchTask(DelayedSwitch.TeamIndex);
                    break;
                    
                case ETaskSwitchType::Forced:
                    SetTeamToIdle(DelayedSwitch.TeamIndex);
                    break;
                    
                default:
                    break;
            }
        }
    }
}

// === 戦闘終了イベントハンドラー ===

void UTimeManagerComponent::OnCombatEndedHandler(int32 TeamIndex)
{
    // 即座に処理せず、次フレームで安全に処理
    FDelayedTaskSwitch DelayedSwitch;
    DelayedSwitch.TeamIndex = TeamIndex;
    DelayedSwitch.SwitchType = ETaskSwitchType::PostCombat;
    DelayedSwitch.Timestamp = CurrentGameTime;
    
    PendingTaskSwitches.Add(DelayedSwitch);
    
    UE_LOG(LogTemp, Log, TEXT("OnCombatEndedHandler: Queued post-combat processing for team %d"), TeamIndex);
}

// === コンポーネント登録 ===

void UTimeManagerComponent::RegisterTaskManager(UTaskManagerComponent* InTaskManager)
{
    if (IsValid(InTaskManager))
    {
        TaskManager = InTaskManager;
        UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: TaskManager registered"));
    }
    else
    {
        LogTimeError(TEXT("RegisterTaskManager: Invalid TaskManager component"));
    }
}

void UTimeManagerComponent::RegisterTeamComponent(UTeamComponent* InTeamComponent)
{
    if (IsValid(InTeamComponent))
    {
        TeamComponents.AddUnique(InTeamComponent);
        UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: TeamComponent registered (Total: %d)"), TeamComponents.Num());
    }
    else
    {
        LogTimeError(TEXT("RegisterTeamComponent: Invalid TeamComponent"));
    }
}

void UTimeManagerComponent::RegisterGatheringComponent(UGatheringComponent* InGatheringComponent)
{
    if (IsValid(InGatheringComponent))
    {
        GatheringComponent = InGatheringComponent;
        UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: GatheringComponent registered successfully"));
    }
    else
    {
        LogTimeError(TEXT("RegisterGatheringComponent: Invalid GatheringComponent"));
    }
}

void UTimeManagerComponent::RegisterMovementComponent(ULocationMovementComponent* InMovementComponent)
{
    if (IsValid(InMovementComponent))
    {
        MovementComponent = InMovementComponent;
        
        // 移動完了イベントを監視
        MovementComponent->OnMovementCompleted.AddDynamic(this, &UTimeManagerComponent::OnMovementCompletedHandler);
        
        UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: MovementComponent registered successfully"));
    }
    else
    {
        LogTimeError(TEXT("RegisterMovementComponent: Invalid MovementComponent"));
    }
}

void UTimeManagerComponent::OnMovementCompletedHandler(int32 TeamIndex, const FString& ArrivedLocation)
{
    UE_LOG(LogTemp, Warning, TEXT("TimeManagerComponent: Team %d completed movement to %s"), TeamIndex, *ArrivedLocation);
    
    // 新しいシンプルなロジックでは移動完了時の特別な処理は不要
    // 次のターンで自動的に現在地が正しく判定され、採集が開始される
}

void UTimeManagerComponent::ClearRegisteredComponents()
{
    TaskManager = nullptr;
    TeamComponents.Empty();
    GatheringComponent = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("TimeManagerComponent: All registered components cleared"));
}

// === 時間関連ユーティリティ ===

float UTimeManagerComponent::GetElapsedTime(float StartTime) const
{
    return FMath::Max(0.0f, CurrentGameTime - StartTime);
}

void UTimeManagerComponent::AdvanceTime(float Hours)
{
    CurrentGameTime += Hours * 3600.0f; // 時間を秒に変換
    UE_LOG(LogTemp, Log, TEXT("AdvanceTime: Advanced %.2f hours, current time: %.2f"), Hours, CurrentGameTime);
}

// === 安全性確保機能 ===

void UTimeManagerComponent::ProcessTimeUpdateSafe()
{
    // null チェック付きループ処理
    for (int32 i = TeamComponents.Num() - 1; i >= 0; i--)
    {
        if (!IsValid(TeamComponents[i]))
        {
            // 無効なコンポーネントを削除
            TeamComponents.RemoveAt(i);
            UE_LOG(LogTemp, Warning, TEXT("ProcessTimeUpdateSafe: Removed invalid TeamComponent at index %d"), i);
            continue;
        }
        
        ProcessTeamTaskSafe(i);
    }
    
    // その他の安全な処理
    if (IsValid(TaskManager))
    {
        ProcessGlobalTasks();
    }
    
    CheckResourceConditions();
    ProcessTaskSwitching();
    BroadcastTimeEvents();
}

void UTimeManagerComponent::ProcessTeamTaskSafe(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    
    // 二重チェック
    if (!IsValid(TeamComp))
    {
        return;
    }

    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    // 安全な状態処理
    switch (Team.ActionState)
    {
        case ETeamActionState::InCombat:
            MonitorCombatSafe(TeamIndex);
            break;
        case ETeamActionState::Working:
        case ETeamActionState::Idle:
            ProcessNormalTaskSafe(TeamIndex);
            break;
        case ETeamActionState::Moving:
            // 移動中も処理を継続（MovementComponentで移動処理）
            UE_LOG(LogTemp, Warning, TEXT("ProcessTeamTaskSafe: Team %d is moving, continuing task processing"), TeamIndex);
            ProcessNormalTaskSafe(TeamIndex);
            break;
        case ETeamActionState::Locked:
            MonitorLockedAction(TeamIndex);
            break;
    }
}

void UTimeManagerComponent::MonitorCombatSafe(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    if (Team.IsCombatFinished())
    {
        // 戦闘終了検出 - 遅延処理でタスク切り替え
        OnCombatEndedHandler(TeamIndex);
    }
    else
    {
        // 戦闘継続中 - 監視のみ
        UE_LOG(LogTemp, VeryVerbose, TEXT("MonitorCombatSafe: Team %d still in combat"), TeamIndex);
    }
}

void UTimeManagerComponent::ProcessNormalTaskSafe(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }

    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessNormalTaskSafe: Team %d - AssignedTask: %s"), 
        TeamIndex, *UTaskTypeUtils::GetTaskTypeDisplayName(Team.AssignedTask));
    
    if (Team.AssignedTask == ETaskType::All)
    {
        ProcessAllModeTask(TeamIndex);
    }
    else
    {
        ProcessSpecificTask(TeamIndex, Team.AssignedTask);
    }
}

// === 検証・デバッグ ===

bool UTimeManagerComponent::AreReferencesValid() const
{
    return IsValid(TaskManager) && TeamComponents.Num() > 0 && IsValid(GatheringComponent);
}

FString UTimeManagerComponent::GetProcessingStats() const
{
    return FString::Printf(TEXT("Updates: %d, Switches: %d, Processing Time: %.2fs"), 
                          TotalUpdatesProcessed, TaskSwitchesPerformed, TotalProcessingTime);
}

void UTimeManagerComponent::PrintDebugInfo() const
{
    UE_LOG(LogTemp, Log, TEXT("=== TimeManagerComponent Debug Info ==="));
    UE_LOG(LogTemp, Log, TEXT("Active: %s"), bTimeSystemActive ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Processing: %s"), bProcessingTimeUpdate ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Current Time: %.2f"), CurrentGameTime);
    UE_LOG(LogTemp, Log, TEXT("Team Components: %d"), TeamComponents.Num());
    UE_LOG(LogTemp, Log, TEXT("Pending Switches: %d"), PendingTaskSwitches.Num());
    UE_LOG(LogTemp, Log, TEXT("Stats: %s"), *GetProcessingStats());
    UE_LOG(LogTemp, Log, TEXT("References Valid: %s"), AreReferencesValid() ? TEXT("Yes") : TEXT("No"));
}

// === 内部ヘルパー関数 ===

void UTimeManagerComponent::SetupTimer()
{
    if (UWorld* World = GetWorld())
    {
        float Interval = bFastProcessingMode ? TimeUpdateInterval * 0.1f : TimeUpdateInterval;
        
        World->GetTimerManager().SetTimer(
            TimeUpdateTimerHandle,
            this,
            &UTimeManagerComponent::ProcessTimeUpdate,
            Interval,
            true  // ループ
        );
        
        UE_LOG(LogTemp, Log, TEXT("SetupTimer: Timer set with interval %.2fs"), Interval);
    }
}

void UTimeManagerComponent::ClearTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimeUpdateTimerHandle);
    }
}

bool UTimeManagerComponent::CheckResourceRequirements(int32 TeamIndex, const FTeamTask& Task)
{
    if (!IsValid(TaskManager))
    {
        return false;
    }

    return TaskManager->CheckResourceRequirements(Task);
}

bool UTimeManagerComponent::IsValidTeam(int32 TeamIndex) const
{
    return TeamIndex >= 0 && 
           TeamIndex < TeamComponents.Num() && 
           IsValid(TeamComponents[TeamIndex]);
}

void UTimeManagerComponent::LogTimeOperation(const FString& Operation) const
{
    UE_LOG(LogTemp, Log, TEXT("TimeManager %s: Current time %.2f, Interval %.2fs"), 
           *Operation, CurrentGameTime, TimeUpdateInterval);
}

void UTimeManagerComponent::LogTimeError(const FString& ErrorMessage) const
{
    UE_LOG(LogTemp, Error, TEXT("TimeManagerComponent Error: %s"), *ErrorMessage);
}

// === シンプルなターンベースロジック ===

FString UTimeManagerComponent::GetCurrentLocation(int32 TeamIndex) const
{
    if (!MovementComponent)
    {
        return TEXT("base"); // MovementComponentがない場合は拠点
    }
    
    int32 CurrentDistance = FMath::RoundToInt(MovementComponent->GetCurrentDistanceFromBase(TeamIndex));
    
    if (CurrentDistance == 0)
    {
        return TEXT("base");
    }
    
    // LocationData.csvから全場所をチェックして正確マッチング
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>())
        {
            TArray<FString> AllLocationIds = {TEXT("plains"), TEXT("forest"), TEXT("swamp"), TEXT("mountain")};
            
            for (const FString& LocationId : AllLocationIds)
            {
                int32 LocationDistance = GetLocationDistance(LocationId);
                if (CurrentDistance == LocationDistance)
                {
                    UE_LOG(LogTemp, VeryVerbose, TEXT("GetCurrentLocation: Team %d at %s (distance=%dm)"), 
                        TeamIndex, *LocationId, CurrentDistance);
                    return LocationId;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("GetCurrentLocation: Team %d travelling (distance=%dm)"), 
        TeamIndex, CurrentDistance);
    return TEXT("travelling"); // 移動中
}

bool UTimeManagerComponent::CanExecuteGatheringAt(const FString& ItemId, const FString& Location) const
{
    if (!TaskManager || Location.IsEmpty() || ItemId.IsEmpty())
    {
        return false;
    }
    
    // 拠点では採集不可
    if (Location == TEXT("base"))
    {
        return false;
    }
    
    // LocationManagerを使用して場所でアイテムが採集可能かチェック
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>())
        {
            FLocationDataRow LocationData;
            if (LocationManager->GetLocationData(Location, LocationData))
            {
                TArray<FGatherableItemInfo> GatherableItems = LocationData.ParseGatherableItemsList();
                for (const FGatherableItemInfo& ItemInfo : GatherableItems)
                {
                    if (ItemInfo.ItemId == ItemId)
                    {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

FString UTimeManagerComponent::FindLocationForItem(const FString& ItemId) const
{
    if (!TaskManager || ItemId.IsEmpty())
    {
        return TEXT("");
    }
    
    // LocationManagerを使用してアイテムが採集可能な場所を検索
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>())
        {
            // 全ての場所をチェック（今後は効率化可能）
            // TODO: アイテム→場所のマッピングをキャッシュ
            TArray<FString> AllLocations = { TEXT("plains"), TEXT("forest"), TEXT("mountain"), TEXT("swamp") };
            
            for (const FString& Location : AllLocations)
            {
                if (CanExecuteGatheringAt(ItemId, Location))
                {
                    return Location;
                }
            }
        }
    }
    
    return TEXT("");
}

void UTimeManagerComponent::ExecuteGathering(int32 TeamIndex, const FString& ItemId)
{
    if (!IsValidTeam(TeamIndex) || ItemId.IsEmpty())
    {
        return;
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ExecuteGathering: Team %d gathering %s"), TeamIndex, *ItemId);
    
    // チーム状態を作業中に設定
    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Working);
    
    // シンプルな採集処理：チームのGatheringPowerに基づいてアイテムを直接追加
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    float TeamGatheringPower = 0.0f;
    
    // チームの総GatheringPowerを計算
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (Member && Member->GetStatusComponent())
        {
            TeamGatheringPower += Member->GetStatusComponent()->GetGatheringPower();
        }
    }
    
    // 基本採取量計算（1tick あたり）
    float BaseGatherRate = TeamGatheringPower / 40.0f; // 効率係数
    
    int32 ItemsGathered = 0;
    if (BaseGatherRate >= 1.0f)
    {
        // 1以上なら整数部分を確定獲得
        ItemsGathered = FMath::FloorToInt(BaseGatherRate);
        // 小数部分は確率で追加1個
        float ChanceForExtra = BaseGatherRate - ItemsGathered;
        if (FMath::FRand() < ChanceForExtra)
        {
            ItemsGathered++;
        }
    }
    else
    {
        // 1未満なら確率判定
        if (FMath::FRand() < BaseGatherRate)
        {
            ItemsGathered = 1;
        }
    }
    
    // タスクの目標数量をチェック（簡易版 - 後で詳細実装）
    // 現在は採集を続けるが、将来的にTaskManagerでタスクの完了をチェック
    
    // アイテムをチームのキャラクターインベントリに追加
    if (ItemsGathered > 0)
    {
        // チームメンバーに分配
        int32 RemainingItems = ItemsGathered;
        for (AC_IdleCharacter* Member : Team.Members)
        {
            if (Member && Member->GetInventoryComponent() && RemainingItems > 0)
            {
                UInventoryComponent* CharInventory = Member->GetInventoryComponent();
                
                // キャラクターが持てる分だけ追加
                int32 CanAdd = CharInventory->CanAddItemByWeight(ItemId, RemainingItems);
                if (CanAdd > 0)
                {
                    CharInventory->AddItem(ItemId, CanAdd);
                    RemainingItems -= CanAdd;
                    FString CharacterName = IIdleCharacterInterface::Execute_GetCharacterName(Member);
                    UE_LOG(LogTemp, VeryVerbose, TEXT("ExecuteGathering: Added %d x %s to %s"), 
                        CanAdd, *ItemId, *CharacterName);
                }
            }
        }
        
        // 持ちきれない分があれば帰還
        if (RemainingItems > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("ExecuteGathering: Team %d inventory full, returning to base"), TeamIndex);
            ProcessReturnToBase(TeamIndex);
        }
        else
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("ExecuteGathering: Team %d gathered %d x %s (GatheringPower: %.1f)"), 
                TeamIndex, ItemsGathered, *ItemId, TeamGatheringPower);
            
            // タスク進行度を更新
            if (TaskManager)
            {
                FGlobalTask ActiveTask = TaskManager->FindActiveGatheringTask(ItemId);
                if (!ActiveTask.TaskId.IsEmpty())
                {
                    TaskManager->UpdateTaskProgress(ActiveTask.TaskId, ItemsGathered);
                    // 重要：採集成功
                    UE_LOG(LogTemp, Warning, TEXT("✓ GATHERED: +%d %s (Task: %s)"), 
                        ItemsGathered, *ItemId, *ActiveTask.TaskId);
                }
            }
            
            // UI更新のためTeamComponentのイベントを発行
            if (TeamComp)
            {
                TeamComp->OnCharacterDataChanged.Broadcast(nullptr); // キャラクターデータ更新
                TeamComp->OnTeamActionStateChanged.Broadcast(TeamIndex, ETeamActionState::Working); // ステータス表示更新
            }
        }
    }
    else
    {
    UE_LOG(LogTemp, VeryVerbose, TEXT("ExecuteGathering: Team %d failed to gather %s this turn"), TeamIndex, *ItemId);
    }
}

void UTimeManagerComponent::ExecuteMovementStep(int32 TeamIndex, const FString& TargetLocation)
{
    if (!MovementComponent || !IsValidTeam(TeamIndex) || TargetLocation.IsEmpty())
    {
        return;
    }
    
    // 現在距離と目標距離を取得（整数）
    int32 CurrentDistance = FMath::RoundToInt(MovementComponent->GetCurrentDistanceFromBase(TeamIndex));
    int32 TargetDistance = GetLocationDistance(TargetLocation);
    int32 MovementSpeed = GetTeamMovementSpeed(TeamIndex);
    
    // 残り距離を計算
    int32 RemainingDistance = FMath::Abs(TargetDistance - CurrentDistance);
    int32 ActualMovement = FMath::Min(MovementSpeed, RemainingDistance);
    
    // 目標を超えずに移動
    int32 NewDistance;
    if (TargetDistance > CurrentDistance)
    {
        // 前進（目標を超えない）
        NewDistance = CurrentDistance + ActualMovement;
    }
    else
    {
        // 後退（目標を下回らない）
        NewDistance = CurrentDistance - ActualMovement;
    }
    
    // 距離を更新
    MovementComponent->SetCurrentDistanceFromBase(TeamIndex, static_cast<float>(NewDistance));
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ExecuteMovementStep: Team %d moved from %dm to %dm (target: %dm, speed: %dm/turn)"), 
        TeamIndex, CurrentDistance, NewDistance, TargetDistance, MovementSpeed);
    
    // 採集場所を設定
    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    TeamComp->SetTeamGatheringLocation(TeamIndex, TargetLocation);
}

void UTimeManagerComponent::ProcessReturnToBase(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }
    
    FString CurrentLocation = GetCurrentLocation(TeamIndex);
    UE_LOG(LogTemp, Log, TEXT("ProcessReturnToBase: Team %d returning from %s"), TeamIndex, *CurrentLocation);
    
    if (CurrentLocation == TEXT("base"))
    {
        // 既に拠点にいる場合はIdleに設定
        UTeamComponent* TeamComp = TeamComponents[TeamIndex];
        TeamComp->SetTeamActionState(TeamIndex, ETeamActionState::Idle);
        UE_LOG(LogTemp, Warning, TEXT("ProcessReturnToBase: Team %d already at base, set to Idle"), TeamIndex);
        return;
    }
    
    // 新しいターンベース設計：拠点への移動ステップを実行
    UE_LOG(LogTemp, Log, TEXT("ProcessReturnToBase: Team %d executing movement step to base"), TeamIndex);
    ExecuteMovementStep(TeamIndex, TEXT("base"));
}

int32 UTimeManagerComponent::GetLocationDistance(const FString& LocationId) const
{
    if (LocationId == TEXT("base"))
    {
        return 0;
    }
    
    // LocationManagerを使用して距離を取得
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (ULocationDataTableManager* LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>())
        {
            FLocationDataRow LocationData;
            if (LocationManager->GetLocationData(LocationId, LocationData))
            {
                return FMath::RoundToInt(LocationData.Distance);
            }
        }
    }
    
    // デフォルト値（plainsは100m）
    if (LocationId == TEXT("plains"))
    {
        return 100;
    }
    
    return 0;
}

int32 UTimeManagerComponent::GetTeamMovementSpeed(int32 TeamIndex) const
{
    if (!IsValidTeam(TeamIndex))
    {
        return 30; // デフォルト速度
    }
    
    int32 BaseSpeed = 30; // m/turn
    
    // チーム内最も遅いメンバーに合わせる
    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    int32 MinMemberSpeed = BaseSpeed;
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (Member && Member->GetStatusComponent())
        {
            // 将来的にキャラクターの移動速度ステータスを取得
            // 現在は全て同じ速度として扱う
            int32 MemberSpeed = BaseSpeed;
            MinMemberSpeed = FMath::Min(MinMemberSpeed, MemberSpeed);
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("GetTeamMovementSpeed: Team %d speed = %dm/turn"), TeamIndex, MinMemberSpeed);
    return MinMemberSpeed;
}

bool UTimeManagerComponent::CanTeamCarryNextGather(int32 TeamIndex, const FString& ItemId)
{
    if (!IsValidTeam(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("CanTeamCarryNextGather: Invalid team %d"), TeamIndex);
        return false;
    }
    
    // ItemIdが空の場合は運べないと判定（拠点など採集不可能な場所）
    if (ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("CanTeamCarryNextGather: Team %d - ItemId is empty, cannot carry"), TeamIndex);
        return false;
    }
    
    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("CanTeamCarryNextGather: Checking team %d for item %s"), TeamIndex, *ItemId);
    
    // チームメンバー数チェック
    if (Team.Members.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CanTeamCarryNextGather: Team %d has no members"), TeamIndex);
        return false;
    }
    
    // アイテム1個分の重さで判定
    for (int32 i = 0; i < Team.Members.Num(); ++i)
    {
        AC_IdleCharacter* Member = Team.Members[i];
        if (!Member)
        {
            UE_LOG(LogTemp, Warning, TEXT("CanTeamCarryNextGather: Member %d is null"), i);
            continue;
        }
        
        // デバッグ：コンポーネント取得の詳細チェック
        UInventoryComponent* CharInventory = Member->GetInventoryComponent();
        if (!CharInventory)
        {
            // より詳細なデバッグ情報
            UE_LOG(LogTemp, VeryVerbose, TEXT("CanTeamCarryNextGather: Member %d (%s) GetInventoryComponent() returned null"), 
                i, *Member->GetName());
            
            // コンポーネントを探す
            UInventoryComponent* FoundComp = Member->FindComponentByClass<UInventoryComponent>();
            if (FoundComp)
            {
                UE_LOG(LogTemp, Warning, TEXT("CanTeamCarryNextGather: Found InventoryComponent via FindComponentByClass"));
                CharInventory = FoundComp;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CanTeamCarryNextGather: FindComponentByClass also failed"));
            }
            
            if (!CharInventory)
            {
                continue;
            }
        }
        
        // 詳細デバッグ情報
        float CurrentWeight = CharInventory->GetTotalWeight();
        float MaxWeight = Member->GetStatusComponent() ? Member->GetStatusComponent()->GetCarryingCapacity() : 100.0f;
        
        // アイテムデータ取得チェック
        if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            if (UItemDataTableManager* ItemManager = GameInstance->GetSubsystem<UItemDataTableManager>())
            {
                FItemDataRow ItemData;
                bool bItemDataValid = ItemManager->GetItemData(ItemId, ItemData);
                UE_LOG(LogTemp, VeryVerbose, TEXT("CanTeamCarryNextGather: ItemData for %s - Valid: %s, Weight: %.1f"), 
                    *ItemId, bItemDataValid ? TEXT("Yes") : TEXT("No"), bItemDataValid ? ItemData.Weight : 0.0f);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CanTeamCarryNextGather: ItemDataTableManager not found"));
            }
        }
        
        bool CanCarry = CharInventory->CanAddItemByWeight(ItemId, 1);
        
        // 追加の詳細チェック
        float MaxCapacityFromInventory = CharInventory->GetMaxCarryingCapacity();
        bool IsOverweight = CharInventory->IsOverweight();
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("CanTeamCarryNextGather: Member %d (%s) - Current: %.1f, Max(Status): %.1f, Max(Inventory): %.1f, Overweight: %s, CanCarry: %s"), 
            i, *Member->GetName(), CurrentWeight, MaxWeight, MaxCapacityFromInventory, 
            IsOverweight ? TEXT("Yes") : TEXT("No"), CanCarry ? TEXT("Yes") : TEXT("No"));
        
        if (CanCarry)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("CanTeamCarryNextGather: Team %d can carry %s (member %d has space)"), 
                TeamIndex, *ItemId, i);
            return true; // 誰か一人でも1個持てれば採集継続
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("CanTeamCarryNextGather: Team %d cannot carry %s (all members full)"), 
        TeamIndex, *ItemId);
    return false; // 全員が1個も持てない場合は帰還
}

void UTimeManagerComponent::AutoUnloadResourceItems(int32 TeamIndex)
{
    if (!IsValidTeam(TeamIndex))
    {
        return;
    }
    
    UTeamComponent* TeamComp = TeamComponents[TeamIndex];
    FTeam Team = TeamComp->GetTeam(TeamIndex);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("AutoUnloadResourceItems: Starting auto-unload for team %d"), TeamIndex);
    
    // グローバル倉庫を取得
    UInventoryComponent* GlobalInventory = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AC_PlayerController* IdlePC = Cast<AC_PlayerController>(PC))
            {
                GlobalInventory = IdlePC->GlobalInventory;
            }
        }
    }
    
    if (!GlobalInventory)
    {
        UE_LOG(LogTemp, Error, TEXT("AutoUnloadResourceItems: GlobalInventory not found"));
        return;
    }
    
    // 各チームメンバーのインベントリからアイテムを倉庫に移動
    for (AC_IdleCharacter* Member : Team.Members)
    {
        if (Member && Member->GetInventoryComponent())
        {
            UInventoryComponent* CharInventory = Member->GetInventoryComponent();
            
            // 現在のアイテムを倉庫に移動
            TArray<FInventorySlot> AllSlots = CharInventory->GetAllSlots();
            for (const FInventorySlot& Slot : AllSlots)
            {
                if (Slot.Quantity > 0)
                {
                    UE_LOG(LogTemp, VeryVerbose, TEXT("AutoUnloadResourceItems: %s transferring %d x %s to global storage"), 
                        *Member->GetName(), Slot.Quantity, *Slot.ItemId);
                    
                    // グローバル倉庫にアイテムを追加
                    GlobalInventory->AddItem(Slot.ItemId, Slot.Quantity);
                    
                    // キャラクターインベントリから削除
                    CharInventory->RemoveItem(Slot.ItemId, Slot.Quantity);
                    
                    UE_LOG(LogTemp, VeryVerbose, TEXT("AutoUnloadResourceItems: Transferred %d x %s from %s to global storage"), 
                        Slot.Quantity, *Slot.ItemId, *Member->GetName());
                }
            }
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("AutoUnloadResourceItems: Completed auto-unload for team %d"), TeamIndex);
}