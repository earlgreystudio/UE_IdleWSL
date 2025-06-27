#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/TeamTypes.h"
#include "../Types/TaskTypes.h"
#include "TeamComponent.generated.h"

class AC_IdleCharacter;
class UInventoryComponent;

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamCreated, int32, TeamIndex, const FString&, TeamName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamDeleted, int32, TeamIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemberAssigned, int32, TeamIndex, AC_IdleCharacter*, Character, const FString&, TeamName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberRemoved, int32, TeamIndex, AC_IdleCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskChanged, int32, TeamIndex, ETaskType, NewTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamNameChanged, int32, TeamIndex, const FString&, NewName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamsUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterAdded, AC_IdleCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterRemoved, AC_IdleCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterListChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDataChanged, AC_IdleCharacter*, Character);

// 新しいタスク管理用デリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamTaskStarted, int32, TeamIndex, const FTeamTask&, StartedTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamTaskCompleted, int32, TeamIndex, const FTeamTask&, CompletedTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamTaskSwitched, int32, TeamIndex, ETaskSwitchType, SwitchType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEnded, int32, TeamIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamActionStateChanged, int32, TeamIndex, ETeamActionState, NewState);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTeamComponent();

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	// === 新しいタスク管理データ ===

	// チーム別タスクリスト（各チーム3個まで）
	UPROPERTY(BlueprintReadWrite, Category = "Team Task Management", meta = (AllowPrivateAccess = "true"))
	TArray<FTeamTaskList> TeamTasks;

	// 処理中フラグ（安全性確保用）
	UPROPERTY(BlueprintReadOnly, Category = "Team State")
	bool bCombatEndProcessing = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Team State")
	bool bTaskSwitchProcessing = false;

	// 最大チーム別タスク数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Task Settings")
	int32 MaxTeamTasks = 3;

