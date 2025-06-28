#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "../Types/TeamTypes.h"
#include "../Components/TeamComponent.h"
#include "C_TeamCard.h"
#include "C_TeamList.generated.h"

class UTeamComponent;

/**
 * チームリスト表示ウィジェット
 * 全チームをWrapBoxで表示し、チーム作成機能を提供
 */
UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_TeamList : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_TeamList(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Containers")
    UWrapBox* TeamsWrapBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Buttons")
    UButton* CreateTeamButton;

    // TeamComponent参照
    UPROPERTY(BlueprintReadOnly, Category = "Team List")
    UTeamComponent* TeamComponent;

    // チームカードクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team List")
    TSubclassOf<UC_TeamCard> TeamCardClass;

public:
    // 初期化 - TeamComponentを設定
    UFUNCTION(BlueprintCallable, Category = "Team List")
    void InitializeWithTeamComponent(UTeamComponent* InTeamComponent);

    // 表示更新
    UFUNCTION(BlueprintCallable, Category = "Team List")
    void UpdateDisplay();

    // 全チームカードを再生成
    UFUNCTION(BlueprintCallable, Category = "Team List")
    void RefreshAllTeamCards();

    // 特定チームカードの更新
    UFUNCTION(BlueprintCallable, Category = "Team List")
    void UpdateTeamCard(int32 TeamIndex);

protected:
    // チームカード管理
    UPROPERTY()
    TArray<UC_TeamCard*> TeamCards;

    // チーム作成ボタンクリックハンドラー
    UFUNCTION()
    void OnCreateTeamClicked();

    // チーム作成処理
    UFUNCTION(BlueprintCallable, Category = "Team List")
    void CreateNewTeam();

    // デバッグ用 - Blueprint側からテスト可能
    UFUNCTION(BlueprintCallable, Category = "Team List Debug")
    void TestCreateTeam();

    // デバッグ用 - 初期化状態確認
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team List Debug")
    bool IsTeamListInitialized() const { return bIsInitialized; }

    // デバッグ用 - TeamComponent状態確認
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team List Debug")
    bool HasValidTeamComponent() const { return TeamComponent != nullptr; }

    // デバッグ用 - 強制的にボタンクリックイベントを発火
    UFUNCTION(BlueprintCallable, Category = "Team List Debug")
    void ForceButtonClick();

    // デバッグ用 - ボタンの状態確認
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team List Debug")
    bool IsCreateButtonValid() const;

    // チーム作成名入力ダイアログ（Blueprint実装）
    UFUNCTION(BlueprintImplementableEvent, Category = "Team List")
    void ShowTeamNameInputDialog();

    // Blueprint側から呼ばれるチーム作成確認
    UFUNCTION(BlueprintCallable, Category = "Team List")
    void ConfirmCreateTeam(const FString& TeamName);

private:
    // 内部状態管理
    bool bIsInitialized;

    // 自動初期化試行
    void TryAutoInitialize();

    // イベントバインディング
    void BindTeamEvents();
    void UnbindTeamEvents();

    // TeamComponentイベントハンドラー
    UFUNCTION()
    void OnTeamCreated(int32 TeamIndex, const FString& TeamName);

    UFUNCTION()
    void OnTeamDeleted(int32 TeamIndex);

    UFUNCTION()
    void OnTeamsUpdated();

    UFUNCTION()
    void OnMemberAssigned(int32 TeamIndex, AC_IdleCharacter* Character, const FString& TeamName);

    UFUNCTION()
    void OnMemberRemoved(int32 TeamIndex, AC_IdleCharacter* Character);

    UFUNCTION()
    void OnTaskChanged(int32 TeamIndex, ETaskType NewTask);

    UFUNCTION()
    void OnTeamNameChanged(int32 TeamIndex, const FString& NewName);

    // 内部ヘルパー関数
    void ClearAllTeamCards();
    UC_TeamCard* CreateTeamCard(int32 TeamIndex);
    UC_TeamCard* FindTeamCard(int32 TeamIndex) const;
    void RemoveTeamCard(int32 TeamIndex);
};