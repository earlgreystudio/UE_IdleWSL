#include "CharacterPresetManager.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/InventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UCharacterPresetManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // DataTableの自動検索（エディタでパスを設定している場合）
    if (!CharacterPresetDataTable)
    {
        CharacterPresetDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/CharacterPresets"));
        if (CharacterPresetDataTable)
        {
            UE_LOG(LogTemp, Log, TEXT("CharacterPresetDataTable loaded from default path"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterPresetDataTable not found at default path"));
        }
    }
    
    if (!LocationDataTable)
    {
        LocationDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/LocationData"));
        if (LocationDataTable)
        {
            UE_LOG(LogTemp, Log, TEXT("LocationDataTable loaded from default path"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("LocationDataTable not found at default path"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("CharacterPresetManager initialized"));
}

void UCharacterPresetManager::Deinitialize()
{
    Super::Deinitialize();
}

void UCharacterPresetManager::SetCharacterPresetDataTable(UDataTable* InDataTable)
{
    CharacterPresetDataTable = InDataTable;
    
    if (CharacterPresetDataTable)
    {
        UE_LOG(LogTemp, Log, TEXT("CharacterPresetDataTable set successfully. Row count: %d"), 
            CharacterPresetDataTable->GetRowMap().Num());
    }
}

FCharacterPresetDataRow UCharacterPresetManager::GetCharacterPreset(const FString& PresetId)
{
    if (!CharacterPresetDataTable)
    {
        LogCharacterPresetError(PresetId);
        return FCharacterPresetDataRow();
    }

    FCharacterPresetDataRow* FoundRow = CharacterPresetDataTable->FindRow<FCharacterPresetDataRow>(
        FName(*PresetId), 
        TEXT("GetCharacterPreset")
    );

    if (FoundRow)
    {
        return *FoundRow;
    }

    LogCharacterPresetError(PresetId);
    return FCharacterPresetDataRow();
}

bool UCharacterPresetManager::DoesPresetExist(const FString& PresetId) const
{
    if (!CharacterPresetDataTable)
    {
        return false;
    }

    return CharacterPresetDataTable->FindRow<FCharacterPresetDataRow>(
        FName(*PresetId), 
        TEXT("DoesPresetExist"), 
        false
    ) != nullptr;
}

TArray<FString> UCharacterPresetManager::GetAllPresetIds() const
{
    TArray<FString> PresetIds;
    
    if (CharacterPresetDataTable)
    {
        TArray<FName> RowNames = CharacterPresetDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            PresetIds.Add(RowName.ToString());
        }
    }
    
    return PresetIds;
}

TArray<FString> UCharacterPresetManager::GetEnemyPresetIds() const
{
    TArray<FString> EnemyPresetIds;
    
    if (CharacterPresetDataTable)
    {
        TArray<FName> RowNames = CharacterPresetDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            FCharacterPresetDataRow* Row = CharacterPresetDataTable->FindRow<FCharacterPresetDataRow>(
                RowName, 
                TEXT("GetEnemyPresetIds"), 
                false
            );
            
            if (Row && Row->bIsEnemy)
            {
                EnemyPresetIds.Add(RowName.ToString());
            }
        }
    }
    
    return EnemyPresetIds;
}

AC_IdleCharacter* UCharacterPresetManager::SpawnCharacterFromPreset(
    UObject* WorldContextObject,
    const FString& PresetId,
    const FVector& SpawnLocation,
    const FRotator& SpawnRotation,
    TSubclassOf<AC_IdleCharacter> CharacterClass)
{
    UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnCharacterFromPreset: Invalid world context"));
        return nullptr;
    }

    // プリセットデータ取得
    FCharacterPresetDataRow PresetData = GetCharacterPreset(PresetId);
    if (PresetData.Name.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnCharacterFromPreset: Preset not found: %s"), *PresetId);
        return nullptr;
    }

    // キャラクタークラス決定
    TSubclassOf<AC_IdleCharacter> ClassToSpawn = CharacterClass;
    if (!ClassToSpawn)
    {
        ClassToSpawn = AC_IdleCharacter::StaticClass();
    }

    // スポーン
    FActorSpawnParameters SpawnParams;
    AC_IdleCharacter* SpawnedCharacter = World->SpawnActor<AC_IdleCharacter>(
        ClassToSpawn, 
        SpawnLocation, 
        SpawnRotation, 
        SpawnParams
    );

    if (!SpawnedCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnCharacterFromPreset: Failed to spawn character"));
        return nullptr;
    }

    // キャラクター初期化
    InitializeCharacter(SpawnedCharacter, PresetData);

    return SpawnedCharacter;
}

void UCharacterPresetManager::InitializeCharacter(AC_IdleCharacter* Character, const FCharacterPresetDataRow& PresetData)
{
    if (!Character)
    {
        return;
    }

    // 名前設定
    Character->SetCharacterName(PresetData.Name);

    // ステータス設定
    if (UCharacterStatusComponent* StatusComp = Character->GetStatusComponent())
    {
        FCharacterTalent Talent = PresetData.CreateCharacterTalent();
        StatusComp->SetTalent(Talent);
        
        // ステータスを才能から計算（UCharacterStatusManagerが存在しない場合はデフォルト値を使用）
        FCharacterStatus Status;
        // TODO: UCharacterStatusManagerが実装されたら置き換え
        StatusComp->SetStatus(Status);
    }

    // 初期装備設定
    if (UInventoryComponent* InventoryComp = Character->GetInventoryComponent())
    {
        TMap<FString, int32> InitialItems = PresetData.ParseInitialItems();
        
        for (const auto& ItemPair : InitialItems)
        {
            bool bSuccess = InventoryComp->AddItem(ItemPair.Key, ItemPair.Value);
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Added %d x %s to %s's inventory"), 
                    ItemPair.Value, *ItemPair.Key, *PresetData.Name);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to add %s to %s's inventory"), 
                    *ItemPair.Key, *PresetData.Name);
            }
        }
    }

    // 敵の場合の追加処理
    if (PresetData.bIsEnemy)
    {
        // 敵はアクティブではない（TeamComponentに追加されない）
        // 必要に応じて敵専用の処理を追加
        UE_LOG(LogTemp, Log, TEXT("Spawned enemy character: %s"), *PresetData.Name);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Spawned ally character: %s"), *PresetData.Name);
    }
}

