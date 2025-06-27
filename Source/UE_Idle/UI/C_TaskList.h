#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/TaskTypes.h"
#include "C_TaskList.generated.h"

class UScrollBox;
class UTaskManagerComponent;
class UC_GlobalTaskCard;
class UVerticalBox;

/**
 * タスク一覧表示ウィジェット
 * グローバルタスクの管理UIを提供
 */
UCLASS()
class UE_IDLE_API UC_TaskList : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // TaskManagerの設定
    UFUNCTION(BlueprintCallable, Category = "Task List")
    void InitializeWithTaskManager(UTaskManagerComponent* InTaskManager);

    // タスクリストの更新
    UFUNCTION(BlueprintCallable, Category = "Task List")
    void RefreshTaskList();


protected:
    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UScrollBox* TaskScrollBox;


    // タスクカードのウィジェットクラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Task List")
    TSubclassOf<UC_GlobalTaskCard> GlobalTaskCardClass;

    // タスクマネージャーへの参照
    UPROPERTY()
    UTaskManagerComponent* TaskManager;

    // 作成済みタスクカードのリスト
    UPROPERTY()
    TArray<UC_GlobalTaskCard*> TaskCards;

private:

    // タスクマネージャーのイベントハンドラー
    UFUNCTION()
    void OnGlobalTaskAdded(const FGlobalTask& NewTask);

    UFUNCTION()
    void OnGlobalTaskRemoved(int32 TaskIndex);

    UFUNCTION()
    void OnTaskPriorityChanged(int32 TaskIndex, int32 NewPriority);

    // タスクカード作成
    UC_GlobalTaskCard* CreateTaskCard(const FGlobalTask& Task, int32 TaskIndex);

    // 優先度に基づいてカードを並べ替え
    void SortTaskCardsByPriority();

    // イベントバインディング
    void BindTaskManagerEvents();
    void UnbindTaskManagerEvents();
};
