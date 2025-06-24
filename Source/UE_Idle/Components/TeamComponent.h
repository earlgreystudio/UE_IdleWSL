#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/TeamTypes.h"
#include "TeamComponent.generated.h"

class AC_IdleCharacter;

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamCreated, int32, TeamIndex, const FString&, TeamName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamDeleted, int32, TeamIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemberAssigned, int32, TeamIndex, AC_IdleCharacter*, Character, const FString&, TeamName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberRemoved, int32, TeamIndex, AC_IdleCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskChanged, int32, TeamIndex, ETaskType, NewTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamNameChanged, int32, TeamIndex, const FString&, NewName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamsUpdated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTeamComponent();

protected:
	virtual void BeginPlay() override;

public:
	// キャラクターリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	TArray<AC_IdleCharacter*> AllPlayerCharacters;

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

protected:
	// チーム管理データ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team Management")
	TArray<FTeam> Teams;

private:
	// 内部管理関数
	bool IsCharacterInAnyTeam(AC_IdleCharacter* Character) const;
	void RemoveCharacterFromAllTeams(AC_IdleCharacter* Character);
};