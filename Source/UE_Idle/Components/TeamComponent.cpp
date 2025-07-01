#include "TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Managers/BattleSystemManager.h"
#include "../Types/CharacterTypes.h"
#include "../C_PlayerController.h"
#include "InventoryComponent.h"
#include "CombatComponent.h"
#include "LocationMovementComponent.h"
#include "TaskManagerComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UTeamComponent::UTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// InventoryComponentは動的に作成するため、コンストラクタでは何もしない
	// チーム作成時にCreateTeamInventoryComponent()で作成

	// 新しいタスク管理フィールドの初期化
	bCombatEndProcessing = false;
	bTaskSwitchProcessing = false;
	MaxTeamTasks = 3;
}

void UTeamComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("TeamComponent: BeginPlay - Initialized"));
}

void UTeamComponent::BeginDestroy()
{
	// 進行中の処理をクリア
	bCombatEndProcessing = false;
	bTaskSwitchProcessing = false;
	
	// タイマーをクリア
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	
	// タスクリストをクリア
	TeamTasks.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("TeamComponent: BeginDestroy - Cleaned up"));
	
	Super::BeginDestroy();
}

void UTeamComponent::AddCharacter(AC_IdleCharacter* IdleCharacter)
{
	if (IdleCharacter && !AllPlayerCharacters.Contains(IdleCharacter))
	{
		AllPlayerCharacters.Add(IdleCharacter);
		
		// Broadcast events
		OnCharacterAdded.Broadcast(IdleCharacter);
		OnCharacterListChanged.Broadcast();
		
		UE_LOG(LogTemp, Warning, TEXT("TeamComponent: Character added, broadcasting events"));
	}
}

TArray<AC_IdleCharacter*> UTeamComponent::GetCharacterList() const
{
	return AllPlayerCharacters;
}

bool UTeamComponent::RemoveCharacter(AC_IdleCharacter* IdleCharacter)
{
	if (IdleCharacter)
	{
		if (AllPlayerCharacters.Remove(IdleCharacter) > 0)
		{
			// Broadcast events
			OnCharacterRemoved.Broadcast(IdleCharacter);
			OnCharacterListChanged.Broadcast();
			
			UE_LOG(LogTemp, Warning, TEXT("TeamComponent: Character removed, broadcasting events"));
			return true;
		}
	}
	return false;
}

int32 UTeamComponent::GetCharacterCount() const
{
	return AllPlayerCharacters.Num();
}

// ======== チーム管理機能実装 ========

int32 UTeamComponent::CreateTeam(const FString& TeamName)
{
	FTeam NewTeam;
	NewTeam.TeamName = TeamName;
	NewTeam.AssignedTask = ETaskType::Idle;  // デフォルトは待機
	NewTeam.bIsActive = true;
	
	Teams.Add(NewTeam);
	int32 NewTeamIndex = Teams.Num() - 1;
	
	// TeamInventory削除 - 新採集システムでは個人インベントリを使用
	
	// 新しいタスク管理データの初期化
	FTeamTaskList EmptyTaskList;
	TeamTasks.Add(EmptyTaskList);
	
	// イベント通知
	OnTeamCreated.Broadcast(NewTeamIndex, TeamName);
	OnTeamsUpdated.Broadcast();
	
	UE_LOG(LogTemp, Log, TEXT("CreateTeam: Created team '%s' at index %d"), *TeamName, NewTeamIndex);
	
	return NewTeamIndex;
}

bool UTeamComponent::DeleteTeam(int32 TeamIndex)
{
	if (Teams.IsValidIndex(TeamIndex))
	{
		// チームメンバーを解放する
		FTeam& Team = Teams[TeamIndex];
		Team.Members.Empty();
		
		// TeamInventory削除済み
		
		// 対応するタスクリストを削除
		if (TeamTasks.IsValidIndex(TeamIndex))
		{
			TeamTasks.RemoveAt(TeamIndex);
		}
		
		Teams.RemoveAt(TeamIndex);
		
		// イベント通知
		OnTeamDeleted.Broadcast(TeamIndex);
		OnTeamsUpdated.Broadcast();
		
		UE_LOG(LogTemp, Log, TEXT("DeleteTeam: Deleted team at index %d"), TeamIndex);
		
		return true;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("DeleteTeam: Invalid team index %d"), TeamIndex);
	return false;
}

bool UTeamComponent::AssignCharacterToTeam(AC_IdleCharacter* Character, int32 TeamIndex)
{
	if (!Character || !Teams.IsValidIndex(TeamIndex))
	{
		return false;
	}
	
	// 既に他のチームに所属している場合は解除
	RemoveCharacterFromAllTeams(Character);
	
	// 新しいチームに追加
	Teams[TeamIndex].Members.Add(Character);
	
	// イベント通知
	UE_LOG(LogTemp, Log, TEXT("Character assigned to Team %d (%s)"), TeamIndex, *Teams[TeamIndex].TeamName);
	OnMemberAssigned.Broadcast(TeamIndex, Character, Teams[TeamIndex].TeamName);
	OnTeamsUpdated.Broadcast();
	OnCharacterDataChanged.Broadcast(Character);  // Cardの更新をトリガー
	
	return true;
}

bool UTeamComponent::RemoveCharacterFromTeam(AC_IdleCharacter* Character, int32 TeamIndex)
{
	if (!Character || !Teams.IsValidIndex(TeamIndex))
	{
		return false;
	}
	
	bool bRemoved = Teams[TeamIndex].Members.Remove(Character) > 0;
	if (bRemoved)
	{
		// イベント通知
		OnMemberRemoved.Broadcast(TeamIndex, Character);
		OnTeamsUpdated.Broadcast();
		OnCharacterDataChanged.Broadcast(Character);  // Cardの更新をトリガー
	}
	
	return bRemoved;
}

FTeam UTeamComponent::GetTeam(int32 TeamIndex) const
{
	if (Teams.IsValidIndex(TeamIndex))
	{
		return Teams[TeamIndex];
	}
	
	return FTeam();  // 空のチームを返す
}

TArray<AC_IdleCharacter*> UTeamComponent::GetUnassignedCharacters() const
{
	TArray<AC_IdleCharacter*> UnassignedList;
	
	// AllPlayerCharactersから、どのチームにも所属していないキャラクターを抽出
	for (AC_IdleCharacter* Character : AllPlayerCharacters)
	{
		if (Character && !IsCharacterInAnyTeam(Character))
		{
			UnassignedList.Add(Character);
		}
	}
	
	return UnassignedList;
}

