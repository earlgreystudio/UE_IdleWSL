#include "TaskInformationService.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/TaskManagerComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/CharacterStatusComponent.h"
#include "../C_PlayerController.h"
#include "MovementService.h"
#include "GatheringService.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UTaskInformationService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 統計情報初期化
    ResetStatistics();
    
    // キャッシュ初期化
    TaskDetailsCache.Empty();
    ExecutabilityCache.Empty();
    LastCacheUpdate = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("📋 TaskInformationService initialized successfully"));
}

void UTaskInformationService::Deinitialize()
{
    // クリーンアップ
    TaskDetailsCache.Empty();
    ExecutabilityCache.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("📋 TaskInformationService deinitialized"));
    
    Super::Deinitialize();
}

TArray<FTaskOption> UTaskInformationService::GetAvailableTaskOptions(AC_IdleCharacter* Character)
{
    TArray<FTaskOption> TaskOptions;
    
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Error, TEXT("📋❌ TaskInformationService: Invalid character provided"));
        return TaskOptions;
    }

    TotalInformationRequests++;
    
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return TaskOptions;
    }
    
    // 全グローバルタスクを取得
    TArray<FGlobalTask> GlobalTasks = TaskManager->GetGlobalTasks();
    
    for (const FGlobalTask& Task : GlobalTasks)
    {
        // キャラクターが実行可能かチェック
        if (CanCharacterExecuteTask(Character, Task))
        {
            FTaskOption Option;
            Option.TaskType = Task.TaskType;
            Option.TaskId = Task.TaskId;
            Option.Description = Task.DisplayName;
            Option.Priority = Task.Priority;
            Option.bIsAvailable = true;
            
            TaskOptions.Add(Option);
        }
    }
    
    // 優先度順でソート
    TaskOptions.Sort([](const FTaskOption& A, const FTaskOption& B) {
        return A.Priority < B.Priority;
    });
    
    UE_LOG(LogTemp, VeryVerbose, 
        TEXT("📋 TaskInformationService: Found %d available task options for character %s"),
        TaskOptions.Num(), *Character->GetCharacterName());
    
    return TaskOptions;
}

FGlobalTask UTaskInformationService::GetTaskDetails(const FString& TaskId)
{
    if (TaskId.IsEmpty())
    {
        return FGlobalTask();
    }
    
    TotalInformationRequests++;
    
    // キャッシュチェック
    if (IsCacheValid() && TaskDetailsCache.Contains(TaskId))
    {
        CacheHits++;
        return TaskDetailsCache[TaskId];
    }
    
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return FGlobalTask();
    }
    
    // TaskManagerから詳細を取得
    int32 TaskIndex = TaskManager->FindTaskByID(TaskId);
    if (TaskIndex != -1)
    {
        FGlobalTask Task = TaskManager->GetGlobalTask(TaskIndex);
        
        // キャッシュに保存
        TaskDetailsCache.Add(TaskId, Task);
        
        return Task;
    }
    
    return FGlobalTask();
}

bool UTaskInformationService::IsTaskPossibleAt(const FString& TaskId, const FString& Location)
{
    if (TaskId.IsEmpty() || Location.IsEmpty())
    {
        return false;
    }
    
    TotalInformationRequests++;
    
    // キャッシュチェック
    FString CacheKey = GenerateCacheKey(TEXT("TaskPossible"), {TaskId, Location});
    if (IsCacheValid() && ExecutabilityCache.Contains(CacheKey))
    {
        CacheHits++;
        return ExecutabilityCache[CacheKey];
    }
    
    FGlobalTask Task = GetTaskDetails(TaskId);
    if (!Task.IsValid())
    {
        return false;
    }
    
    bool bPossible = false;
    
    // タスクタイプに応じた場所チェック
    switch (Task.TaskType)
    {
        case ETaskType::Gathering:
            // 採集タスクは採集可能な場所でのみ実行可能
            bPossible = !GetGatheringLocationsForItem(Task.TargetItemId).IsEmpty();
            break;
            
        case ETaskType::Adventure:
            // 冒険タスクは戦闘可能な場所でのみ実行可能
            bPossible = (Location != TEXT("base"));
            break;
            
        case ETaskType::Construction:
        case ETaskType::Cooking:
        case ETaskType::Crafting:
            // 建築・料理・製作は拠点でのみ実行可能
            bPossible = (Location == TEXT("base"));
            break;
            
        default:
            bPossible = true;
            break;
    }
    
    // キャッシュに保存
    ExecutabilityCache.Add(CacheKey, bPossible);
    
    return bPossible;
}

