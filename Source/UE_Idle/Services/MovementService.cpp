#include "MovementService.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/LocationMovementComponent.h"
#include "../Components/TeamComponent.h"
#include "../C_PlayerController.h"
#include "../Types/CommonTypes.h"
#include "../Types/ItemTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UMovementService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 統計情報初期化
    ResetStatistics();
    
    // キャッシュ初期化
    DistanceCache.Empty();
    LastCacheUpdate = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("🚶 MovementService initialized successfully"));
}

void UMovementService::Deinitialize()
{
    // クリーンアップ
    DistanceCache.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("🚶 MovementService deinitialized"));
    
    Super::Deinitialize();
}

bool UMovementService::MoveCharacterToLocation(AC_IdleCharacter* Character, const FString& TargetLocation)
{
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Error, TEXT("🚶❌ MovementService: Invalid character provided"));
        return false;
    }

    if (TargetLocation.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("🚶❌ MovementService: Empty target location"));
        return false;
    }

    // 統計更新
    TotalMovementRequests++;
    
    // 既存の移動ロジックを実行
    bool bSuccess = ExecuteExistingMovementLogic(Character, TargetLocation);
    
    if (bSuccess)
    {
        SuccessfulMovements++;
        
        // 移動時間を統計に追加
        float MovementTime = CalculateMovementTime(Character, TargetLocation);
        TotalMovementTime += MovementTime;
        
        UE_LOG(LogTemp, VeryVerbose, 
            TEXT("🚶✅ MovementService: Character %s moving to %s (%.1f seconds)"),
            *IIdleCharacterInterface::Execute_GetCharacterName(Character), *TargetLocation, MovementTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("🚶⚠️ MovementService: Failed to move character %s to %s"),
            *IIdleCharacterInterface::Execute_GetCharacterName(Character), *TargetLocation);
    }
    
    return bSuccess;
}

float UMovementService::CalculateMovementTime(AC_IdleCharacter* Character, const FString& TargetLocation)
{
    if (!IsValid(Character))
    {
        return 0.0f;
    }
    
    // 現在地を取得
    FString CurrentLocation = GetCharacterCurrentLocation(Character);
    
    // 既存の移動時間計算を使用
    return GetExistingMovementTime(CurrentLocation, TargetLocation);
}

FString UMovementService::GetCharacterCurrentLocation(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return TEXT("base"); // デフォルト
    }
    
    // チームインデックスを取得
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚶⚠️ MovementService: Character not in any team, defaulting to base"));
        return TEXT("base");
    }
    
    // LocationMovementComponentから現在地を取得
    ULocationMovementComponent* MovementComp = GetLocationMovementComponent();
    if (!MovementComp)
    {
        return TEXT("base");
    }
    
    // 移動状態をチェック
    EMovementState MovementState = MovementComp->GetMovementState(TeamIndex);
    
    if (MovementState == EMovementState::MovingToDestination || MovementState == EMovementState::MovingToBase)
    {
        FMovementInfo MovementInfo = MovementComp->GetMovementInfo(TeamIndex);
        UE_LOG(LogTemp, VeryVerbose, TEXT("🚶📍 GetCharacterCurrentLocation: Character is moving to %s"), 
            *MovementInfo.ToLocation);
            
        // 拠点への移動中で進捗が90%以上なら拠点とみなす
        if (MovementInfo.ToLocation == TEXT("base") && MovementInfo.Progress >= 0.9f)
        {
            UE_LOG(LogTemp, Warning, TEXT("🚶🏠 GetCharacterCurrentLocation: Almost at base (%d%%), treating as base"), 
                (int32)(MovementInfo.Progress * 100));
            return TEXT("base");
        }
        
        // それ以外は出発地を現在地とする
        return MovementInfo.FromLocation;
    }
    
    // 静止中の場合は距離ベースで判定
    float DistanceFromBase = MovementComp->GetCurrentDistanceFromBase(TeamIndex);
    
    if (DistanceFromBase <= 1.0f)
    {
        return TEXT("base");
    }
    else if (FMath::IsNearlyEqual(DistanceFromBase, 100.0f, 1.0f))
    {
        return TEXT("plains");
    }
    else
    {
        // 中間地点の場合（通常はありえないが念のため）
        return (DistanceFromBase < 50.0f) ? TEXT("base") : TEXT("plains");
    }
}

