#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Input/Events.h"
#include "../Types/TaskTypes.h"
#include "C_TaskMakeSheet.generated.h"

class USpinBox;
class UCheckBox;
class UButton;
class UScrollBox;
class UTaskManagerComponent;
class UC_GlobalTaskCard;
class UCraftingComponent;

/**
 * タスク作成シートウィジェット
 * 新しいグローバルタスクを作成するためのUI
 */
UCLASS()
class UE_IDLE_API UC_TaskMakeSheet : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // TaskManagerComponentを設定
    UFUNCTION(BlueprintCallable, Category = "Task Make Sheet")
    void SetTaskManagerComponent(UTaskManagerComponent* InTaskManager);

    // CraftingComponentを設定
    UFUNCTION(BlueprintCallable, Category = "Task Make Sheet")
    void SetCraftingComponent(UCraftingComponent* InCraftingComponent);

    // UI更新
    UFUNCTION(BlueprintCallable, Category = "Task Make Sheet")
    void RefreshTaskList();

    // タスクタイプ選択時の処理
    UFUNCTION(BlueprintCallable, Category = "Task Make Sheet")
    void OnTaskTypeSelectionChanged();

    // ComboBox選択変更時の処理（内部用）
    UFUNCTION()
    void OnTaskTypeComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

protected:
    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* TaskTypeComboBox;


    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* TargetItemComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UComboBoxString* LocationComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    USpinBox* TargetQuantitySpinBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    USpinBox* PrioritySpinBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCheckBox* KeepQuantityCheckBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CreateTaskButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CancelButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UScrollBox* TaskScrollBox;

    // TaskManagerへの参照
    UPROPERTY()
    UTaskManagerComponent* TaskManagerComponent;

    // CraftingComponentへの参照
    UPROPERTY()
    UCraftingComponent* CraftingComponent;

    // タスクカードのクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Task Make Sheet")
    TSubclassOf<UC_GlobalTaskCard> GlobalTaskCardClass;

private:
    // ボタンクリックハンドラー
    UFUNCTION()
    void OnCreateTaskClicked();

    UFUNCTION()
    void OnCancelClicked();

    // UI初期化
    void InitializeTaskTypeComboBox();
    void InitializeDefaultValues();

    // アイテムリスト更新
    void UpdateTargetItemComboBox();

    // 場所リスト更新
    void UpdateLocationComboBox();

    // タスクタイプに応じたUI制御
    void UpdateUIVisibilityForTaskType(ETaskType TaskType);

    // 入力検証
    bool ValidateInput(FString& OutErrorMessage) const;

    // タスク作成
    FGlobalTask CreateTaskFromInput() const;

    // 指定優先度にタスクを挿入（既存タスクの優先度を自動調整）
    bool InsertTaskAtPriority(const FGlobalTask& NewTask);

    // ユニークなタスクID生成
    FString GenerateUniqueTaskId() const;

    // タスクカード作成
    UC_GlobalTaskCard* CreateTaskCard(const FGlobalTask& Task, int32 TaskIndex);

    // UI状態管理
    void UpdateCreateButtonState();
    void ClearInputFields();

    // イベントバインディング
    void BindEvents();
    void UnbindEvents();

    // TaskManagerイベントハンドラー
    UFUNCTION()
    void OnGlobalTaskAdded(const FGlobalTask& NewTask);

    UFUNCTION()
    void OnGlobalTaskRemoved(int32 TaskIndex);

    // テキスト変更ハンドラー
    UFUNCTION(BlueprintCallable, Category = "Task Make Sheet")
    void OnInputChanged();

protected:
    // Blueprint実装イベント
    UFUNCTION(BlueprintImplementableEvent, Category = "Task Make Sheet")
    void ShowValidationError(const FString& ErrorMessage);

    UFUNCTION(BlueprintImplementableEvent, Category = "Task Make Sheet")
    void OnTaskCreatedSuccessfully(const FGlobalTask& CreatedTask);

    UFUNCTION(BlueprintImplementableEvent, Category = "Task Make Sheet")
    void OnTaskCreationCancelled();
};