TArray<FGlobalTask> UTaskInformationService::GetExecutableTasksForTeam(int32 TeamIndex)
{
    TArray<FGlobalTask> ExecutableTasks;
    
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return ExecutableTasks;
    }
    
    TotalInformationRequests++;
    
    // 全グローバルタスクを取得
    TArray<FGlobalTask> AllTasks = TaskManager->GetGlobalTasksByPriority();
    
    for (const FGlobalTask& Task : AllTasks)
    {
        // チームが実行可能かチェック
        if (TaskManager->CanTeamExecuteTask(TeamIndex, Task))
        {
            ExecutableTasks.Add(Task);
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, 
        TEXT("📋 TaskInformationService: Found %d executable tasks for team %d"),
        ExecutableTasks.Num(), TeamIndex);
    
    return ExecutableTasks;
}

bool UTaskInformationService::CanCharacterExecuteTask(AC_IdleCharacter* Character, const FGlobalTask& Task)
{
    if (!IsValid(Character) || !Task.IsValid())
    {
        return false;
    }
    
    // チーム所属チェック
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    // リソース要件チェック
    if (!CheckTaskResourceRequirements(Task, TeamIndex))
    {
        return false;
    }
    
    // スキル要件チェック
    if (!CheckTaskSkillRequirements(Character, Task))
    {
        return false;
    }
    
    // 場所要件チェック
    FString CurrentLocation = GetCharacterCurrentLocation(Character);
    if (!IsTaskPossibleAt(Task.TaskId, CurrentLocation))
    {
        return false;
    }
    
    return true;
}

bool UTaskInformationService::CheckTaskResourceRequirements(const FGlobalTask& Task, int32 TeamIndex)
{
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return false;
    }
    
    // TaskManagerの既存機能を使用
    if (TeamIndex != -1)
    {
        return TaskManager->CanTeamExecuteTask(TeamIndex, Task);
    }
    else
    {
        return TaskManager->CheckGlobalTaskRequirements(Task, 0); // デフォルトチーム0
    }
}

bool UTaskInformationService::CheckTaskSkillRequirements(AC_IdleCharacter* Character, const FGlobalTask& Task)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    // 基本的なスキルチェック（簡易版）
    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (!StatusComp)
    {
        return true; // スキル情報がない場合は実行可能とみなす
    }
    
    // タスクタイプに応じた能力チェック
    float RequiredAbility = GetCharacterTaskAbility(Character, Task.TaskType);
    return RequiredAbility > 0.0f;
}

float UTaskInformationService::EstimateTaskCompletionTime(AC_IdleCharacter* Character, const FGlobalTask& Task)
{
    if (!IsValid(Character) || !Task.IsValid())
    {
        return 1.0f; // デフォルト時間
    }
    
    // キャラクターの効率を計算
    float Efficiency = CalculateTaskEfficiency(Character, Task);
    
    // 基本時間をタスクタイプに応じて決定
    float BaseTime = 1.0f;
    switch (Task.TaskType)
    {
        case ETaskType::Gathering:
            BaseTime = 1.0f * Task.TargetQuantity;
            break;
        case ETaskType::Adventure:
            BaseTime = 5.0f; // 戦闘時間
            break;
        case ETaskType::Construction:
            BaseTime = 3.0f;
            break;
        case ETaskType::Cooking:
        case ETaskType::Crafting:
            BaseTime = 2.0f;
            break;
        default:
            BaseTime = 1.0f;
            break;
    }
    
    // 効率で調整
    float EstimatedTime = BaseTime / FMath::Max(Efficiency, 0.1f);
    
    return FMath::Max(EstimatedTime, 0.5f); // 最低0.5秒
}

FGlobalTask UTaskInformationService::GetHighestPriorityTask(int32 TeamIndex)
{
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return FGlobalTask();
    }
    
    TotalInformationRequests++;
    
    if (TeamIndex != -1)
    {
        // チーム用の次利用可能タスクを取得
        return TaskManager->GetNextAvailableTask(TeamIndex);
    }
    else
    {
        // 全体で最高優先度タスクを取得
        TArray<FGlobalTask> Tasks = TaskManager->GetGlobalTasksByPriority();
        if (Tasks.Num() > 0)
        {
            return Tasks[0];
        }
    }
    
    return FGlobalTask();
}

