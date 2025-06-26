#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../Types/CharacterTypes.h"
#include "IdleCharacterInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UIdleCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

class UE_IDLE_API IIdleCharacterInterface
{
	GENERATED_BODY()

public:
	// キャラクター名取得
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
	FString GetCharacterName();
	virtual FString GetCharacterName_Implementation() { return TEXT("Unknown"); }



	// アクティブ状態
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
	bool IsActive();
	virtual bool IsActive_Implementation() { return true; }

	// コンポーネント取得
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
	class UCharacterStatusComponent* GetCharacterStatusComponent();
	virtual class UCharacterStatusComponent* GetCharacterStatusComponent_Implementation() { return nullptr; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
	class UInventoryComponent* GetCharacterInventoryComponent();
	virtual class UInventoryComponent* GetCharacterInventoryComponent_Implementation() { return nullptr; }

	// 才能取得
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
	struct FCharacterTalent GetCharacterTalent();
	virtual struct FCharacterTalent GetCharacterTalent_Implementation() { return FCharacterTalent(); }
};