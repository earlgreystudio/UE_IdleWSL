#include "TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Managers/BattleSystemManager.h"
#include "InventoryComponent.h"
#include "Engine/World.h"

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
		
		// イベント通知
		OnTaskChanged.Broadcast(TeamIndex, NewTask);
		OnTeamsUpdated.Broadcast();
		
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
	
	// 冒険場所を設定した場合、タスクも冒険に変更
	if (!LocationId.IsEmpty() && Teams[TeamIndex].AssignedTask != ETaskType::Adventure)
	{
		Teams[TeamIndex].AssignedTask = ETaskType::Adventure;
		OnTaskChanged.Broadcast(TeamIndex, ETaskType::Adventure);
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
	
	// 採集場所を設定した場合、タスクも採集に変更
	if (!LocationId.IsEmpty() && Teams[TeamIndex].AssignedTask != ETaskType::Gathering)
	{
		Teams[TeamIndex].AssignedTask = ETaskType::Gathering;
		OnTaskChanged.Broadcast(TeamIndex, ETaskType::Gathering);
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
// 新採集システムでは個人キャラクターの積載量をGatheringComponentで管理

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