FGlobalTask UTaskInformationService::GetMostEfficientTask(AC_IdleCharacter* Character, const TArray<FGlobalTask>& AvailableTasks)
{
    if (!IsValid(Character) || AvailableTasks.Num() == 0)
    {
        return FGlobalTask();
    }
    
    FGlobalTask BestTask;
    float HighestEfficiency = 0.0f;
    
    for (const FGlobalTask& Task : AvailableTasks)
    {
        float Efficiency = CalculateTaskEfficiency(Character, Task);
        if (Efficiency > HighestEfficiency)
        {
            HighestEfficiency = Efficiency;
            BestTask = Task;
        }
    }
    
    return BestTask;
}

float UTaskInformationService::GetTaskProgress(const FString& TaskId)
{
    FGlobalTask Task = GetTaskDetails(TaskId);
    if (!Task.IsValid())
    {
        return 0.0f;
    }
    
    if (Task.TargetQuantity <= 0)
    {
        return Task.bIsCompleted ? 1.0f : 0.0f;
    }
    
    return FMath::Clamp((float)Task.CurrentProgress / Task.TargetQuantity, 0.0f, 1.0f);
}

bool UTaskInformationService::IsTaskCompleted(const FString& TaskId)
{
    FGlobalTask Task = GetTaskDetails(TaskId);
    return Task.IsValid() && Task.bIsCompleted;
}

int32 UTaskInformationService::GetTargetGatheringQuantity(int32 TeamIndex, const FString& ItemId)
{
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return 0;
    }
    
    // チームタスクから目標量を検索
    UTeamComponent* TeamComp = GetTeamComponent();
    if (TeamComp && TeamIndex != -1)
    {
        TArray<FTeamTask> TeamTasks = TeamComp->GetTeamTasks(TeamIndex);
        for (const FTeamTask& TeamTask : TeamTasks)
        {
            if (TeamTask.TaskType == ETaskType::Gathering)
            {
                // 簡易的にチームタスクの要件をチェック
                // 実際の実装では、TeamTaskにTargetItemが含まれている必要がある
                return 10; // 仮の目標量
            }
        }
    }
    
    // グローバルタスクから検索
    TArray<FGlobalTask> GlobalTasks = TaskManager->GetGlobalTasks();
    for (const FGlobalTask& Task : GlobalTasks)
    {
        if (Task.TaskType == ETaskType::Gathering && Task.TargetItemId == ItemId)
        {
            return Task.TargetQuantity;
        }
    }
    
    return 0;
}

TArray<FString> UTaskInformationService::GetGatheringLocationsForItem(const FString& ItemId)
{
    // GatheringServiceを使用して採集場所を取得
    UGatheringService* GatheringService = GetWorld() ? 
        GetWorld()->GetGameInstance()->GetSubsystem<UGatheringService>() : nullptr;
    
    if (GatheringService)
    {
        return GatheringService->FindLocationsForItem(ItemId);
    }
    
    // フォールバック: 簡易的な場所判定
    TArray<FString> Locations;
    if (!ItemId.IsEmpty())
    {
        Locations.Add(TEXT("plains")); // デフォルトで平野を追加
    }
    
    return Locations;
}

int32 UTaskInformationService::GetCurrentResourceAmount(const FString& ResourceId, bool bIncludeTeamInventories)
{
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return 0;
    }
    
    // TaskManagerの既存機能を使用
    return TaskManager->GetTotalResourceAmount(ResourceId);
}

TArray<FGlobalTask> UTaskInformationService::GetAllGlobalTasks()
{
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return TArray<FGlobalTask>();
    }
    
    TotalInformationRequests++;
    return TaskManager->GetGlobalTasks();
}

TArray<FTeamTask> UTaskInformationService::GetTeamTasks(int32 TeamIndex)
{
    UTeamComponent* TeamComp = GetTeamComponent();
    if (!TeamComp)
    {
        return TArray<FTeamTask>();
    }
    
    TotalInformationRequests++;
    return TeamComp->GetTeamTasks(TeamIndex);
}

FString UTaskInformationService::GetTaskInformationStatistics()
{
    float CacheHitRate = (TotalInformationRequests > 0) ? 
        (float)CacheHits / TotalInformationRequests * 100.0f : 0.0f;
    
    return FString::Printf(
        TEXT("TaskInfo Stats: %d requests, %d cache hits (%.1f%%), cache size: %d"),
        TotalInformationRequests, CacheHits, CacheHitRate, TaskDetailsCache.Num());
}

