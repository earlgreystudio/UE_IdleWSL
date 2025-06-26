#include "TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Managers/BattleSystemManager.h"

UTeamComponent::UTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTeamComponent::BeginPlay()
{
	Super::BeginPlay();
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
	
	// イベント通知
	OnTeamCreated.Broadcast(NewTeamIndex, TeamName);
	OnTeamsUpdated.Broadcast();
	
	return NewTeamIndex;
}

bool UTeamComponent::DeleteTeam(int32 TeamIndex)
{
	if (Teams.IsValidIndex(TeamIndex))
	{
		// チームメンバーを解放する
		FTeam& Team = Teams[TeamIndex];
		Team.Members.Empty();
		
		Teams.RemoveAt(TeamIndex);
		
		// イベント通知
		OnTeamDeleted.Broadcast(TeamIndex);
		OnTeamsUpdated.Broadcast();
		
		return true;
	}
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
		TEXT("自由"),
		TEXT("冒険"),
		TEXT("料理")
	};
}

ETaskType UTeamComponent::GetTaskTypeFromString(const FString& TaskName)
{
	if (TaskName == TEXT("待機"))
		return ETaskType::Idle;
	else if (TaskName == TEXT("自由"))
		return ETaskType::Free;
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
	case ETaskType::Free:
		return TEXT("自由");
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
		ETaskType::Free,
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