#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Input/Events.h"
#include "../Types/TaskTypes.h"
#include "../Types/TeamTypes.h"
#include "C_TeamTaskMakeSheet.generated.h"

class UTeamComponent;
class UC_TeamTaskCard;

/**
 * チーム用タスク作成シートウィジェット
 * シンプルなチーム別タスク作成UI
 * - 基本選択: タスクタイプのみ
 * - 冒険時のみ: 目的地選択が追加表示
 */
UCLASS()
class UE_IDLE_API UC_TeamTaskMakeSheet : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // チーム情報設定
    UFUNCTION(BlueprintCallable, Category = "Team Task Make Sheet")
    void InitializeWithTeam(int32 InTeamIndex, UTeamComponent* InTeamComponent);

    // タスクタイプ選択時の処理
    UFUNCTION(BlueprintCallable, Category = "Team Task Make Sheet")
    void OnTaskTypeSelectionChanged();

protected:
    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* TaskTypeComboBox;

    // 冒険時のみ表示
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* LocationComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CreateTaskButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CancelButton;

    // チーム情報
    UPROPERTY(BlueprintReadOnly, Category = "Team Task Make Sheet")
    int32 TeamIndex;

    UPROPERTY()
    UTeamComponent* TeamComponent;

    // タスクカードのクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Task Make Sheet")
    TSubclassOf<UC_TeamTaskCard> TeamTaskCardClass;

private:
    // ボタンクリックハンドラー
    UFUNCTION()
    void OnCreateTaskClicked();

    UFUNCTION()
    void OnCancelClicked();

    UFUNCTION()
    void OnTaskTypeComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    // UI初期化
    void InitializeTaskTypeComboBox();
    void InitializeDefaultValues();

    // 場所リスト更新（冒険用）
    void UpdateLocationComboBox();

    // タスクタイプに応じたUI制御
    void UpdateUIVisibilityForTaskType(ETaskType TaskType);

    // Widget閉じる処理
    void CloseTaskMakeSheet();

    // 入力検証
    bool ValidateInput(FString& OutErrorMessage) const;

    // タスク作成
    FTeamTask CreateTeamTaskFromInput() const;

    // UI状態管理
    void UpdateCreateButtonState();
    void ClearInputFields();

    // 入力変更ハンドラー
    UFUNCTION()
    void OnInputChanged();

protected:
    // Blueprint実装イベント
    UFUNCTION(BlueprintImplementableEvent, Category = "Team Task Make Sheet")
    void ShowValidationError(const FString& ErrorMessage);

    UFUNCTION(BlueprintImplementableEvent, Category = "Team Task Make Sheet")
    void OnTeamTaskCreatedSuccessfully(const FTeamTask& CreatedTask);

    UFUNCTION(BlueprintImplementableEvent, Category = "Team Task Make Sheet")
    void OnTeamTaskCreationCancelled();

    UFUNCTION(BlueprintImplementableEvent, Category = "Team Task Make Sheet")
    void OnTeamTaskMakeSheetClosed();
};