FString UTaskInformationService::GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId)
{
    UTaskManagerComponent* TaskManager = GetTaskManagerComponent();
    if (!TaskManager)
    {
        return TEXT("");
    }
    
    return TaskManager->GetTargetItemForTeam(TeamIndex, LocationId);
}

bool UTaskInformationService::CheckTaskTypeRequirements(ETaskType TaskType, AC_IdleCharacter* Character, const FString& Location)
{
    // 基本的なタスクタイプ別要件チェック
    switch (TaskType)
    {
        case ETaskType::Gathering:
            return !Location.IsEmpty();
        case ETaskType::Adventure:
            return (Location != TEXT("base"));
        case ETaskType::Construction:
        case ETaskType::Cooking:
        case ETaskType::Crafting:
            return (Location == TEXT("base"));
        default:
            return true;
    }
}

int32 UTaskInformationService::GetCharacterTeamIndex(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return -1;
    }
    
    UTeamComponent* TeamComp = GetTeamComponent();
    if (!TeamComp)
    {
        return -1;
    }
    
    // キャラクターが所属するチームを検索
    for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
    {
        FTeam Team = TeamComp->GetTeam(i);
        if (Team.Members.Contains(Character))
        {
            return i;
        }
    }
    
    return -1;
}

FString UTaskInformationService::GetCharacterCurrentLocation(AC_IdleCharacter* Character)
{
    // MovementServiceを使用して現在地を取得
    UMovementService* MovementService = GetWorld() ? 
        GetWorld()->GetGameInstance()->GetSubsystem<UMovementService>() : nullptr;
    
    if (MovementService)
    {
        return MovementService->GetCharacterCurrentLocation(Character);
    }
    
    return TEXT("base"); // デフォルト
}

float UTaskInformationService::CalculateTaskEfficiency(AC_IdleCharacter* Character, const FGlobalTask& Task)
{
    if (!IsValid(Character))
    {
        return 1.0f;
    }
    
    // キャラクターの能力を取得
    float Ability = GetCharacterTaskAbility(Character, Task.TaskType);
    
    // 基本効率は能力値をそのまま使用
    return FMath::Max(Ability / 10.0f, 0.1f); // 10が標準、最低0.1倍
}

float UTaskInformationService::GetCharacterTaskAbility(AC_IdleCharacter* Character, ETaskType TaskType)
{
    if (!IsValid(Character))
    {
        return 1.0f;
    }
    
    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (!StatusComp)
    {
        return 1.0f;
    }
    
    // 派生ステータスからタスク別能力を取得
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    
    switch (TaskType)
    {
        case ETaskType::Gathering:
            return DerivedStats.GatheringPower;
        case ETaskType::Construction:
            return DerivedStats.ConstructionPower;
        case ETaskType::Cooking:
            return DerivedStats.CookingPower;
        case ETaskType::Crafting:
            return DerivedStats.CraftingPower;
        case ETaskType::Adventure:
            return DerivedStats.CombatPower;
        default:
            return DerivedStats.WorkPower;
    }
}

UTaskManagerComponent* UTaskInformationService::GetTaskManagerComponent()
{
    // PlayerControllerからTaskManagerComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return nullptr;
    }
    
    return PlayerController->FindComponentByClass<UTaskManagerComponent>();
}

UTeamComponent* UTaskInformationService::GetTeamComponent()
{
    // PlayerControllerからTeamComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return nullptr;
    }
    
    return PlayerController->FindComponentByClass<UTeamComponent>();
}

bool UTaskInformationService::AreReferencesValid()
{
    return IsValid(GetTaskManagerComponent()) && IsValid(GetTeamComponent());
}

void UTaskInformationService::ResetStatistics()
{
    TotalInformationRequests = 0;
    CacheHits = 0;
}

void UTaskInformationService::UpdateCache()
{
    if (!IsCacheValid())
    {
        TaskDetailsCache.Empty();
        ExecutabilityCache.Empty();
        LastCacheUpdate = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    }
}

bool UTaskInformationService::IsCacheValid() const
{
    if (!GetWorld())
    {
        return false;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    return (CurrentTime - LastCacheUpdate) < CacheValidDuration;
}

FString UTaskInformationService::GenerateCacheKey(const FString& Prefix, const TArray<FString>& Params)
{
    FString Key = Prefix;
    for (const FString& Param : Params)
    {
        Key += TEXT("_") + Param;
    }
    return Key;
}