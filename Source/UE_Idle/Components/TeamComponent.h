#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TeamComponent.generated.h"

class AC_IdleCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTeamComponent();

protected:
	virtual void BeginPlay() override;

public:
	// キャラクターリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	TArray<AC_IdleCharacter*> AllPlayerCharacters;

	// キャラクター追加
	UFUNCTION(BlueprintCallable, Category = "Team")
	void AddCharacter(AC_IdleCharacter* IdleCharacter);

	// キャラクターリスト取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	TArray<AC_IdleCharacter*> GetCharacterList() const;

	// キャラクター削除
	UFUNCTION(BlueprintCallable, Category = "Team")
	bool RemoveCharacter(AC_IdleCharacter* IdleCharacter);

	// キャラクター数取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	int32 GetCharacterCount() const;
};