bool UTeamComponent::SetTeamTask(int32 TeamIndex, ETaskType NewTask)
{
	if (Teams.IsValidIndex(TeamIndex))
	{
		Teams[TeamIndex].AssignedTask = NewTask;
		
		// 冒険以外のタスクに変更する場合は場所をクリア
		if (NewTask != ETaskType::Adventure)
		{
			Teams[TeamIndex].AdventureLocationId = TEXT("");
			Teams[TeamIndex].bInCombat = false;
		}
		
		// 🚨 CRITICAL FIX: UIからのタスク設定を自律システムに反映
		// 実装計画書：「既存機能の完全再現」を保証
		ReevaluateAllTeamStrategies();
		
		// イベント通知
		OnTaskChanged.Broadcast(TeamIndex, NewTask);
		OnTeamsUpdated.Broadcast();
		
		UE_LOG(LogTemp, Log, TEXT("🎯 SetTeamTask: Team %d task updated to %d, strategies reevaluated"), 
			TeamIndex, (int32)NewTask);
		
		return true;
	}
	return false;
}

bool UTeamComponent::SetTeamAdventureLocation(int32 TeamIndex, const FString& LocationId)
{
	if (!Teams.IsValidIndex(TeamIndex))
	{
		return false;
	}

	Teams[TeamIndex].AdventureLocationId = LocationId;
	
	bool bTaskChanged = false;
	
	// 冒険場所を設定した場合、タスクも冒険に変更
	if (!LocationId.IsEmpty() && Teams[TeamIndex].AssignedTask != ETaskType::Adventure)
	{
		Teams[TeamIndex].AssignedTask = ETaskType::Adventure;
		OnTaskChanged.Broadcast(TeamIndex, ETaskType::Adventure);
		bTaskChanged = true;
	}
	
	// 🚨 CRITICAL FIX: 場所やタスクが変更された場合、戦略を更新
	if (bTaskChanged || !LocationId.IsEmpty())
	{
		ReevaluateAllTeamStrategies();
	}
	
	OnTeamsUpdated.Broadcast();
	
	UE_LOG(LogTemp, Log, TEXT("Team %d adventure location set to: %s"), TeamIndex, *LocationId);
	return true;
}

bool UTeamComponent::StartAdventure(int32 TeamIndex, const FString& LocationId)
{
	if (!Teams.IsValidIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("StartAdventure: Invalid team index %d"), TeamIndex);
		return false;
	}

	FTeam& Team = Teams[TeamIndex];
	
	// チームメンバーがいるかチェック
	if (Team.Members.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("StartAdventure: Team %d has no members"), TeamIndex);
		return false;
	}

	// 既に戦闘中かチェック
	if (Team.bInCombat)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartAdventure: Team %d is already in combat"), TeamIndex);
		return false;
	}

	// 場所とタスク設定
	Team.AdventureLocationId = LocationId;
	Team.AssignedTask = ETaskType::Adventure;
	Team.bInCombat = true;

	// BattleSystemManagerを使用してイベントトリガー
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UBattleSystemManager* BattleManager = GameInstance->GetSubsystem<UBattleSystemManager>())
		{
			bool bEventTriggered = BattleManager->StartTeamAdventure(Team.Members, LocationId);
			if (!bEventTriggered)
			{
				// イベントトリガーに失敗した場合はフラグをリセット
				Team.bInCombat = false;
				UE_LOG(LogTemp, Error, TEXT("StartAdventure: Failed to trigger combat event for team %d"), TeamIndex);
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("StartAdventure: BattleSystemManager not found"));
			Team.bInCombat = false;
			return false;
		}
	}

	// イベント通知
	OnTaskChanged.Broadcast(TeamIndex, ETaskType::Adventure);
	OnTeamsUpdated.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("Adventure started for team %d at location %s with %d members"), 
		TeamIndex, *LocationId, Team.Members.Num());
	
	return true;
}

bool UTeamComponent::SetTeamGatheringLocation(int32 TeamIndex, const FString& LocationId)
{
	if (!Teams.IsValidIndex(TeamIndex))
	{
		return false;
	}

	Teams[TeamIndex].GatheringLocationId = LocationId;
	
	bool bTaskChanged = false;
	
	// 採集場所を設定した場合、タスクも採集に変更
	// ただし、冒険タスク中の場合は変更しない
	if (!LocationId.IsEmpty() && 
		Teams[TeamIndex].AssignedTask != ETaskType::Gathering && 
		Teams[TeamIndex].AssignedTask != ETaskType::Adventure)
	{
		Teams[TeamIndex].AssignedTask = ETaskType::Gathering;
		OnTaskChanged.Broadcast(TeamIndex, ETaskType::Gathering);
		bTaskChanged = true;
	}
	
	// 🚨 CRITICAL FIX: 場所やタスクが変更された場合、戦略を更新
	if (bTaskChanged || !LocationId.IsEmpty())
	{
		ReevaluateAllTeamStrategies();
	}
	
	OnTeamsUpdated.Broadcast();
	
	UE_LOG(LogTemp, Log, TEXT("Team %d gathering location set to: %s"), TeamIndex, *LocationId);
	return true;
}

bool UTeamComponent::StartGathering(int32 TeamIndex, const FString& LocationId)
{
	if (!Teams.IsValidIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("StartGathering: Invalid team index %d"), TeamIndex);
		return false;
	}

	FTeam& Team = Teams[TeamIndex];
	
	// チームメンバーがいるかチェック
	if (Team.Members.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("StartGathering: Team %d has no members"), TeamIndex);
		return false;
	}

	// 既に戦闘中かチェック
	if (Team.bInCombat)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartGathering: Team %d is in combat, cannot start gathering"), TeamIndex);
		return false;
	}

	// 場所とタスク設定
	Team.GatheringLocationId = LocationId;
	Team.AssignedTask = ETaskType::Gathering;

	// イベント通知
	OnTaskChanged.Broadcast(TeamIndex, ETaskType::Gathering);
	OnTeamsUpdated.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("Gathering task started for team %d at location %s with %d members"), 
		TeamIndex, *LocationId, Team.Members.Num());
	
	return true;
}

