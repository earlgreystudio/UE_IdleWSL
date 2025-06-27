#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/TaskTypes.h"
#include "../Types/TeamTypes.h"
#include "../Types/CharacterTypes.h"
#include "C_TeamTaskCard.generated.h"

class UTextBlock;
class UButton;
class UTeamComponent;
class AC_IdleCharacter;
class UVerticalBox;

/**
 * チームタスクカードウィジェット
 * チーム別タスクの情報表示と操作を提供
 */
UCLASS()
class UE_IDLE_API UC_TeamTaskCard : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初期化
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // タスク情報の設定
    UFUNCTION(BlueprintCallable, Category = "Team Task Card")
    void InitializeWithTeamTask(const FTeamTask& InTask, int32 InTeamIndex, int32 InTaskPriority, UTeamComponent* InTeamComponent);

    // 表示更新
    UFUNCTION(BlueprintCallable, Category = "Team Task Card")
    void UpdateDisplay();

    // チームメンバーが変更されたときの更新
    UFUNCTION(BlueprintCallable, Category = "Team Task Card")
    void OnTeamMembersChanged();

protected:
    // ウィジェットバインディング
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* TaskTypeText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* PriorityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* SkillsBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* PriorityUpButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* PriorityDownButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* DeleteButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* StatusText;

    // タスクデータ
    UPROPERTY(BlueprintReadOnly, Category = "Team Task Card")
    FTeamTask TaskData;

    UPROPERTY(BlueprintReadOnly, Category = "Team Task Card")
    int32 TeamIndex;

    UPROPERTY(BlueprintReadOnly, Category = "Team Task Card")
    int32 TaskPriority;

    // マネージャーへの参照
    UPROPERTY()
    UTeamComponent* TeamComponent;

    // スキル表示用のテキストブロッククラス
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Task Card")
    TSubclassOf<UTextBlock> SkillTextBlockClass;

protected:
    // 削除確認ダイアログ
    UFUNCTION(BlueprintImplementableEvent, Category = "Team Task Card")
    void ShowDeleteConfirmDialog();

    // Blueprint側から呼ばれる削除確認
    UFUNCTION(BlueprintCallable, Category = "Team Task Card")
    void ConfirmDelete();

private:
    // ボタンクリックハンドラー
    UFUNCTION()
    void OnPriorityUpClicked();

    UFUNCTION()
    void OnPriorityDownClicked();

    UFUNCTION()
    void OnDeleteClicked();

    // 表示更新
    void UpdateTaskTypeDisplay();
    void UpdatePriorityDisplay();
    void UpdateSkillsDisplay();
    void UpdateStatusDisplay();
    void UpdateButtonStates();

    // チームメンバーのスキル合計を計算
    float CalculateTeamSkillTotal(const FString& SkillPropertyName) const;

    // FDerivedStatsからスキル値を取得
    float GetSkillValueFromStats(const FDerivedStats& Stats, const FString& PropertyName) const;

    // チームメンバーを取得
    TArray<AC_IdleCharacter*> GetTeamMembers() const;
};
