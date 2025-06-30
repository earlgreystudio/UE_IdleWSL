// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "../Types/ItemTypes.h"
#include "../Types/CharacterTypes.h"
#include "../Types/TeamTypes.h"
#include "../Types/TaskTypes.h"
#include "C_IdleCharacter.generated.h"

class UCharacterStatusComponent;
class UInventoryComponent;
class UCharacterBrain;

UCLASS()
class UE_IDLE_API AC_IdleCharacter : public AActor, public IIdleCharacterInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_IdleCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// IIdleCharacterInterface Implementation
	virtual FString GetCharacterName_Implementation() override;
	virtual bool IsActive_Implementation() override;
	virtual UCharacterStatusComponent* GetCharacterStatusComponent_Implementation() override;
	virtual UInventoryComponent* GetCharacterInventoryComponent_Implementation() override;
	virtual FCharacterTalent GetCharacterTalent_Implementation() override;

	// ステータス関連（安全性チェック付き）
	UFUNCTION(BlueprintCallable, Category = "Character")
	UCharacterStatusComponent* GetStatusComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	UInventoryComponent* GetInventoryComponent() const;

	// キャラクター名取得・設定
	UFUNCTION(BlueprintCallable, Category = "Character")
	FString GetName() const { return CharacterName; }
	
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetCharacterName(const FString& NewName) { CharacterName = NewName; }

	// キャラクター種族取得・設定
	UFUNCTION(BlueprintCallable, Category = "Character")
	FString GetCharacterRace() const { return CharacterRace; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetCharacterRace(const FString& NewRace) { CharacterRace = NewRace; }

	// 派生ステータス取得
	UFUNCTION(BlueprintCallable, Category = "Character")
	FDerivedStats GetDerivedStats() const;

	// ===========================================
	// 自律的キャラクターシステム（Phase 2）
	// ===========================================
	
	/**
	 * ターン開始時に呼ばれる自律的処理のメインエントリポイント
	 * TimeManagerから呼ばれる唯一のメソッド
	 * @param CurrentTurn 現在のターン番号
	 */
	UFUNCTION(BlueprintCallable, Category = "Autonomous Character")
	void OnTurnTick(int32 CurrentTurn);

	/**
	 * キャラクターの性格を設定
	 * @param NewPersonality 新しい性格
	 */
	UFUNCTION(BlueprintCallable, Category = "Autonomous Character")
	void SetPersonality(ECharacterPersonality NewPersonality);

	/**
	 * 現在の性格を取得
	 * @return 現在の性格
	 */
	UFUNCTION(BlueprintCallable, Category = "Autonomous Character")
	ECharacterPersonality GetPersonality() const { return MyPersonality; }

	/**
	 * 現在の状況データを取得
	 * @return 現在の状況
	 */
	UFUNCTION(BlueprintCallable, Category = "Autonomous Character")
	FCharacterSituation GetCurrentSituation() const { return CurrentSituation; }

	/**
	 * 計画された行動を取得
	 * @return 計画された行動
	 */
	UFUNCTION(BlueprintCallable, Category = "Autonomous Character")
	FCharacterAction GetPlannedAction() const { return PlannedAction; }

	/**
	 * CharacterBrainの参照を取得
	 * @return CharacterBrainの参照
	 */
	UFUNCTION(BlueprintCallable, Category = "Autonomous Character")
	UCharacterBrain* GetMyBrain() const { return MyBrain; }

protected:
	// ===========================================
	// 自律的判断プロセス（内部実装）
	// ===========================================
	
	/**
	 * 現在の状況を分析し、CurrentSituationを更新
	 */
	void AnalyzeMySituation();

	/**
	 * チームメンバーとの情報共有・調整
	 */
	void ConsultMyTeam();

	/**
	 * 状況に基づいて最適な行動を決定
	 */
	void DecideMyAction();

	/**
	 * 決定された行動を実行
	 */
	void ExecuteMyAction();
	
	/**
	 * 行動の進行状況をチェックし、完了時に次のステップに進む
	 */
	void CheckAndUpdateActionProgress();

	/**
	 * 移動アクションを実行
	 */
	void ExecuteMovementAction();

	/**
	 * 採集アクションを実行
	 */
	void ExecuteGatheringAction();

	/**
	 * 戦闘アクションを実行
	 */
	void ExecuteCombatAction();

	/**
	 * 拠点帰還アクションを実行
	 */
	void ExecuteReturnAction();
	
	/**
	 * 荷下ろしアクションを実行
	 */
	void ExecuteUnloadAction();

	// ===========================================
	// Phase 2.3: チーム連携用ヘルパーメソッド
	// ===========================================
	
	/**
	 * チーム調整が必要な場合の代替行動を決定
	 * @return 調整された行動
	 */
	FCharacterAction DetermineAdjustedAction();
	
	/**
	 * 元の行動をチーム戦略に合わせて調整
	 * @param OriginalAction 元の行動
	 * @return チーム戦略に沿った行動
	 */
	FCharacterAction AlignActionWithTeamStrategy(const FCharacterAction& OriginalAction);

protected:
	// Equipment change handlers
	UFUNCTION()
	void HandleItemEquipped(const FString& ItemId, EEquipmentSlot Slot);
	
	UFUNCTION()
	void HandleItemUnequipped(const FString& ItemId, EEquipmentSlot Slot);

protected:
	// コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCharacterStatusComponent> StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	// キャラクター基本データ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FString CharacterName = TEXT("Idle Character");

	// キャラクターの種族（CharacterPresets.csvのRowNameに対応）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FString CharacterRace = TEXT("human");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	bool bIsActive = true;

	// ===========================================
	// 自律的キャラクターシステムのプロパティ
	// ===========================================
	
	/**
	 * このキャラクターの思考エンジン
	 * 状況分析と行動決定を担当
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Autonomous Character")
	TObjectPtr<UCharacterBrain> MyBrain;

	/**
	 * キャラクターの性格タイプ
	 * 行動判定に影響を与える
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autonomous Character")
	ECharacterPersonality MyPersonality;

	/**
	 * 現在の状況分析結果
	 * AnalyzeMySituation()で更新される
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Autonomous Character") 
	FCharacterSituation CurrentSituation;

	/**
	 * 決定された次の行動
	 * DecideMyAction()で設定される
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Autonomous Character")
	FCharacterAction PlannedAction;

	/**
	 * 自律システムが有効かどうか
	 * falseの場合は従来のシステムを使用
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autonomous Character")
	bool bAutonomousSystemEnabled;

	/**
	 * デバッグ情報を表示するか
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autonomous Character")
	bool bShowDebugInfo;

};