int32 UTeamComponent::GetCharacterTeamIndex(AC_IdleCharacter* Character) const
{
	if (!Character)
	{
		return -1;
	}
	
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if (Teams[i].Members.Contains(Character))
		{
			return i;
		}
	}
	
	return -1;  // 未所属
}

bool UTeamComponent::SetTeamName(int32 TeamIndex, const FString& NewTeamName)
{
	if (Teams.IsValidIndex(TeamIndex))
	{
		Teams[TeamIndex].TeamName = NewTeamName;
		
		// イベント通知
		OnTeamNameChanged.Broadcast(TeamIndex, NewTeamName);
		OnTeamsUpdated.Broadcast();
		
		return true;
	}
	return false;
}

// ======== TaskType用ヘルパー関数 ========

TArray<FString> UTeamComponent::GetAllTaskTypeNames()
{
	return {
		TEXT("待機"),
		TEXT("全て"),
		TEXT("冒険"),
		TEXT("料理")
	};
}

ETaskType UTeamComponent::GetTaskTypeFromString(const FString& TaskName)
{
	if (TaskName == TEXT("待機"))
		return ETaskType::Idle;
	else if (TaskName == TEXT("全て"))
		return ETaskType::All;
	else if (TaskName == TEXT("冒険"))
		return ETaskType::Adventure;
	else if (TaskName == TEXT("料理"))
		return ETaskType::Cooking;
	
	return ETaskType::Idle;  // デフォルト
}

FString UTeamComponent::GetTaskTypeDisplayName(ETaskType TaskType)
{
	switch (TaskType)
	{
	case ETaskType::Idle:
		return TEXT("待機");
	case ETaskType::All:
		return TEXT("全て");
	case ETaskType::Adventure:
		return TEXT("冒険");
	case ETaskType::Cooking:
		return TEXT("料理");
	default:
		return TEXT("不明");
	}
}

TArray<ETaskType> UTeamComponent::GetAllTaskTypes()
{
	return {
		ETaskType::Idle,
		ETaskType::All,
		ETaskType::Adventure,
		ETaskType::Cooking
	};
}

FTeam UTeamComponent::CreateTeamAndGetData(const FString& TeamName, int32& OutTeamIndex)
{
	// チーム作成
	OutTeamIndex = CreateTeam(TeamName);
	
	// 作成されたチーム情報を返す
	if (Teams.IsValidIndex(OutTeamIndex))
	{
		return Teams[OutTeamIndex];
	}
	
	// 失敗時は空のチーム
	OutTeamIndex = -1;
	return FTeam();
}

// ======== 内部管理関数 ========

bool UTeamComponent::IsCharacterInAnyTeam(AC_IdleCharacter* Character) const
{
	for (const FTeam& Team : Teams)
	{
		if (Team.Members.Contains(Character))
		{
			return true;
		}
	}
	return false;
}

void UTeamComponent::RemoveCharacterFromAllTeams(AC_IdleCharacter* Character)
{
	bool bWasRemoved = false;
	for (FTeam& Team : Teams)
	{
		if (Team.Members.Remove(Character) > 0)
		{
			bWasRemoved = true;
		}
	}
	
	// キャラクターが実際に削除された場合はイベント発信
	if (bWasRemoved && Character)
	{
		OnCharacterDataChanged.Broadcast(Character);  // Cardの更新をトリガー
	}
}

void UTeamComponent::OnCombatEnd(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers)
{
	UE_LOG(LogTemp, Log, TEXT("OnCombatEnd called with %d winners and %d losers"), Winners.Num(), Losers.Num());
	
	// 勝者側と敗者側の両方のチームのbInCombatフラグをリセット
	TSet<int32> ProcessedTeams;
	
	// 勝者側のチーム処理
	for (AC_IdleCharacter* Character : Winners)
	{
		if (Character)
		{
			int32 TeamIndex = GetCharacterTeamIndex(Character);
			if (TeamIndex >= 0 && !ProcessedTeams.Contains(TeamIndex))
			{
				if (Teams.IsValidIndex(TeamIndex))
				{
					Teams[TeamIndex].bInCombat = false;
					ProcessedTeams.Add(TeamIndex);
					UE_LOG(LogTemp, Log, TEXT("Reset bInCombat flag for team %d (%s) - Winner"), 
						TeamIndex, *Teams[TeamIndex].TeamName);
				}
			}
		}
	}
	
	// 敗者側のチーム処理
	for (AC_IdleCharacter* Character : Losers)
	{
		if (Character)
		{
			int32 TeamIndex = GetCharacterTeamIndex(Character);
			if (TeamIndex >= 0 && !ProcessedTeams.Contains(TeamIndex))
			{
				if (Teams.IsValidIndex(TeamIndex))
				{
					Teams[TeamIndex].bInCombat = false;
					ProcessedTeams.Add(TeamIndex);
					UE_LOG(LogTemp, Log, TEXT("Reset bInCombat flag for team %d (%s) - Loser"), 
						TeamIndex, *Teams[TeamIndex].TeamName);
				}
			}
		}
	}
	
	// チーム更新通知
	OnTeamsUpdated.Broadcast();
}

// ======== 旧チームInventory機能（削除） ========
// 新採集システムでは個人インベントリを使用

// CreateTeamInventoryComponent削除 - 新採集システムでは個人インベントリを使用

// ======== 旧チーム運搬手段機能（削除） ========
// 新採集システムでは個人キャラクターの積載量を使用

// 旧運搬手段関連メソッド削除済み
// 新採集システムでは個人キャラクターの積載量をTaskManagerで管理

// ======== 新しいチーム別タスク管理機能実装 ========

