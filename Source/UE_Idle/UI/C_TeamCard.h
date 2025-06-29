#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "../Types/TeamTypes.h"
#include "../Types/TaskTypes.h"
#include "../Components/TeamComponent.h"
#include "../Components/LocationMovementComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "C_CharacterCard.h"
#include "C_TeamTaskCard.h"
#include "C_TeamTaskMakeSheet.h"
#include "C_TeamCard.generated.h"

class UTeamComponent;
class UTimeManagerComponent;

/**
 * チームカードウィジェット
 * 単一チームの情報を表示し、チームメンバーとタスクを管理
 */
UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_TeamCard : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_TeamCard(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // テキスト表示用ウィジェット
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* TeamNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* CurrentTaskText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* TeamStatusText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* DistanceFromBaseText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* MemberCountText;

    // コンテナウィジェット
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Containers")
    UWrapBox* CharacterCardsContainer;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Containers")
    UVerticalBox* TeamTaskCardsContainer;

    // チームタスク作成ボタン
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Buttons")
    UButton* CreateTaskButton;

    // チーム情報
    UPROPERTY(BlueprintReadOnly, Category = "Team Card")
    int32 TeamIndex;

    UPROPERTY(BlueprintReadOnly, Category = "Team Card")
    UTeamComponent* TeamComponent;

    // ウィジェットクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Card")
    TSubclassOf<UC_CharacterCard> CharacterCardClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Card")
    TSubclassOf<UC_TeamTaskCard> TeamTaskCardClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Card")
    TSubclassOf<UC_TeamTaskMakeSheet> TeamTaskMakeSheetClass;

public:
    // 初期化
    UFUNCTION(BlueprintCallable, Category = "Team Card")
    void InitializeWithTeam(int32 InTeamIndex, UTeamComponent* InTeamComponent);

    // 表示更新
    UFUNCTION(BlueprintCallable, Category = "Team Card")
    void UpdateDisplay();

    // 個別更新機能
    UFUNCTION(BlueprintCallable, Category = "Team Card")
    void UpdateTeamInfo();

    UFUNCTION(BlueprintCallable, Category = "Team Card")
    void UpdateCharacterCards();

    UFUNCTION(BlueprintCallable, Category = "Team Card")
    void UpdateTaskCards();

    // アクセサー
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Card")
    int32 GetTeamIndex() const { return TeamIndex; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Card")
    FTeam GetTeamData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team Card")
    bool IsValidTeamCard() const;

protected:
    // 内部ウィジェット管理
    UPROPERTY()
    TArray<UC_CharacterCard*> CharacterCards;

    UPROPERTY()
    TArray<UC_TeamTaskCard*> TeamTaskCards;

private:
    // 内部状態管理
    bool bIsInitialized;

    // イベントバインディング
    void BindTeamEvents();
    void UnbindTeamEvents();

    // TeamComponentイベントハンドラー
    UFUNCTION()
    void OnMemberAssigned(int32 InTeamIndex, AC_IdleCharacter* Character, const FString& TeamName);

    UFUNCTION()
    void OnMemberRemoved(int32 InTeamIndex, AC_IdleCharacter* Character);

    UFUNCTION()
    void OnTaskChanged(int32 InTeamIndex, ETaskType NewTask);

    UFUNCTION()
    void OnTeamNameChanged(int32 InTeamIndex, const FString& NewName);

    UFUNCTION()
    void OnTeamActionStateChanged(int32 InTeamIndex, ETeamActionState NewState);

    UFUNCTION()
    void OnTeamTaskStarted(int32 InTeamIndex, const FTeamTask& StartedTask);

    UFUNCTION()
    void OnTeamTaskCompleted(int32 InTeamIndex, const FTeamTask& CompletedTask);

    UFUNCTION()
    void OnCharacterDataChanged(AC_IdleCharacter* Character);

    // MovementComponentイベントハンドラー
    UFUNCTION()
    void OnMovementProgressUpdated(int32 InTeamIndex, const FMovementInfo& MovementInfo);

    // 内部ヘルパー関数
    void ClearCharacterCards();
    void ClearTaskCards();
    UC_CharacterCard* CreateCharacterCard(AC_IdleCharacter* Character);
    UC_TeamTaskCard* CreateTaskCard(const FTeamTask& TaskData, int32 TaskPriority);
    FString GetCurrentTaskDisplayText() const;
    FString GetTeamStatusDisplayText() const;
    FString GetDistanceFromBaseDisplayText() const;
    int32 GetCurrentResourceCount() const;
    
    // 表示更新ヘルパー
    void UpdateTeamNameDisplay();
    void UpdateCurrentTaskDisplay();
    void UpdateTeamStatusDisplay();
    void UpdateMemberCountDisplay();
    void UpdateDistanceFromBaseDisplay();

    // ボタンクリックハンドラー
    UFUNCTION()
    void OnCreateTaskClicked();

public:
    // チームタスク作成シート表示
    UFUNCTION(BlueprintCallable, Category = "Team Task Management")
    void ShowTeamTaskMakeSheet();
};