bool UMovementService::IsCharacterMoving(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    ULocationMovementComponent* MovementComp = GetLocationMovementComponent();
    if (!MovementComp)
    {
        return false;
    }
    
    // MovementStateをチェック（既存ロジックの移植）
    return MovementComp->GetMovementState(TeamIndex) == EMovementState::MovingToDestination;
}

bool UMovementService::ProcessMovementProgress(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    ULocationMovementComponent* MovementComp = GetLocationMovementComponent();
    if (!MovementComp)
    {
        return false;
    }
    
    // 既存の移動進行処理を実行
    MovementComp->ProcessMovement(TeamIndex);
    
    // 移動完了チェック
    bool bMovementCompleted = MovementComp->GetMovementState(TeamIndex) == EMovementState::Stationary;
    
    if (bMovementCompleted)
    {
        UE_LOG(LogTemp, VeryVerbose, 
            TEXT("🚶✅ MovementService: Character %s completed movement"),
            *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
    
    return bMovementCompleted;
}

bool UMovementService::CheckMovementProgress(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    ULocationMovementComponent* MovementComp = GetLocationMovementComponent();
    if (!MovementComp)
    {
        return false;
    }
    
    // 単純に現在の移動状態をチェック（状態を変更しない）
    bool bMovementCompleted = MovementComp->GetMovementState(TeamIndex) == EMovementState::Stationary;
    
    if (bMovementCompleted)
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("🚶✅ MovementService: Character %s movement is completed"),
            *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
    else
    {
        // 移動進捗を取得してログ出力
        FMovementInfo MovementInfo = MovementComp->GetMovementInfo(TeamIndex);
        UE_LOG(LogTemp, Warning, 
            TEXT("🚶🚶 MovementService: Character %s still moving (%.1f%% complete)"),
            *IIdleCharacterInterface::Execute_GetCharacterName(Character),
            MovementInfo.Progress * 100.0f);
    }
    
    return bMovementCompleted;
}

float UMovementService::GetDistanceBetweenLocations(const FString& FromLocation, const FString& ToLocation)
{
    // キャッシュチェック
    FString CacheKey = FString::Printf(TEXT("%s_%s"), *FromLocation, *ToLocation);
    
    if (IsCacheValid() && DistanceCache.Contains(CacheKey))
    {
        return DistanceCache[CacheKey];
    }
    
    float Distance = 0.0f;
    
    // 既存の距離計算ロジック（簡易版）
    if (FromLocation == ToLocation)
    {
        Distance = 0.0f;
    }
    else if ((FromLocation == TEXT("base") && ToLocation == TEXT("plains")) ||
             (FromLocation == TEXT("plains") && ToLocation == TEXT("base")))
    {
        Distance = 100.0f; // 既存システムと同じ距離
    }
    else
    {
        Distance = 50.0f; // デフォルト距離
    }
    
    // キャッシュに保存
    DistanceCache.Add(CacheKey, Distance);
    
    return Distance;
}

float UMovementService::GetDistanceFromBase(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return 0.0f;
    }
    
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return 0.0f;
    }
    
    ULocationMovementComponent* MovementComp = GetLocationMovementComponent();
    if (!MovementComp)
    {
        return 0.0f;
    }
    
    return MovementComp->GetCurrentDistanceFromBase(TeamIndex);
}

bool UMovementService::IsLocationAccessible(const FString& LocationId)
{
    // 基本的な場所IDのバリデーション
    TArray<FString> ValidLocations = {TEXT("base"), TEXT("plains")};
    return ValidLocations.Contains(LocationId);
}

