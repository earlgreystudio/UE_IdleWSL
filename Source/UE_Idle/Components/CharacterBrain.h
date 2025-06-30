#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../Types/CharacterTypes.h"
#include "../Types/TeamTypes.h"
#include "../Types/TaskTypes.h"
#include "LocationMovementComponent.h"
#include "TeamComponent.h"
#include "CharacterBrain.generated.h"

// Forward declarations
class AC_IdleCharacter;
class UTaskManagerComponent;

/**
 * キャラクター自律判断システムの基底クラス
 * 現在のTimeManagerロジックを移植し、キャラクター個別の判断機能を提供
 */
UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UCharacterBrain : public UObject
{
    GENERATED_BODY()

public:
    UCharacterBrain();

    // ===========================================
    // メイン判断システム
    // ===========================================
    
    /**
     * 現在の状況に基づいて最適な行動を決定する
     * @param Situation 現在のキャラクター状況
     * @return 決定された行動
     */
    UFUNCTION(BlueprintCallable, Category = "Decision")
    FCharacterAction DecideOptimalAction(const FCharacterSituation& Situation);

    /**
     * キャラクターの性格を設定
     * @param NewPersonality 新しい性格タイプ
     */
    UFUNCTION(BlueprintCallable, Category = "Character Brain")
    void SetPersonality(ECharacterPersonality NewPersonality);

    /**
     * 現在の性格を取得
     * @return 現在の性格タイプ
     */
    UFUNCTION(BlueprintCallable, Category = "Character Brain")
    ECharacterPersonality GetPersonality() const { return MyPersonality; }

    /**
     * 必要な参照コンポーネントを設定
     */
    UFUNCTION(BlueprintCallable, Category = "Character Brain")
    void InitializeReferences(UTaskManagerComponent* TaskManager, UTeamComponent* TeamComp, ULocationMovementComponent* MovementComp);

    /**
     * 所有キャラクターの参照を設定
     */
    UFUNCTION(BlueprintCallable, Category = "Character Brain")
    void SetCharacterReference(AC_IdleCharacter* Character);

protected:
    // ===========================================
    // タスク別判断ロジック（既存機能の移植）
    // ===========================================
    
    /**
     * 採集行動の判断（TimeManagerからの移植）
     * @param Situation 現在の状況
     * @return 採集行動
     */
    FCharacterAction DecideGatheringAction(const FCharacterSituation& Situation);

    /**
     * 移動行動の判断（TimeManagerからの移植）
     * @param Situation 現在の状況
     * @return 移動行動
     */
    FCharacterAction DecideMovementAction(const FCharacterSituation& Situation);

    /**
     * 冒険行動の判断（TimeManagerからの移植）
     * @param Situation 現在の状況
     * @return 冒険行動
     */
    FCharacterAction DecideAdventureAction(const FCharacterSituation& Situation);

    /**
     * 待機行動の判断
     * @param Situation 現在の状況
     * @param Reason 待機する理由
     * @return 待機行動
     */
    FCharacterAction DecideWaitAction(const FCharacterSituation& Situation, const FString& Reason = TEXT("No available actions"));

    // ===========================================
    // ヘルパー関数（既存ロジックの移植）
    // ===========================================
    
    /**
     * 指定アイテムの採集に最適な場所を検索（TimeManagerからの移植）
     * @param TargetItem 採集対象アイテム
     * @return 採集場所ID
     */
    FString FindGatheringLocation(const FString& TargetItem);

    /**
     * 拠点に帰還すべきかの判定（TimeManagerからの移植）
     * @param Situation 現在の状況
     * @return 帰還が必要ならtrue
     */
    bool ShouldReturnToBase(const FCharacterSituation& Situation);
    
    /**
     * 荷下ろしすべきアイテムがあるかチェック
     * @param Situation 現在の状況
     * @return 荷下ろしが必要ならtrue
     */
    bool HasItemsToUnload(const FCharacterSituation& Situation);

    /**
     * 指定チームの現在の目標アイテムを取得（TaskManagerからの移植）
     * @param TeamIndex チームインデックス
     * @param LocationId 現在の場所ID
     * @return 目標アイテムID
     */
    FString GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId);

    /**
     * 指定チームの採集場所を取得
     * @param TeamIndex チームインデックス
     * @return 採集場所ID
     */
    FString GetTeamGatheringLocation(int32 TeamIndex);

    /**
     * 性格に基づく行動優先度の修正
     * @param BaseAction 基本行動
     * @param Situation 現在の状況
     * @return 性格を考慮した行動
     */
    FCharacterAction ApplyPersonalityModifiers(const FCharacterAction& BaseAction, const FCharacterSituation& Situation);

    // ===========================================
    // デバッグ・ログ機能
    // ===========================================
    
    /**
     * 判断プロセスをログ出力
     * @param Situation 判断時の状況
     * @param Decision 決定された行動
     */
    void LogDecisionProcess(const FCharacterSituation& Situation, const FCharacterAction& Decision);

private:
    // ===========================================
    // プロパティ
    // ===========================================
    
    /**
     * このキャラクターの性格タイプ
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Brain", meta = (AllowPrivateAccess = "true"))
    ECharacterPersonality MyPersonality;

    /**
     * このキャラクターの行動優先度設定
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Brain", meta = (AllowPrivateAccess = "true"))
    TArray<FActionPreference> MyActionPreferences;

    // ===========================================
    // 参照コンポーネント（既存システムとの連携用）
    // ===========================================
    
    /**
     * タスクマネージャー参照（情報取得用）
     */
    UPROPERTY()
    TObjectPtr<UTaskManagerComponent> TaskManagerRef;

    /**
     * チームコンポーネント参照（チーム情報取得用）
     */
    UPROPERTY()
    TObjectPtr<UTeamComponent> TeamComponentRef;

    /**
     * 移動コンポーネント参照（移動情報取得用）
     */
    UPROPERTY()
    TObjectPtr<ULocationMovementComponent> MovementComponentRef;

    /**
     * 所有キャラクター参照
     */
    UPROPERTY()
    TObjectPtr<AC_IdleCharacter> CharacterRef;

    // ===========================================
    // 初期化フラグ
    // ===========================================
    
    /**
     * 参照が正しく設定されているかのフラグ
     */
    bool bReferencesInitialized;

    // ===========================================
    // 内部ヘルパー関数
    // ===========================================
    
    /**
     * 性格別優先度設定の初期化
     */
    void InitializePersonalityPreferences();

    /**
     * 参照の有効性をチェック
     * @return 全ての必要な参照が有効ならtrue
     */
    bool AreReferencesValid() const;
};