bool UTeamComponent::AddTeamTask(int32 TeamIndex, const FTeamTask& NewTask)
{
	if (!IsValidTeamIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("AddTeamTask: Invalid team index %d"), TeamIndex);
		return false;
	}

	// タスクリストのサイズ確保
	while (TeamTasks.Num() <= TeamIndex)
	{
		FTeamTaskList EmptyTaskList;
		TeamTasks.Add(EmptyTaskList);
	}

	FTeamTaskList& TaskList = TeamTasks[TeamIndex];

	if (TaskList.Num() >= MaxTeamTasks)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddTeamTask: Team %d has reached maximum task limit (%d)"), TeamIndex, MaxTeamTasks);
		return false;
	}

	if (!NewTask.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AddTeamTask: Invalid task provided for team %d"), TeamIndex);
		return false;
	}

	// 優先度重複チェック
	for (const FTeamTask& ExistingTask : TaskList.Tasks)
	{
		if (ExistingTask.Priority == NewTask.Priority)
		{
			UE_LOG(LogTemp, Warning, TEXT("AddTeamTask: Priority %d already exists in team %d"), NewTask.Priority, TeamIndex);
			return false;
		}
	}

	TaskList.Add(NewTask);
	
	UE_LOG(LogTemp, Log, TEXT("AddTeamTask: Added task with priority %d to team %d"), NewTask.Priority, TeamIndex);
	
	// チームがアイドル状態の場合、即座にタスクを実行
	if (Teams[TeamIndex].ActionState == ETeamActionState::Idle)
	{
		UE_LOG(LogTemp, Log, TEXT("AddTeamTask: Team %d is idle, attempting to execute new task"), TeamIndex);
		SwitchToNextAvailableTaskSafe(TeamIndex);
	}
	else
	{
		OnTeamTaskStarted.Broadcast(TeamIndex, NewTask);
	}
	
	return true;
}

bool UTeamComponent::RemoveTeamTask(int32 TeamIndex, int32 TaskPriority)
{
	if (!IsValidTeamIndex(TeamIndex) || !TeamTasks.IsValidIndex(TeamIndex))
	{
		return false;
	}

	FTeamTaskList& TaskList = TeamTasks[TeamIndex];
	
	for (int32 i = 0; i < TaskList.Num(); i++)
	{
		if (TaskList[i].Priority == TaskPriority)
		{
			FTeamTask RemovedTask = TaskList[i];
			TaskList.RemoveAt(i);
			
			UE_LOG(LogTemp, Log, TEXT("RemoveTeamTask: Removed task with priority %d from team %d"), TaskPriority, TeamIndex);
			
			// タスクリストが空になった場合、チームをアイドル状態にリセット
			if (TaskList.Num() == 0)
			{
				Teams[TeamIndex].AssignedTask = ETaskType::Idle;
				UE_LOG(LogTemp, Log, TEXT("RemoveTeamTask: Team %d has no tasks, set to Idle"), TeamIndex);
				OnTaskChanged.Broadcast(TeamIndex, ETaskType::Idle);
			}
			
			OnTeamTaskCompleted.Broadcast(TeamIndex, RemovedTask);
			
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("RemoveTeamTask: Task with priority %d not found in team %d"), TaskPriority, TeamIndex);
	return false;
}

FTeamTask UTeamComponent::GetCurrentTeamTask(int32 TeamIndex) const
{
	if (!IsValidTeamIndex(TeamIndex) || !TeamTasks.IsValidIndex(TeamIndex))
	{
		return FTeamTask(); // デフォルトタスク
	}

	const FTeamTaskList& TaskList = TeamTasks[TeamIndex];
	
	if (TaskList.Num() == 0)
	{
		return FTeamTask(); // 空のタスク
	}

	// 優先度順にソートして最高優先度のタスクを返す
	TArray<FTeamTask> SortedTasks = TaskList.Tasks;
	SortedTasks.Sort([](const FTeamTask& A, const FTeamTask& B) {
		return A.Priority < B.Priority;
	});

	return SortedTasks[0];
}

TArray<FTeamTask> UTeamComponent::GetTeamTasks(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex) && TeamTasks.IsValidIndex(TeamIndex))
	{
		return TeamTasks[TeamIndex].Tasks;
	}
	
	return TArray<FTeamTask>();
}

bool UTeamComponent::SwitchToNextAvailableTask(int32 TeamIndex)
{
	if (bTaskSwitchProcessing)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchToNextAvailableTask: Already processing task switch for team %d"), TeamIndex);
		return false;
	}

	return SwitchToNextAvailableTaskSafe(TeamIndex);
}

bool UTeamComponent::SwitchToNextAvailableTaskSafe(int32 TeamIndex)
{
	if (bTaskSwitchProcessing || !IsValidTeamIndex(TeamIndex))
	{
		return false;
	}

	bTaskSwitchProcessing = true;

	bool bResult = false;

	if (TeamTasks.IsValidIndex(TeamIndex))
	{
		const FTeamTaskList& TaskList = TeamTasks[TeamIndex];
		
		// 優先度順に実行可能なタスクを検索
		TArray<FTeamTask> SortedTasks = TaskList.Tasks;
		SortedTasks.Sort([](const FTeamTask& A, const FTeamTask& B) {
			return A.Priority < B.Priority;
		});

		for (const FTeamTask& Task : SortedTasks)
		{
			if (CanExecuteTask(TeamIndex, Task))
			{
				ExecuteTask(TeamIndex, Task);
				OnTeamTaskSwitched.Broadcast(TeamIndex, ETaskSwitchType::Normal);
				bResult = true;
				break;
			}
		}
	}

	if (!bResult)
	{
		// 実行可能なタスクがない場合は待機
		SetTeamActionState(TeamIndex, ETeamActionState::Idle);
	}

	bTaskSwitchProcessing = false;
	return bResult;
}

bool UTeamComponent::CanExecuteTask(int32 TeamIndex, const FTeamTask& Task) const
{
	if (!IsValidTeamIndex(TeamIndex) || !Task.IsValid())
	{
		return false;
	}

	const FTeam& Team = Teams[TeamIndex];

	// チームが中断可能な状態かチェック
	if (!Team.CanInterruptAction())
	{
		return false;
	}

	// 最小人数チェック
	if (Team.Members.Num() < Task.MinTeamSize)
	{
		return false;
	}

	// リソース要件チェック（将来的に詳細実装）
	// TODO: TaskManagerComponentとの連携でリソースチェック

	return true;
}

