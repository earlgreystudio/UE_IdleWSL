#include "C_BP_GenerateCharacter.h"
#include "C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"
#include "../Managers/CharacterPresetManager.h"
#include "../CharacterGenerator/SpecialtySystem.h"
#include "../CharacterGenerator/CharacterTalent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AC_BP_GenerateCharacter::AC_BP_GenerateCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// デフォルトでC_IdleCharacterを指定（Blueprintで変更可能）
	CharacterClassToSpawn = AC_IdleCharacter::StaticClass();
	
	// 名前リスト初期化
	InitializeNameLists();
}

void AC_BP_GenerateCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AC_BP_GenerateCharacter::GenerateRandomCharacter()
{
	if (!CharacterClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("CharacterClassToSpawn is not set!"));
		return;
	}

	// 1. ランダム専門性取得
	ESpecialtyType RandomSpecialty = USpecialtySystem::GetRandomSpecialty();
	
	// 2. ランダム才能生成
	FCharacterTalent BaseTalent = UCharacterTalentGenerator::GenerateRandomTalent();
	
	// 3. 専門性ボーナス適用
	FCharacterTalent FinalTalent = UCharacterTalentGenerator::ApplySpecialtyBonus(BaseTalent, RandomSpecialty);
	
	// 4. ステータス計算
	FCharacterStatus CalculatedStatus = UCharacterStatusManager::CalculateMaxStatus(FinalTalent);
	
	// 5. ランダム名前生成
	FString RandomName = GenerateRandomName();
	
	// 6. キャラクタースポーン
	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = GetActorRotation();
	FActorSpawnParameters SpawnParams;
	
	AC_IdleCharacter* SpawnedActor = GetWorld()->SpawnActor<AC_IdleCharacter>(CharacterClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn character!"));
		return;
	}
	
	// 7. キャラクターデータ設定
	if (SpawnedActor) // 既にAC_IdleCharacter*なのでキャスト不要
	{
		// 名前設定
		SpawnedActor->SetCharacterName(RandomName);
		
		// ステータスコンポーネントにデータ設定
		if (UCharacterStatusComponent* StatusComp = SpawnedActor->GetStatusComponent())
		{
			StatusComp->SetTalent(FinalTalent);
			StatusComp->SetStatus(CalculatedStatus);
			StatusComp->SetSpecialtyType(RandomSpecialty);
			UE_LOG(LogTemp, Log, TEXT("Character %s StatusComponent configured successfully"), *RandomName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Character %s has no StatusComponent!"), *RandomName);
		}
		
		// 8. PlayerControllerに追加
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (PC->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass()))
			{
				IPlayerControllerInterface::Execute_AddCharacter(PC, SpawnedActor);
				UE_LOG(LogTemp, Log, TEXT("Generated character: %s"), *RandomName);
			}
		}
	}
	// SpawnedActorがnullの場合は既に上でログ出力済み
}

FString AC_BP_GenerateCharacter::GenerateRandomName()
{
	if (LastNames.Num() == 0)
	{
		return TEXT("Unknown Character");
	}
	
	// ランダムに苗字を選択
	FString LastName = LastNames[FMath::RandRange(0, LastNames.Num() - 1)];
	
	return LastName;
}

void AC_BP_GenerateCharacter::InitializeNameLists()
{
	// 一般的な日本の姓
	LastNames = {
		TEXT("田中"), TEXT("佐藤"), TEXT("鈴木"), TEXT("高橋"), TEXT("渡辺"),
		TEXT("伊藤"), TEXT("山本"), TEXT("中村"), TEXT("小林"), TEXT("加藤"),
		TEXT("吉田"), TEXT("山田"), TEXT("佐々木"), TEXT("山口"), TEXT("松本"),
		TEXT("井上"), TEXT("木村"), TEXT("林"), TEXT("斎藤"), TEXT("清水"),
		TEXT("石川"), TEXT("森"), TEXT("池田"), TEXT("橋本"), TEXT("山崎"),
		TEXT("石井"), TEXT("坂本"), TEXT("前田"), TEXT("近藤"), TEXT("村上")
	};
}

void AC_BP_GenerateCharacter::GenerateCharacterFromPreset(const FString& PresetId)
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCharacterFromPreset: Invalid world"));
		return;
	}

	// CharacterPresetManagerの取得
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCharacterFromPreset: Invalid GameInstance"));
		return;
	}

	UCharacterPresetManager* PresetManager = GameInstance->GetSubsystem<UCharacterPresetManager>();
	if (!PresetManager)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCharacterFromPreset: Failed to get CharacterPresetManager"));
		return;
	}

	// プリセットデータ取得
	FCharacterPresetDataRow PresetData = PresetManager->GetCharacterPreset(PresetId);
	if (PresetData.Name.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCharacterFromPreset: Preset not found: %s"), *PresetId);
		return;
	}

	// キャラクタークラス決定
	TSubclassOf<AC_IdleCharacter> ClassToSpawn;
	if (CharacterClassToSpawn)
	{
		ClassToSpawn = CharacterClassToSpawn;
	}
	else
	{
		ClassToSpawn = AC_IdleCharacter::StaticClass();
	}

	// スポーン位置
	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = GetActorRotation();

	// キャラクタースポーン
	AC_IdleCharacter* SpawnedCharacter = PresetManager->SpawnCharacterFromPreset(
		this,
		PresetId,
		SpawnLocation,
		SpawnRotation,
		ClassToSpawn
	);

	if (!SpawnedCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateCharacterFromPreset: Failed to spawn character"));
		return;
	}

	// 敵でない場合はPlayerControllerに追加
	if (!PresetData.bIsEnemy)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (PC->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass()))
			{
				IPlayerControllerInterface::Execute_AddCharacter(PC, SpawnedCharacter);
				UE_LOG(LogTemp, Log, TEXT("Generated character from preset: %s"), *PresetId);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Generated enemy from preset: %s"), *PresetId);
	}
}

AC_IdleCharacter* AC_BP_GenerateCharacter::GenerateEnemyAtLocation(
	UObject* WorldContextObject,
	const FString& LocationId,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateEnemyAtLocation: Invalid WorldContextObject"));
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateEnemyAtLocation: Invalid world"));
		return nullptr;
	}

	// CharacterPresetManagerの取得
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateEnemyAtLocation: Invalid GameInstance"));
		return nullptr;
	}

	UCharacterPresetManager* PresetManager = GameInstance->GetSubsystem<UCharacterPresetManager>();
	if (!PresetManager)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateEnemyAtLocation: Failed to get CharacterPresetManager"));
		return nullptr;
	}

	// 場所からランダムな敵を選択
	FString EnemyPresetId = PresetManager->GetRandomEnemyFromLocation(LocationId);
	if (EnemyPresetId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerateEnemyAtLocation: No enemies available at location %s"), *LocationId);
		return nullptr;
	}

	// キャラクタークラス決定
	TSubclassOf<AC_IdleCharacter> ClassToSpawn;
	if (CharacterClassToSpawn)
	{
		ClassToSpawn = CharacterClassToSpawn;
	}
	else
	{
		ClassToSpawn = AC_IdleCharacter::StaticClass();
	}

	// 敵をスポーン
	AC_IdleCharacter* SpawnedEnemy = PresetManager->SpawnCharacterFromPreset(
		WorldContextObject,
		EnemyPresetId,
		SpawnLocation,
		SpawnRotation,
		ClassToSpawn
	);

	if (SpawnedEnemy)
	{
		UE_LOG(LogTemp, Log, TEXT("Generated enemy %s at location %s"), *EnemyPresetId, *LocationId);
	}

	return SpawnedEnemy;
}