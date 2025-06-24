#include "C_BP_GenerateCharacter.h"
#include "C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
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

	// 1. ランダム部活動取得
	EClubType RandomClub = UClubSystem::GetRandomClub();
	
	// 2. ランダム才能生成
	FCharacterTalent BaseTalent = UCharacterTalentGenerator::GenerateRandomTalent();
	
	// 3. 部活動ボーナス適用
	FCharacterTalent FinalTalent = UCharacterTalentGenerator::ApplyClubBonus(BaseTalent, RandomClub);
	
	// 4. ステータス計算
	FCharacterStatus CalculatedStatus = UCharacterStatusManager::CalculateMaxStatus(FinalTalent);
	
	// 5. ランダム名前生成
	FString RandomName = GenerateRandomName();
	
	// 6. キャラクタースポーン
	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = GetActorRotation();
	FActorSpawnParameters SpawnParams;
	
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(CharacterClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn character!"));
		return;
	}
	
	// 7. キャラクターデータ設定
	if (AC_IdleCharacter* IdleCharacter = Cast<AC_IdleCharacter>(SpawnedActor))
	{
		// 名前設定
		IdleCharacter->SetCharacterName(RandomName);
		
		// ステータスコンポーネントにデータ設定
		if (UCharacterStatusComponent* StatusComp = IdleCharacter->GetStatusComponent())
		{
			StatusComp->SetTalent(FinalTalent);
			StatusComp->SetStatus(CalculatedStatus);
			StatusComp->SetClubType(RandomClub);
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
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawned actor is not AC_IdleCharacter!"));
	}
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