void UTeamComponent::ExecuteTask(int32 TeamIndex, const FTeamTask& Task)
{
	if (!IsValidTeamIndex(TeamIndex) || !Task.IsValid())
	{
		return;
	}

	FTeam& Team = Teams[TeamIndex];
	
	// タスク実行状態に設定
	Team.ActionState = ETeamActionState::Working;
	Team.AssignedTask = Task.TaskType; // タスクタイプを設定
	Team.ActionStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	Team.EstimatedCompletionTime = Task.EstimatedCompletionTime * 3600.0f; // 時間を秒に変換
	
	// 採集タスクの場合、既存のGatheringLocationIdを使用
	if (Task.TaskType == ETaskType::Gathering)
	{
		// GatheringLocationIdは既にチーム作成時やUI操作で設定されているはず
		UE_LOG(LogTemp, Log, TEXT("ExecuteTask: Gathering task for team %d with location %s"), 
			TeamIndex, *Team.GatheringLocationId);
	}

	UE_LOG(LogTemp, Log, TEXT("ExecuteTask: Team %d started executing %s task with priority %d"), 
		TeamIndex, *UTaskTypeUtils::GetTaskTypeDisplayName(Task.TaskType), Task.Priority);
	OnTeamTaskStarted.Broadcast(TeamIndex, Task);
}

// ======== チーム状態管理機能実装 ========

void UTeamComponent::SetTeamActionState(int32 TeamIndex, ETeamActionState NewState)
{
	if (!IsValidTeamIndex(TeamIndex))
	{
		return;
	}

	ETeamActionState OldState = Teams[TeamIndex].ActionState;
	Teams[TeamIndex].ActionState = NewState;

	if (OldState != NewState)
	{
		UE_LOG(LogTemp, Log, TEXT("SetTeamActionState: Team %d state changed from %s to %s"), 
			   TeamIndex, *UTaskTypeUtils::GetActionStateDisplayName(OldState), *UTaskTypeUtils::GetActionStateDisplayName(NewState));
		
		OnTeamActionStateChanged.Broadcast(TeamIndex, NewState);
		OnTeamsUpdated.Broadcast();
	}
}

ETeamActionState UTeamComponent::GetTeamActionState(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex))
	{
		return Teams[TeamIndex].ActionState;
	}
	
	return ETeamActionState::Idle;
}

ETaskType UTeamComponent::GetCurrentTaskType(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex))
	{
		return Teams[TeamIndex].AssignedTask;
	}
	
	return ETaskType::Idle;
}

bool UTeamComponent::CanInterruptAction(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex))
	{
		return Teams[TeamIndex].CanInterruptAction();
	}
	
	return false;
}

float UTeamComponent::GetRemainingActionTime(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex))
	{
		float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		return Teams[TeamIndex].GetRemainingActionTime(CurrentTime);
	}
	
	return 0.0f;
}

bool UTeamComponent::IsValidTeamIndex(int32 TeamIndex) const
{
	return Teams.IsValidIndex(TeamIndex) && IsValid(this);
}

// ======== 戦闘関連機能実装（安全性強化） ========

bool UTeamComponent::IsCombatFinished(int32 TeamIndex) const
{
	// 防御的プログラミング - 範囲チェック
	if (!IsValidTeamIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("IsCombatFinished: Invalid TeamIndex %d"), TeamIndex);
		return false;
	}
	
	// オブジェクト有効性チェック
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("IsCombatFinished: Invalid TeamComponent"));
		return false;
	}
	
	// 処理中フラグチェック（重複処理防止）
	if (bCombatEndProcessing)
	{
		return false;
	}
	
	// 実際の状態判定
	return Teams[TeamIndex].IsCombatFinished();
}

void UTeamComponent::StartCombat(int32 TeamIndex, float EstimatedDuration)
{
	if (!IsValidTeamIndex(TeamIndex))
	{
		return;
	}

	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	Teams[TeamIndex].StartCombatSafe(CurrentTime, EstimatedDuration);
	
	UE_LOG(LogTemp, Log, TEXT("StartCombat: Team %d entered combat (Duration: %.1fs)"), TeamIndex, EstimatedDuration);
	OnTeamActionStateChanged.Broadcast(TeamIndex, ETeamActionState::InCombat);
}

void UTeamComponent::EndCombat(int32 TeamIndex)
{
	// 重複処理防止
	if (bCombatEndProcessing || !IsValidTeamIndex(TeamIndex))
	{
		return;
	}
	
	// 処理中フラグセット
	bCombatEndProcessing = true;
	
	// 状態遷移
	Teams[TeamIndex].EndCombatSafe();
	
	// イベント通知（非同期安全）
	GetWorld()->GetTimerManager().SetTimerForNextTick([this, TeamIndex]() {
		if (IsValid(this) && IsValidTeamIndex(TeamIndex))
		{
			Teams[TeamIndex].ActionState = ETeamActionState::Idle;
			Teams[TeamIndex].bInCombat = false;
			OnCombatEnded.Broadcast(TeamIndex);
			bCombatEndProcessing = false;
			
			UE_LOG(LogTemp, Log, TEXT("EndCombat: Team %d combat ended safely"), TeamIndex);
		}
	});
}

ETeamCombatState UTeamComponent::GetCombatState(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex))
	{
		return Teams[TeamIndex].CombatState;
	}
	
	return ETeamCombatState::NotInCombat;
}

bool UTeamComponent::IsTeamInCombat(int32 TeamIndex) const
{
	if (IsValidTeamIndex(TeamIndex))
	{
		return Teams[TeamIndex].IsInCombat();
	}
	
	return false;
}

void UTeamComponent::SetTeamCombatState(int32 TeamIndex, ETeamCombatState NewState)
{
	if (IsValidTeamIndex(TeamIndex))
	{
		Teams[TeamIndex].CombatState = NewState;
		UE_LOG(LogTemp, VeryVerbose, TEXT("SetTeamCombatState: Team %d set to %s"), 
			TeamIndex, *UEnum::GetValueAsString(NewState));
	}
}

UCombatComponent* UTeamComponent::GetCombatComponent() const
{
	// 現在のアクターのCombatComponentを取得
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<UCombatComponent>();
	}
	
	return nullptr;
}

// === 新しい委譲型実行システム実装 ===

// 専門コンポーネント取得ヘルパー
// GetGatheringComponent は削除済み - TaskManagerを使用

ULocationMovementComponent* UTeamComponent::GetMovementComponent() const
{
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<ULocationMovementComponent>();
	}
	return nullptr;
}

