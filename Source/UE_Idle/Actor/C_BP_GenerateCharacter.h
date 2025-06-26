#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Types/CharacterTypes.h"
#include "../CharacterGenerator/SpecialtySystem.h"
#include "../CharacterGenerator/CharacterTalent.h"
#include "../CharacterGenerator/CharacterStatus.h"
#include "../Interfaces/PlayerControllerInterface.h"
#include "Engine/World.h"
#include "C_BP_GenerateCharacter.generated.h"

class AC_IdleCharacter;

UCLASS()
class UE_IDLE_API AC_BP_GenerateCharacter : public AActor
{
	GENERATED_BODY()
	
public:	
	AC_BP_GenerateCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	// ランダムキャラクター生成のメイン関数
	UFUNCTION(BlueprintCallable, Category = "Character Generation")
	void GenerateRandomCharacter();

	// プリセットからキャラクター生成
	UFUNCTION(BlueprintCallable, Category = "Character Generation")
	void GenerateCharacterFromPreset(const FString& PresetId);

	// 場所にランダムな敵を生成
	UFUNCTION(BlueprintCallable, Category = "Character Generation", meta = (WorldContext = "WorldContextObject"))
	AC_IdleCharacter* GenerateEnemyAtLocation(
		UObject* WorldContextObject,
		const FString& LocationId,
		const FVector& SpawnLocation,
		const FRotator& SpawnRotation = FRotator::ZeroRotator
	);

	// スポーンするキャラクタークラス（BP_IdleCharacterを指定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Generation")
	TSubclassOf<AC_IdleCharacter> CharacterClassToSpawn;

private:
	// ランダム名前生成
	FString GenerateRandomName();

	// 日本語苗字リスト
	TArray<FString> LastNames;

	// 名前リストの初期化
	void InitializeNameLists();
};