void UCharacterPresetManager::SetLocationDataTable(UDataTable* InDataTable)
{
    LocationDataTable = InDataTable;
    
    if (LocationDataTable)
    {
        UE_LOG(LogTemp, Log, TEXT("LocationDataTable set successfully. Row count: %d"), 
            LocationDataTable->GetRowMap().Num());
    }
}

FLocationDataRow UCharacterPresetManager::GetLocationData(const FString& LocationId)
{
    if (!LocationDataTable)
    {
        LogLocationError(LocationId);
        return FLocationDataRow();
    }

    FLocationDataRow* FoundRow = LocationDataTable->FindRow<FLocationDataRow>(
        FName(*LocationId), 
        TEXT("GetLocationData")
    );

    if (FoundRow)
    {
        return *FoundRow;
    }

    LogLocationError(LocationId);
    return FLocationDataRow();
}

FString UCharacterPresetManager::GetRandomEnemyFromLocation(const FString& LocationId)
{
    FLocationDataRow LocationData = GetLocationData(LocationId);
    return LocationData.GetRandomEnemyPreset();
}

void UCharacterPresetManager::LogCharacterPresetError(const FString& PresetId) const
{
    if (!CharacterPresetDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterPresetDataTable is not set!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Character preset not found: %s"), *PresetId);
    }
}

void UCharacterPresetManager::LogLocationError(const FString& LocationId) const
{
    if (!LocationDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("LocationDataTable is not set!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Location not found: %s"), *LocationId);
    }
}