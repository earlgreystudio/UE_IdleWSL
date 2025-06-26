#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerControllerInterface.generated.h"

class UInventoryComponent;
class AC_IdleCharacter;
class UTeamComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class UPlayerControllerInterface : public UInterface
{
	GENERATED_BODY()
};

class UE_IDLE_API IPlayerControllerInterface
{
	GENERATED_BODY()

public:
	// アイテムをストレージに追加
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Controller Interface")
	void AddItemToStorage(const FString& ItemId, int32 Quantity);
	virtual void AddItemToStorage_Implementation(const FString& ItemId, int32 Quantity) {}

	// キャラクターを追加（シンプル版）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Controller Interface")
	void AddCharacter(AActor* NewCharacter);
	virtual void AddCharacter_Implementation(AActor* NewCharacter) {}

	// GlobalInventoryComponent取得
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Controller Interface")
	UInventoryComponent* GetGlobalInventoryComp();
	virtual UInventoryComponent* GetGlobalInventoryComp_Implementation() { return nullptr; }

	// キャラクターリスト取得（シンプル版）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Controller Interface")
	TArray<AActor*> GetCharacterList();
	virtual TArray<AActor*> GetCharacterList_Implementation() { return TArray<AActor*>(); }

	// TeamComponent取得
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Controller Interface")
	UTeamComponent* GetTeamComponent();
	virtual UTeamComponent* GetTeamComponent_Implementation() { return nullptr; }

};