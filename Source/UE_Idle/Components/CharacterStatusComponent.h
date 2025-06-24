#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/CharacterTypes.h"
#include "CharacterStatusComponent.generated.h"

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
	void SetStatus(const FCharacterStatus& NewStatus) { Status = NewStatus; }

	// 部活動取得・設定
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	EClubType GetClubType() const { return ClubType; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetClubType(EClubType NewClubType) { ClubType = NewClubType; }

	// 才能取得・設定
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Status")
	FCharacterTalent GetTalent() const { return Talent; }

	UFUNCTION(BlueprintCallable, Category = "Character Status")
	void SetTalent(const FCharacterTalent& NewTalent) { Talent = NewTalent; }
};