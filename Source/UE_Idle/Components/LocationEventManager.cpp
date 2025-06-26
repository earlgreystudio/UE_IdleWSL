#include "LocationEventManager.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Managers/CharacterPresetManager.h"
#include "CombatComponent.h"
#include "ActionSystemComponent.h"
#include "Engine/World.h"
#include "../Types/LocationTypes.h"

ULocationEventManager::ULocationEventManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    MaxEnemiesPerTeam = 3;
    EnemySpawnRadius = 500.0f;
    PresetManager = nullptr;
    CombatComponent = nullptr;
}

void ULocationEventManager::BeginPlay()
{
    Super::BeginPlay();
    
    // CharacterPresetManagerの取得
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        PresetManager = GameInstance->GetSubsystem<UCharacterPresetManager>();
        if (!PresetManager)
        {
            UE_LOG(LogTemp, Error, TEXT("LocationEventManager: Failed to get CharacterPresetManager"));
        }
    }

    // CombatComponentの取得
    if (AActor* Owner = GetOwner())
    {
        CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
        if (!CombatComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("LocationEventManager: CombatComponent not found on owner"));
        }
    }
}

bool ULocationEventManager::TriggerCombatEvent(const FString& LocationId, const TArray<AC_IdleCharacter*>& AllyTeam)
{
    if (AllyTeam.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEvent: Empty ally team"));
        return false;
    }

    if (!CanTriggerCombatAtLocation(LocationId))
    {
        UE_LOG(LogTemp, Warning, TEXT("TriggerCombatEvent: No enemies available at location %s"), *LocationId);
        return false;
    }

    // 敵チーム生成
    FVector SpawnLocation = GetOwner()->GetActorLocation();
    TArray<AC_IdleCharacter*> EnemyTeam = CreateEnemyTeamForLocation(
        GetOwner(), LocationId, SpawnLocation, 1);

    if (EnemyTeam.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEvent: Failed to create enemy team"));
        return false;
    }

    // 敵チーム登録
    RegisterEnemyTeam(EnemyTeam, LocationId);

    // CombatComponentに戦闘開始を委託
    if (CombatComponent)
    {
        // TeamをFTeam構造体に変換してCombatComponentに渡す
        FTeam AllyTeamStruct;
        AllyTeamStruct.Members = AllyTeam;
        AllyTeamStruct.TeamName = TEXT("派遣チーム");
        AllyTeamStruct.bInCombat = true;

        bool bCombatStarted = CombatComponent->StartCombat(AllyTeamStruct, EnemyTeam, LocationId);
        if (!bCombatStarted)
        {
            UE_LOG(LogTemp, Error, TEXT("TriggerCombatEvent: Failed to start combat"));
            UnregisterEnemyTeam(EnemyTeam);
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TriggerCombatEvent: CombatComponent not available"));
        UnregisterEnemyTeam(EnemyTeam);
        return false;
    }

    // イベント通知
    OnLocationEventTriggered.Broadcast(LocationId, TEXT("Combat"), AllyTeam);

    UE_LOG(LogTemp, Log, TEXT("Combat event triggered at location %s with %d allies vs %d enemies"), 
        *LocationId, AllyTeam.Num(), EnemyTeam.Num());

    return true;
}

bool ULocationEventManager::TriggerGatheringEvent(const FString& LocationId, const TArray<AC_IdleCharacter*>& AllyTeam)
{
    // 将来拡張用の採取イベント
    UE_LOG(LogTemp, Log, TEXT("Gathering event triggered at location %s (not implemented yet)"), *LocationId);
    
    OnLocationEventTriggered.Broadcast(LocationId, TEXT("Gathering"), AllyTeam);
    return true;
}

TArray<AC_IdleCharacter*> ULocationEventManager::CreateEnemyTeamForLocation(
    UObject* WorldContextObject,
    const FString& LocationId,
    const FVector& SpawnLocation,
    int32 EnemyCount)
{
    TArray<AC_IdleCharacter*> EnemyTeam;

    if (!PresetManager || !WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateEnemyTeamForLocation: Invalid PresetManager or WorldContext"));
        return EnemyTeam;
    }

    for (int32 i = 0; i < EnemyCount; i++)
    {
        // 場所からランダムな敵を選択
        FString EnemyPresetId = SelectRandomEnemyForLocation(LocationId);
        
        if (EnemyPresetId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateEnemyTeamForLocation: No enemy preset found for location %s"), *LocationId);
            continue;
        }

        // スポーン位置計算
        FVector EnemySpawnLocation = CalculateEnemySpawnPosition(SpawnLocation, EnemySpawnRadius, i);
        FRotator EnemySpawnRotation = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

        // 敵をスポーン
        AC_IdleCharacter* Enemy = PresetManager->SpawnCharacterFromPreset(
            WorldContextObject, EnemyPresetId, EnemySpawnLocation, EnemySpawnRotation);

        if (Enemy)
        {
            EnemyTeam.Add(Enemy);
            UE_LOG(LogTemp, Log, TEXT("Created enemy %s for location %s"), *EnemyPresetId, *LocationId);
        }
    }

    // イベント通知
    if (EnemyTeam.Num() > 0)
    {
        OnEnemyTeamCreated.Broadcast(EnemyTeam, LocationId);
    }

    return EnemyTeam;
}

FLocationDataRow ULocationEventManager::GetLocationData(const FString& LocationId) const
{
    if (PresetManager)
    {
        return PresetManager->GetLocationData(LocationId);
    }
    
    return FLocationDataRow();
}

FString ULocationEventManager::GetLocationDisplayName(const FString& LocationId) const
{
    FLocationDataRow LocationData = GetLocationData(LocationId);
    return LocationData.Name.IsEmpty() ? LocationId : LocationData.Name;
}

TArray<FString> ULocationEventManager::GetAllLocationIds() const
{
    // ハードコードされた場所ID
    return { TEXT("base"), TEXT("plains"), TEXT("swamp"), TEXT("cave") };
}

TArray<FString> ULocationEventManager::GetValidLocationIds() const
{
    TArray<FString> LocationIds;
    
    if (PresetManager)
    {
        // PresetManagerからDataTableのRowNameを取得する処理を追加予定
        // 現在はハードコードされた値を返す
        return GetAllLocationIds();
    }
    
    return LocationIds;
}

FString ULocationEventManager::LocationTypeToString(ELocationType LocationType)
{
    switch (LocationType)
    {
        case ELocationType::Base:
            return TEXT("base");
        case ELocationType::Plains:
            return TEXT("plains");
        case ELocationType::Swamp:
            return TEXT("swamp");
        case ELocationType::Cave:
            return TEXT("cave");
        default:
            return TEXT("base");
    }
}

TArray<ELocationType> ULocationEventManager::GetAllLocationTypes()
{
    TArray<ELocationType> LocationTypes;
    
    // ELocationType::Count を除く全てのタイプを追加
    for (int32 i = 0; i < (int32)ELocationType::Count; i++)
    {
        LocationTypes.Add(static_cast<ELocationType>(i));
    }
    
    return LocationTypes;
}

bool ULocationEventManager::CanTriggerCombatAtLocation(const FString& LocationId) const
{
    FLocationDataRow LocationData = GetLocationData(LocationId);
    return !LocationData.EnemySpawnListString.IsEmpty();
}

void ULocationEventManager::RegisterEnemyTeam(const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationId)
{
    if (EnemyTeam.Num() > 0)
    {
        ActiveEnemyTeams.Add(LocationId, EnemyTeam);
        UE_LOG(LogTemp, Log, TEXT("Registered enemy team (%d enemies) for location %s"), 
            EnemyTeam.Num(), *LocationId);
    }
}

void ULocationEventManager::UnregisterEnemyTeam(const TArray<AC_IdleCharacter*>& EnemyTeam)
{
    for (auto& Pair : ActiveEnemyTeams)
    {
        if (Pair.Value == EnemyTeam)
        {
            UE_LOG(LogTemp, Log, TEXT("Starting enemy team cleanup for location %s"), *Pair.Key);
            
            // Step 1: ActionSystemから敵キャラクターを事前削除
            if (UWorld* World = GetWorld())
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    if (UActionSystemComponent* ActionComp = PC->FindComponentByClass<UActionSystemComponent>())
                    {
                        for (AC_IdleCharacter* Enemy : EnemyTeam)
                        {
                            if (IsValid(Enemy))
                            {
                                UE_LOG(LogTemp, Log, TEXT("Pre-removing enemy %s from ActionSystem"), *Enemy->GetName());
                                ActionComp->UnregisterCharacter(Enemy);
                            }
                        }
                    }
                }
            }
            
            // Step 2: 短時間後に敵キャラクターを削除（ActionSystem処理完了を待つ）
            if (GetWorld())
            {
                FTimerHandle EnemyDestroyTimerHandle;
                GetWorld()->GetTimerManager().SetTimer(EnemyDestroyTimerHandle, [this, EnemyTeam, LocationId = Pair.Key]()
                {
                    UE_LOG(LogTemp, Log, TEXT("Destroying enemy characters for location %s"), *LocationId);
                    for (AC_IdleCharacter* Enemy : EnemyTeam)
                    {
                        if (IsValid(Enemy))
                        {
                            UE_LOG(LogTemp, Log, TEXT("Destroying enemy %s"), *Enemy->GetName());
                            Enemy->Destroy();
                        }
                    }
                }, 0.3f, false);
            }
            
            ActiveEnemyTeams.Remove(Pair.Key);
            UE_LOG(LogTemp, Log, TEXT("Unregistered enemy team from location %s"), *Pair.Key);
            break;
        }
    }
}

void ULocationEventManager::ClearAllEnemyTeams()
{
    for (auto& Pair : ActiveEnemyTeams)
    {
        for (AC_IdleCharacter* Enemy : Pair.Value)
        {
            if (IsValid(Enemy))
            {
                Enemy->Destroy();
            }
        }
    }
    
    ActiveEnemyTeams.Empty();
    UE_LOG(LogTemp, Log, TEXT("Cleared all enemy teams"));
}

FVector ULocationEventManager::CalculateEnemySpawnPosition(const FVector& CenterLocation, float Radius, int32 Index) const
{
    // 敵を円形に配置
    float Angle = (360.0f / MaxEnemiesPerTeam) * Index * (PI / 180.0f);
    float Distance = FMath::RandRange(Radius * 0.5f, Radius);
    
    FVector Offset(
        Distance * FMath::Cos(Angle),
        Distance * FMath::Sin(Angle),
        0.0f
    );
    
    return CenterLocation + Offset;
}

FString ULocationEventManager::SelectRandomEnemyForLocation(const FString& LocationId) const
{
    if (PresetManager)
    {
        return PresetManager->GetRandomEnemyFromLocation(LocationId);
    }
    
    return TEXT("");
}