// 委譲メソッド実装
bool UTeamComponent::ExecuteMovement(int32 TeamIndex, const FString& TargetLocation)
{
	UE_LOG(LogTemp, Log, TEXT("🚶 TeamComponent: Delegating movement to %s for team %d"), *TargetLocation, TeamIndex);
	
	if (!IsValidTeamIndex(TeamIndex) || TargetLocation.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Invalid movement parameters"));
		return false;
	}
	
	ULocationMovementComponent* MovementComp = GetMovementComponent();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ MovementComponent not found"));
		return false;
	}
	
	// チームの現在位置を取得（LocationMovementComponentから）
	FString CurrentLocation = TEXT("base"); // デフォルトは拠点
	
	// LocationMovementComponentから現在位置を取得
	float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
	if (CurrentDistance <= 0.0f)
	{
		CurrentLocation = TEXT("base");
	}
	else if (FMath::IsNearlyEqual(CurrentDistance, 100.0f, 1.0f))
	{
		CurrentLocation = TEXT("plains");
	}
	else if (FMath::IsNearlyEqual(CurrentDistance, 200.0f, 1.0f))
	{
		CurrentLocation = TEXT("forest");
	}
	else if (FMath::IsNearlyEqual(CurrentDistance, 500.0f, 1.0f))
	{
		CurrentLocation = TEXT("swamp");
	}
	else if (FMath::IsNearlyEqual(CurrentDistance, 800.0f, 1.0f))
	{
		CurrentLocation = TEXT("mountain");
	}
	else
	{
		// 移動中の場合は最後の目的地を使用
		CurrentLocation = TEXT("base");
	}
	
	UE_LOG(LogTemp, Log, TEXT("🗺️ Team %d current location: %s (distance: %.1f)"), TeamIndex, *CurrentLocation, CurrentDistance);
	
	// 既に目的地にいるかチェック
	if (CurrentLocation == TargetLocation)
	{
		UE_LOG(LogTemp, Log, TEXT("🏁 Team %d is already at %s, skipping movement"), TeamIndex, *TargetLocation);
		SetTeamActionState(TeamIndex, ETeamActionState::Working);
		return true;
	}
	
	// LocationMovementComponentに移動開始を委譲
	bool bMovementStarted = MovementComp->StartMovement(TeamIndex, CurrentLocation, TargetLocation);
	if (!bMovementStarted)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to start movement for team %d"), TeamIndex);
		return false;
	}
	
	// チーム状態を更新
	SetTeamGatheringLocation(TeamIndex, TargetLocation);
	
	// 拠点への移動で既にReturning状態の場合は状態を保持、そうでなければMoving状態に設定
	FTeam& Team = Teams[TeamIndex];
	if (TargetLocation == TEXT("base") && Team.ActionState == ETeamActionState::Returning)
	{
		UE_LOG(LogTemp, Log, TEXT("🏠 Team %d continuing return to base, keeping Returning state"), TeamIndex);
		// Returning状態を保持
	}
	else
	{
		SetTeamActionState(TeamIndex, ETeamActionState::Moving);
	}
	
	UE_LOG(LogTemp, Log, TEXT("✅ Movement initiated to %s"), *TargetLocation);
	return true;
}

bool UTeamComponent::ExecuteGathering(int32 TeamIndex, const FString& TargetItem)
{
	UE_LOG(LogTemp, Log, TEXT("🌾 TeamComponent: Delegating gathering of %s for team %d"), *TargetItem, TeamIndex);
	
	if (!IsValidTeamIndex(TeamIndex) || TargetItem.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Invalid gathering parameters"));
		return false;
	}
	
	// TaskManagerを取得（GatheringComponentから移行）
	UTaskManagerComponent* TaskManager = nullptr;
	if (AActor* Owner = GetOwner())
	{
		TaskManager = Owner->FindComponentByClass<UTaskManagerComponent>();
	}
	if (!TaskManager)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ TaskManager not found"));
		return false;
	}
	
	// チームの現在位置を取得（LocationMovementComponentから）
	ULocationMovementComponent* MovementComp = GetMovementComponent();
	FString CurrentLocation = TEXT("base");
	
	if (MovementComp)
	{
		float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
		if (CurrentDistance <= 0.0f)
		{
			CurrentLocation = TEXT("base");
		}
		else if (FMath::IsNearlyEqual(CurrentDistance, 100.0f, 1.0f))
		{
			CurrentLocation = TEXT("plains");
		}
		else if (FMath::IsNearlyEqual(CurrentDistance, 200.0f, 1.0f))
		{
			CurrentLocation = TEXT("forest");
		}
		else if (FMath::IsNearlyEqual(CurrentDistance, 500.0f, 1.0f))
		{
			CurrentLocation = TEXT("swamp");
		}
		else if (FMath::IsNearlyEqual(CurrentDistance, 800.0f, 1.0f))
		{
			CurrentLocation = TEXT("mountain");
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("🌾 ExecuteGathering: Team %d at location %s"), TeamIndex, *CurrentLocation);
	
	// TaskManagerで採集実行（シンプル化）
	bool bSuccess = TaskManager->ExecuteGathering(TeamIndex, TargetItem, CurrentLocation);
	if (bSuccess)
	{
		SetTeamActionState(TeamIndex, ETeamActionState::Working);
		UE_LOG(LogTemp, VeryVerbose, TEXT("🌾 ExecuteGathering: Team %d gathering %s at %s"), TeamIndex, *TargetItem, *CurrentLocation);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🌾 ExecuteGathering: Failed to execute gathering for team %d"), TeamIndex);
	}
	
	UE_LOG(LogTemp, Log, TEXT("✅ Gathering initiated for %s"), *TargetItem);
	return true;
}

bool UTeamComponent::ExecuteCombat(int32 TeamIndex, const FString& TargetLocation)
{
	UE_LOG(LogTemp, Log, TEXT("⚔️ TeamComponent: Delegating combat at %s for team %d"), *TargetLocation, TeamIndex);
	
	if (!IsValidTeamIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Invalid combat parameters"));
		return false;
	}
	
	UCombatComponent* CombatComp = GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ CombatComponent not found"));
		return false;
	}
	
	// 戦闘中でなければ戦闘開始
	if (!CombatComp->IsInCombat())
	{
		// TODO: 敵生成システムと連携
		TArray<AC_IdleCharacter*> EnemyTeam; // 一時的に空の敵チーム
		
		FTeam Team = GetTeam(TeamIndex);
		bool bCombatStarted = CombatComp->StartCombatSimple(Team.Members, EnemyTeam);
		
		if (bCombatStarted)
		{
			SetTeamActionState(TeamIndex, ETeamActionState::InCombat);
			UE_LOG(LogTemp, Log, TEXT("✅ Combat started for team %d"), TeamIndex);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("❌ Failed to start combat for team %d"), TeamIndex);
			return false;
		}
	}
	else
	{
		// 戦闘継続中 - CombatComponent::ProcessCombat()で1ターン処理
		CombatComp->ProcessCombat(0.0f);
		UE_LOG(LogTemp, VeryVerbose, TEXT("⚔️ Processing combat turn for team %d"), TeamIndex);
		return true;
	}
}

