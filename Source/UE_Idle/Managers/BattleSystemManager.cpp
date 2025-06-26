#include "BattleSystemManager.h"
#include "../Components/LocationEventManager.h"
#include "../Components/CombatComponent.h"
#include "../Components/TeamComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../C_PlayerController.h"
#include "../Types/CombatTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UBattleSystemManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    CreateBattleSystemActor();
    
    UE_LOG(LogTemp, Log, TEXT("BattleSystemManager initialized"));
}

void UBattleSystemManager::Deinitialize()
{
    if (IsValid(BattleSystemActor))
    {
        BattleSystemActor->Destroy();
        BattleSystemActor = nullptr;
    }
    
    LocationEventManager = nullptr;
    CombatComponent = nullptr;
    
    Super::Deinitialize();
}

void UBattleSystemManager::CreateBattleSystemActor()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("BattleSystemManager: No world available"));
        return;
    }
    
    // 戦闘システム管理用Actorを生成
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = TEXT("BattleSystemActor");
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    BattleSystemActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    
    if (!BattleSystemActor)
    {
        UE_LOG(LogTemp, Error, TEXT("BattleSystemManager: Failed to create BattleSystemActor"));
        return;
    }
    
    // LocationEventManagerを追加
    LocationEventManager = NewObject<ULocationEventManager>(BattleSystemActor, TEXT("LocationEventManager"));
    if (LocationEventManager)
    {
        LocationEventManager->RegisterComponent();
    }
    
    // CombatComponentを追加
    CombatComponent = NewObject<UCombatComponent>(BattleSystemActor, TEXT("CombatComponent"));
    if (CombatComponent)
    {
        CombatComponent->RegisterComponent();
        
        // 戦闘終了イベントをバインド
        CombatComponent->OnCombatCompleted.AddDynamic(this, &UBattleSystemManager::OnCombatCompleted);
    }
    
    UE_LOG(LogTemp, Log, TEXT("BattleSystemManager: Created BattleSystemActor with components"));
}

bool UBattleSystemManager::StartTeamAdventure(const TArray<AC_IdleCharacter*>& TeamMembers, const FString& LocationId)
{
    if (!LocationEventManager)
    {
        UE_LOG(LogTemp, Error, TEXT("StartTeamAdventure: LocationEventManager not available"));
        return false;
    }
    
    // LocationEventManagerに戦闘イベントをトリガー
    return LocationEventManager->TriggerCombatEvent(LocationId, TeamMembers);
}

bool UBattleSystemManager::IsTeamInCombat(const TArray<AC_IdleCharacter*>& TeamMembers) const
{
    if (!CombatComponent)
    {
        return false;
    }
    
    // 戦闘中かチェック
    ECombatState CombatState = CombatComponent->GetCombatState();
    if (CombatState == ECombatState::Inactive || CombatState == ECombatState::Completed)
    {
        return false;
    }
    
    // チームメンバーが戦闘に参加しているかチェック
    TArray<AC_IdleCharacter*> AllyTeam = CombatComponent->GetAllyTeam();
    for (AC_IdleCharacter* Member : TeamMembers)
    {
        if (AllyTeam.Contains(Member))
        {
            return true;
        }
    }
    
    return false;
}

void UBattleSystemManager::OnCombatCompleted(const TArray<AC_IdleCharacter*>& Winners, const TArray<AC_IdleCharacter*>& Losers, float Duration)
{
    UE_LOG(LogTemp, Log, TEXT("BattleSystemManager::OnCombatCompleted - Combat ended"));
    
    // PlayerControllerを取得
    UWorld* World = GetWorld();
    if (World)
    {
        AC_PlayerController* PlayerController = Cast<AC_PlayerController>(World->GetFirstPlayerController());
        if (PlayerController)
        {
            // TeamComponentを取得してOnCombatEndを呼ぶ
            if (UTeamComponent* TeamComp = PlayerController->GetTeamComponent_Implementation())
            {
                TeamComp->OnCombatEnd(Winners, Losers);
                UE_LOG(LogTemp, Log, TEXT("BattleSystemManager: Called TeamComponent->OnCombatEnd"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("BattleSystemManager: TeamComponent not found on PlayerController"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("BattleSystemManager: PlayerController not found"));
        }
    }
}