TArray<FString> UMovementService::GetAllAvailableLocations()
{
    return {TEXT("base"), TEXT("plains")};
}

FString UMovementService::GetMovementStatistics()
{
    float SuccessRate = (TotalMovementRequests > 0) ? 
        (float)SuccessfulMovements / TotalMovementRequests * 100.0f : 0.0f;
    
    float AverageMovementTime = (SuccessfulMovements > 0) ? 
        TotalMovementTime / SuccessfulMovements : 0.0f;
    
    return FString::Printf(
        TEXT("Movement Stats: %d requests, %d successful (%.1f%%), avg time: %.1fs"),
        TotalMovementRequests, SuccessfulMovements, SuccessRate, AverageMovementTime);
}

bool UMovementService::ExecuteExistingMovementLogic(AC_IdleCharacter* Character, const FString& TargetLocation)
{
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    ULocationMovementComponent* MovementComp = GetLocationMovementComponent();
    if (!MovementComp)
    {
        return false;
    }
    
    // 既に移動中かチェック
    EMovementState CurrentState = MovementComp->GetMovementState(TeamIndex);
    if (CurrentState == EMovementState::MovingToDestination)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚶⚠️ MovementService: Character is already moving"));
        return true; // 既に移動中なので成功として扱う
    }
    
    // 基本的な移動処理の実装
    UE_LOG(LogTemp, Warning, TEXT("🚶 MovementService: Starting movement to %s"), *TargetLocation);
    
    // 現在地を取得
    FString CurrentLocation = GetCharacterCurrentLocation(Character);
    UE_LOG(LogTemp, Warning, TEXT("🚶📍 MovementService: Moving from %s to %s"), *CurrentLocation, *TargetLocation);
    
    // 既に目的地にいる場合はスキップ
    if (CurrentLocation == TargetLocation)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚶✅ MovementService: Already at destination %s"), *TargetLocation);
        return true;
    }
    
    // LocationMovementComponentを使って移動開始
    bool bMovementStarted = MovementComp->StartMovement(TeamIndex, CurrentLocation, TargetLocation);
    
    if (bMovementStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚶✅ MovementService: Movement to %s started successfully"), *TargetLocation);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🚶❌ MovementService: Failed to start movement to %s"), *TargetLocation);
        return false;
    }
}

float UMovementService::GetExistingMovementTime(const FString& FromLocation, const FString& ToLocation)
{
    // 既存システムの移動時間計算を移植
    float Distance = GetDistanceBetweenLocations(FromLocation, ToLocation);
    
    if (Distance <= 0.0f)
    {
        return 0.0f; // 既に目的地にいる
    }
    
    // 既存システムでは移動速度は固定（1ターン = 2秒で移動完了）
    return 2.0f;
}

int32 UMovementService::GetCharacterTeamIndex(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return -1;
    }
    
    // PlayerControllerからTeamComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return -1;
    }
    
    UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
    if (!TeamComp)
    {
        return -1;
    }
    
    // キャラクターが所属するチームを検索
    for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
    {
        FTeam Team = TeamComp->GetTeam(i);
        if (Team.Members.Contains(Character))
        {
            return i;
        }
    }
    
    return -1;
}

ULocationMovementComponent* UMovementService::GetLocationMovementComponent()
{
    // PlayerControllerからLocationMovementComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return nullptr;
    }
    
    return PlayerController->FindComponentByClass<ULocationMovementComponent>();
}

bool UMovementService::AreReferencesValid()
{
    return IsValid(GetLocationMovementComponent());
}

void UMovementService::ResetStatistics()
{
    TotalMovementRequests = 0;
    SuccessfulMovements = 0;
    TotalMovementTime = 0.0f;
}

void UMovementService::UpdateDistanceCache()
{
    if (!IsCacheValid())
    {
        DistanceCache.Empty();
        LastCacheUpdate = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    }
}

bool UMovementService::IsCacheValid() const
{
    if (!GetWorld())
    {
        return false;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    return (CurrentTime - LastCacheUpdate) < CacheValidDuration;
}