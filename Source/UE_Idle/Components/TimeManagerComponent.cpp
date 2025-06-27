#include "TimeManagerComponent.h"
#include "TaskManagerComponent.h"
#include "../Components/TeamComponent.h"
#include "Engine/World.h"

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
                
                switch (Team.ActionState)
                {
                    case ETeamActionState::Idle:
                    case ETeamActionState::Working:
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

    // 特定タスクタイプの処理（将来的に詳細実装）
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessSpecificTask: Team %d processing %s"), 
           TeamIndex, *UTaskTypeUtils::GetTaskTypeDisplayName(TaskType));
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

void UTimeManagerComponent::ClearRegisteredComponents()
{
    TaskManager = nullptr;
    TeamComponents.Empty();
    
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
    return IsValid(TaskManager) && TeamComponents.Num() > 0;
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