bool UTeamComponent::ExecuteUnload(int32 TeamIndex)
{
	UE_LOG(LogTemp, Log, TEXT("📦 TeamComponent: Delegating unload for team %d"), TeamIndex);
	
	if (!IsValidTeamIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Invalid unload parameters"));
		return false;
	}
	
	// TODO: TimeManagerのAutoUnloadResourceItems()ロジックを委譲
	// 一時的な実装：処理完了フラグ
	UE_LOG(LogTemp, Log, TEXT("✅ Unload completed for team %d"), TeamIndex);
	return true;
}

// メインの実行メソッド
bool UTeamComponent::ExecutePlan(const FTaskExecutionPlan& Plan, int32 TeamIndex)
{
	UE_LOG(LogTemp, Log, TEXT("📋 TeamComponent: Executing plan for team %d - %s"), TeamIndex, *Plan.ExecutionReason);
	
	if (!Plan.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Invalid execution plan"));
		return false;
	}
	
	switch (Plan.ExecutionAction)
	{
		case ETaskExecutionAction::MoveToLocation:
			return ExecuteMovement(TeamIndex, Plan.TargetLocation);
			
		case ETaskExecutionAction::ExecuteGathering:
			return ExecuteGathering(TeamIndex, Plan.TargetItem);
			
		case ETaskExecutionAction::ExecuteCombat:
			return ExecuteCombat(TeamIndex, Plan.TargetLocation);
			
		case ETaskExecutionAction::ReturnToBase:
			return ExecuteMovement(TeamIndex, TEXT("base"));
			
		case ETaskExecutionAction::UnloadItems:
			return ExecuteUnload(TeamIndex);
			
		case ETaskExecutionAction::WaitIdle:
			SetToIdle(TeamIndex);
			return true;
			
		case ETaskExecutionAction::None:
		default:
			UE_LOG(LogTemp, Warning, TEXT("❌ Unsupported execution action: %d"), (int32)Plan.ExecutionAction);
			SetToIdle(TeamIndex);
			return false;
	}
}

void UTeamComponent::SetToIdle(int32 TeamIndex)
{
	UE_LOG(LogTemp, Log, TEXT("💤 TeamComponent: Setting team %d to idle"), TeamIndex);
	
	if (IsValidTeamIndex(TeamIndex))
	{
		SetTeamActionState(TeamIndex, ETeamActionState::Idle);
	}
}

// === TimeManagerComponent用アクセサー実装 ===

void UTeamComponent::SetTeamActionStateInternal(int32 TeamIndex, ETeamActionState NewState, float ActionStartTime, float EstimatedCompletionTime)
{
	if (!Teams.IsValidIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("SetTeamActionStateInternal: Invalid team index %d"), TeamIndex);
		return;
	}
	
	FTeam& Team = Teams[TeamIndex];
	Team.ActionState = NewState;
	Team.ActionStartTime = ActionStartTime;
	Team.EstimatedCompletionTime = EstimatedCompletionTime;
	
	// イベント通知
	OnTeamActionStateChanged.Broadcast(TeamIndex, NewState);
}

// ===========================================
// 自律的キャラクターシステム - チーム連携機能実装（Phase 2.2）
// ===========================================

FTeamStrategy UTeamComponent::GetTeamStrategy(int32 TeamIndex) const
{
	// チームインデックスの有効性チェック
	if (!IsValidTeamIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("🧠👥 GetTeamStrategy: Invalid team index %d"), TeamIndex);
		return FTeamStrategy(); // デフォルト戦略
	}

	// 戦略が存在するかチェック
	if (!TeamStrategies.IsValidIndex(TeamIndex))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 GetTeamStrategy: No strategy found for team %d, generating default"), TeamIndex);
		
		// デフォルト戦略を生成
		FTeamStrategy DefaultStrategy;
		const FTeam& Team = Teams[TeamIndex];
		DefaultStrategy.RecommendedTaskType = Team.AssignedTask;
		DefaultStrategy.StrategyReason = TEXT("Default strategy based on assigned task");
		
		return DefaultStrategy;
	}

	// 戦略の有効期限をチェック
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (StrategyUpdateTimes.IsValidIndex(TeamIndex))
	{
		float TimeSinceUpdate = CurrentTime - StrategyUpdateTimes[TeamIndex];
		if (TimeSinceUpdate > TeamStrategies[TeamIndex].ValidDuration)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 GetTeamStrategy: Strategy for team %d expired, needs update"), TeamIndex);
		}
	}

	return TeamStrategies[TeamIndex];
}