public:
	// キャラクターリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	TArray<AC_IdleCharacter*> AllPlayerCharacters;

	// チーム用InventoryComponent（動的作成）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team Inventories")
	TArray<UInventoryComponent*> TeamInventories;

	// キャラクター追加
	UFUNCTION(BlueprintCallable, Category = "Team")
	void AddCharacter(AC_IdleCharacter* IdleCharacter);

	// キャラクターリスト取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	TArray<AC_IdleCharacter*> GetCharacterList() const;

	// キャラクター削除
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool RemoveCharacter(AC_IdleCharacter* IdleCharacter);

	// キャラクター数取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	int32 GetCharacterCount() const;

	// ======== チーム管理機能 ========
	
	// チーム作成（作成時は待機タスク）
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	int32 CreateTeam(const FString& TeamName);

	// チーム削除
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool DeleteTeam(int32 TeamIndex);

	// メンバー割り当て
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool AssignCharacterToTeam(AC_IdleCharacter* Character, int32 TeamIndex);

	// メンバー解除
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool RemoveCharacterFromTeam(AC_IdleCharacter* Character, int32 TeamIndex);

	// 全チーム取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	TArray<FTeam> GetAllTeams() const { return Teams; }

	// 特定チーム取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	FTeam GetTeam(int32 TeamIndex) const;

	// 未割り当てキャラクター取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	TArray<AC_IdleCharacter*> GetUnassignedCharacters() const;

	// タスク設定
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool SetTeamTask(int32 TeamIndex, ETaskType NewTask);

	// 冒険タスク用場所設定
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool SetTeamAdventureLocation(int32 TeamIndex, const FString& LocationId);

	// 冒険タスク開始（場所指定付き）
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool StartAdventure(int32 TeamIndex, const FString& LocationId);

	// キャラクターが所属するチームインデックス取得（-1なら未所属）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	int32 GetCharacterTeamIndex(AC_IdleCharacter* Character) const;

	// チーム名変更
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	bool SetTeamName(int32 TeamIndex, const FString& NewTeamName);

	// TaskType用ヘルパー関数（Blueprint用）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	static TArray<FString> GetAllTaskTypeNames();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	static ETaskType GetTaskTypeFromString(const FString& TaskName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	static FString GetTaskTypeDisplayName(ETaskType TaskType);

	// 直接enum使用版（より簡単）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Management")
	static TArray<ETaskType> GetAllTaskTypes();

	// UI用便利関数：チーム作成 + Widget初期化用データ取得
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	FTeam CreateTeamAndGetData(const FString& TeamName, int32& OutTeamIndex);

	// 戦闘終了処理
	UFUNCTION(BlueprintCallable, Category = "Team Management")
	void OnCombatEnd(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers);

	// ======== 新しいチーム別タスク管理機能 ========

	// チーム別タスク追加
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	bool AddTeamTask(int32 TeamIndex, const FTeamTask& NewTask);
	
	// チーム別タスク削除
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	bool RemoveTeamTask(int32 TeamIndex, int32 TaskPriority);
	
	// 現在のチームタスク取得
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	FTeamTask GetCurrentTeamTask(int32 TeamIndex) const;

	// チームタスクリスト取得
	UFUNCTION(BlueprintPure, Category = "Team Task")
	TArray<FTeamTask> GetTeamTasks(int32 TeamIndex) const;
	
	// 次の実行可能タスクへ切り替え
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	bool SwitchToNextAvailableTask(int32 TeamIndex);

	// 安全なタスク切り替え
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	bool SwitchToNextAvailableTaskSafe(int32 TeamIndex);
	
	// タスク実行可能性判定
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	bool CanExecuteTask(int32 TeamIndex, const FTeamTask& Task) const;
	
	// タスク実行処理
	UFUNCTION(BlueprintCallable, Category = "Team Task")
	void ExecuteTask(int32 TeamIndex, const FTeamTask& Task);

	// ======== チーム状態管理機能 ========

	// チームアクション状態設定
	UFUNCTION(BlueprintCallable, Category = "Team State")
	void SetTeamActionState(int32 TeamIndex, ETeamActionState NewState);
	
	// チームアクション状態取得
	UFUNCTION(BlueprintPure, Category = "Team State")
	ETeamActionState GetTeamActionState(int32 TeamIndex) const;

	// 現在のタスクタイプ取得
	UFUNCTION(BlueprintPure, Category = "Team State")
	ETaskType GetCurrentTaskType(int32 TeamIndex) const;
	
	// アクション中断可能かチェック
	UFUNCTION(BlueprintPure, Category = "Team State")
	bool CanInterruptAction(int32 TeamIndex) const;
	
	// 残りアクション時間取得
	UFUNCTION(BlueprintPure, Category = "Team State")
	float GetRemainingActionTime(int32 TeamIndex) const;

	// チームインデックスの有効性チェック
	UFUNCTION(BlueprintPure, Category = "Team Utils")
	bool IsValidTeamIndex(int32 TeamIndex) const;

	// ======== 戦闘関連機能（安全性強化） ========
	
	// 戦闘終了チェック（安全）
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsCombatFinished(int32 TeamIndex) const;
	
	// 戦闘開始（安全）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartCombat(int32 TeamIndex, float EstimatedDuration);
	
	// 戦闘終了（安全）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndCombat(int32 TeamIndex);

	// 戦闘状態取得
	UFUNCTION(BlueprintPure, Category = "Combat")
	ETeamCombatState GetCombatState(int32 TeamIndex) const;

	// 戦闘中チェック
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsTeamInCombat(int32 TeamIndex) const;

	// ======== データアクセス ========

	// Teamsの参照取得（TimeManagerComponent用）
	UFUNCTION(BlueprintPure, Category = "Data Access")
	const TArray<FTeam>& GetTeams() const { return Teams; }

	// 非const版（TimeManagerComponent用）
	TArray<FTeam>& GetTeams() { return Teams; }

	// ======== チームInventory機能 ========

	// チームInventoryComponent取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Inventory")
	UInventoryComponent* GetTeamInventoryComponent(int32 TeamIndex) const;

	// チームInventoryComponent動的作成
	UFUNCTION(BlueprintCallable, Category = "Team Inventory")
	void CreateTeamInventoryComponent(int32 TeamIndex);

	// ======== チーム運搬手段機能 ========

	// チームに運搬手段を設定
	UFUNCTION(BlueprintCallable, Category = "Team Carrying")
	bool SetTeamCarrier(int32 TeamIndex, ECarrierType NewCarrierType);

	// チームの基本積載量を設定
	UFUNCTION(BlueprintCallable, Category = "Team Carrying")
	bool SetTeamBaseCarryingCapacity(int32 TeamIndex, float NewCapacity);

	// チームの総積載量取得（基本積載量 + 運搬手段ボーナス）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Carrying")
	float GetTeamTotalCarryingCapacity(int32 TeamIndex) const;

	// チームの現在の積載重量取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Carrying")
	float GetTeamCurrentWeight(int32 TeamIndex) const;

	// チームの積載率取得（現在重量 / 最大積載量）
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Carrying")
	float GetTeamLoadRatio(int32 TeamIndex) const;

	// ======== イベントディスパッチャー ========
	
	// チーム作成時
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnTeamCreated OnTeamCreated;

	// チーム削除時
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnTeamDeleted OnTeamDeleted;

	// メンバー追加時
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnMemberAssigned OnMemberAssigned;

	// メンバー削除時
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnMemberRemoved OnMemberRemoved;

	// タスク変更時
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnTaskChanged OnTaskChanged;

	// チーム名変更時
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnTeamNameChanged OnTeamNameChanged;

	// 汎用更新通知
	UPROPERTY(BlueprintAssignable, Category = "Team Events")
	FOnTeamsUpdated OnTeamsUpdated;

	// キャラクター追加時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterAdded OnCharacterAdded;

	// キャラクター削除時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterRemoved OnCharacterRemoved;

	// キャラクターリスト変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterListChanged OnCharacterListChanged;

	// キャラクターデータ変更時（チーム配属など）
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterDataChanged OnCharacterDataChanged;

	// === 新しいタスク管理用イベントディスパッチャー ===

	// チームタスク開始時
	UPROPERTY(BlueprintAssignable, Category = "Team Task Events")
	FOnTeamTaskStarted OnTeamTaskStarted;
	
	// チームタスク完了時
	UPROPERTY(BlueprintAssignable, Category = "Team Task Events")
	FOnTeamTaskCompleted OnTeamTaskCompleted;
	
	// チームタスク切り替え時
	UPROPERTY(BlueprintAssignable, Category = "Team Task Events")
	FOnTeamTaskSwitched OnTeamTaskSwitched;

	// 戦闘終了時
	UPROPERTY(BlueprintAssignable, Category = "Combat Events")
	FOnCombatEnded OnCombatEnded;

	// チームアクション状態変更時
	UPROPERTY(BlueprintAssignable, Category = "Team State Events")
	FOnTeamActionStateChanged OnTeamActionStateChanged;

protected:
	// チーム管理データ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Management")
	TArray<FTeam> Teams;

private:
	// 内部管理関数
	bool IsCharacterInAnyTeam(AC_IdleCharacter* Character) const;
	void RemoveCharacterFromAllTeams(AC_IdleCharacter* Character);
};