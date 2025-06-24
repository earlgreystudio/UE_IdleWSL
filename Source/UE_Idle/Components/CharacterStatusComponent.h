#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/CharacterTypes.h"
#include "CharacterStatusComponent.generated.h"

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusChanged, const FCharacterStatus&, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClubTypeChanged, EClubType, NewClubType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTalentChanged, const FCharacterTalent&, NewTalent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDataUpdated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UCharacterStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterStatusComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ステータス
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
	FCharacterStatus Status;

	// 部活動
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
	EClubType ClubType;

	// 才能
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
	FCharacterTalent Talent;

	// ステータス取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	FCharacterStatus GetStatus() const { return Status; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	float GetCurrentHealth() const { return Status.CurrentHealth; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	float GetMaxHealth() const { return Status.MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetStatus(const FCharacterStatus& NewStatus);

	// 部活動取得・設定
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	EClubType GetClubType() const { return ClubType; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetClubType(EClubType NewClubType);

	// 才能取得・設定
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	FCharacterTalent GetTalent() const { return Talent; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetTalent(const FCharacterTalent& NewTalent);

	// 体力変更（個別）
	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetCurrentHealth(float NewHealth);

	// ======== イベントディスパッチャー ========
	
	// ステータス変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnStatusChanged OnStatusChanged;

	// 体力変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnHealthChanged OnHealthChanged;

	// 部活動変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnClubTypeChanged OnClubTypeChanged;

	// 才能変更時
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnTalentChanged OnTalentChanged;

	// 汎用データ更新通知
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterDataUpdated OnCharacterDataUpdated;
};