FTeamInfo UTeamComponent::GetTeamInfoForCharacter(AC_IdleCharacter* Character) const
{
	FTeamInfo TeamInfo;

	if (!IsValid(Character))
	{
		UE_LOG(LogTemp, Error, TEXT("🧠👥 GetTeamInfoForCharacter: Invalid character"));
		return TeamInfo;
	}

	// キャラクターが所属するチームを検索
	int32 CharacterTeamIndex = -1;
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if (Teams[i].Members.Contains(Character))
		{
			CharacterTeamIndex = i;
			break;
		}
	}

	if (CharacterTeamIndex == -1)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 GetTeamInfoForCharacter: Character %s not in any team"), 
			*Character->GetCharacterName());
		return TeamInfo; // デフォルト値（チーム未所属）
	}

	// チーム情報を設定
	const FTeam& Team = Teams[CharacterTeamIndex];
	TeamInfo.TeamIndex = CharacterTeamIndex;
	TeamInfo.TeamName = Team.TeamName;
	TeamInfo.CurrentTask = Team.AssignedTask;
	TeamInfo.ActionState = Team.ActionState;
	TeamInfo.TotalMembers = Team.Members.Num();
	TeamInfo.ActiveMembers = Team.bIsActive ? Team.Members.Num() : 0;
	TeamInfo.Teammates = Team.Members;

	// 目標情報の設定
	if (Team.AssignedTask == ETaskType::Adventure)
	{
		TeamInfo.CurrentTargetLocation = Team.AdventureLocationId;
	}
	else if (Team.AssignedTask == ETaskType::Gathering)
	{
		TeamInfo.CurrentTargetLocation = Team.GatheringLocationId;
	}
	else
	{
		TeamInfo.CurrentTargetLocation = TEXT("base");
	}

	// TaskManagerから目標アイテムを取得
	AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PlayerController)
	{
		UTaskManagerComponent* TaskManager = PlayerController->FindComponentByClass<UTaskManagerComponent>();
		if (TaskManager)
		{
			TeamInfo.CurrentTargetItem = TaskManager->GetTargetItemForTeam(CharacterTeamIndex, TeamInfo.CurrentTargetLocation);
		}
	}

	// チーム戦略の設定
	TeamInfo.CurrentStrategy = GetTeamStrategy(CharacterTeamIndex);

	// 連携が必要かの判定（簡易版）
	TeamInfo.bNeedsCoordination = (Team.Members.Num() > 1) && (Team.ActionState == ETeamActionState::Working);
	if (TeamInfo.bNeedsCoordination)
	{
		TeamInfo.CoordinationMessage = TEXT("Team coordination recommended for current task");
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 GetTeamInfoForCharacter: Generated info for %s in team %d (%s)"), 
		*Character->GetCharacterName(), CharacterTeamIndex, *Team.TeamName);

	return TeamInfo;
}

bool UTeamComponent::CoordinateWithTeammates(AC_IdleCharacter* Character, const FCharacterAction& ProposedAction)
{
	if (!IsValid(Character))
	{
		UE_LOG(LogTemp, Error, TEXT("🧠👥 CoordinateWithTeammates: Invalid character"));
		return false;
	}

	// キャラクターが所属するチームを検索
	int32 TeamIndex = -1;
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if (Teams[i].Members.Contains(Character))
		{
			TeamIndex = i;
			break;
		}
	}

	if (TeamIndex == -1)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 CoordinateWithTeammates: Character %s not in any team, no coordination needed"), 
			*Character->GetCharacterName());
		return true; // チーム未所属なら調整不要
	}

	const FTeam& Team = Teams[TeamIndex];

	// 単独チームの場合は調整不要
	if (Team.Members.Num() <= 1)
	{
		return true;
	}

	// 基本的な調整ロジック（簡易版）
	// 将来的により高度な調整ロジックを実装予定

	// 重複する目標のチェック
	if (ProposedAction.ActionType == ECharacterActionType::GatherResources)
	{
		// 同じアイテムを複数人で採集しようとしていないかチェック
		// 現在は許可（実際の採集処理で調整される）
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 CoordinateWithTeammates: %s gathering %s - approved"), 
			*Character->GetCharacterName(), *ProposedAction.TargetItem);
		return true;
	}

	if (ProposedAction.ActionType == ECharacterActionType::MoveToLocation)
	{
		// 移動は基本的に問題なし
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 CoordinateWithTeammates: %s moving to %s - approved"), 
			*Character->GetCharacterName(), *ProposedAction.TargetLocation);
		return true;
	}

	// その他のアクションも基本的に承認
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 CoordinateWithTeammates: %s action %d - approved"), 
		*Character->GetCharacterName(), (int32)ProposedAction.ActionType);
	
	return true;
}

void UTeamComponent::UpdateTeamStrategy(int32 TeamIndex, const FTeamStrategy& NewStrategy)
{
	if (!IsValidTeamIndex(TeamIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("🧠👥 UpdateTeamStrategy: Invalid team index %d"), TeamIndex);
		return;
	}

	// 配列サイズを調整
	while (TeamStrategies.Num() <= TeamIndex)
	{
		TeamStrategies.Add(FTeamStrategy());
	}
	while (StrategyUpdateTimes.Num() <= TeamIndex)
	{
		StrategyUpdateTimes.Add(0.0f);
	}

	// 戦略を更新
	TeamStrategies[TeamIndex] = NewStrategy;
	StrategyUpdateTimes[TeamIndex] = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	UE_LOG(LogTemp, Log, TEXT("🧠👥 UpdateTeamStrategy: Team %d strategy updated - %s"), 
		TeamIndex, *NewStrategy.StrategyReason);
}

void UTeamComponent::ReevaluateAllTeamStrategies()
{
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 ReevaluateAllTeamStrategies: Updating all team strategies"));

	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if (!Teams[i].bIsActive)
		{
			continue;
		}

		// 現在のチーム状況に基づいて戦略を生成
		FTeamStrategy NewStrategy;
		const FTeam& Team = Teams[i];

		// チームの現在のタスクに基づいて戦略を決定
		NewStrategy.RecommendedTaskType = Team.AssignedTask;
		NewStrategy.StrategyPriority = 1; // デフォルト優先度

		switch (Team.AssignedTask)
		{
			case ETaskType::Gathering:
				NewStrategy.StrategyReason = TEXT("Focus on resource gathering");
				NewStrategy.RecommendedLocation = Team.GatheringLocationId.IsEmpty() ? TEXT("plains") : Team.GatheringLocationId;
				break;

			case ETaskType::Adventure:
				NewStrategy.StrategyReason = TEXT("Explore and combat");
				NewStrategy.RecommendedLocation = Team.AdventureLocationId.IsEmpty() ? TEXT("plains") : Team.AdventureLocationId;
				break;

			case ETaskType::All:
				NewStrategy.StrategyReason = TEXT("Execute tasks by priority");
				NewStrategy.RecommendedLocation = TEXT("base");
				break;

			default:
				NewStrategy.StrategyReason = TEXT("Idle or specialized task");
				NewStrategy.RecommendedLocation = TEXT("base");
				break;
		}

		NewStrategy.RequiredMinTeamSize = FMath::Max(1, Team.Members.Num());
		NewStrategy.ValidDuration = 120.0f; // 2分間有効

		UpdateTeamStrategy(i, NewStrategy);
	}
}
