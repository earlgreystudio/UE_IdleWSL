#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/TaskTypes.h"
#include "C_GlobalTaskCard.generated.h"

class UTextBlock;
class UButton;
class UTaskManagerComponent;
class UItemDataTableManager;
class UHorizontalBox;

/**
 * グローバルタスクカードウィジェット
 * 個別のタスク情報を表示し、操作ボタンを提供
 */
UCLASS()
class UE_IDLE_API UC_GlobalTaskCard : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // タスク情報の設定
    UFUNCTION(BlueprintCallable, Category = "Task Card")
    void InitializeWithTask(const FGlobalTask& InTask, int32 InTaskIndex, UTaskManagerComponent* InTaskManager);

    // 表示更新
    UFUNCTION(BlueprintCallable, Category = "Task Card")
    void UpdateDisplay();

protected:
    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* TaskTypeText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ItemNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* QuantityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* SkillsText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* PriorityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* PriorityUpButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* PriorityDownButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* DeleteButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* ProgressBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ProgressText;

    // タスクデータ
    UPROPERTY(BlueprintReadOnly, Category = "Task Card")
    FGlobalTask TaskData;

    UPROPERTY(BlueprintReadOnly, Category = "Task Card")
    int32 TaskIndex;

    // マネージャーへの参照
    UPROPERTY()
    UTaskManagerComponent* TaskManager;

    UPROPERTY()
    UItemDataTableManager* ItemDataManager;

protected:
    // 削除確認ダイアログ
    UFUNCTION(BlueprintImplementableEvent, Category = "Task Card")
    void ShowDeleteConfirmDialog();

    // Blueprint側から呼ばれる削除確認
    UFUNCTION(BlueprintCallable, Category = "Task Card")
    void ConfirmDelete();

private:
    // ボタンクリックハンドラー
    UFUNCTION()
    void OnPriorityUpClicked();

    UFUNCTION()
    void OnPriorityDownClicked();

    UFUNCTION()
    void OnDeleteClicked();

    // 表示テキストの更新
    void UpdateTaskTypeDisplay();
    void UpdateItemNameDisplay();
    void UpdateQuantityDisplay();
    void UpdateSkillsDisplay();
    void UpdatePriorityDisplay();
    void UpdateProgressDisplay();

    // アイテム名の取得
    FString GetItemDisplayName(const FString& ItemId) const;

    // ボタンの有効/無効を更新
    void UpdateButtonStates();
};
