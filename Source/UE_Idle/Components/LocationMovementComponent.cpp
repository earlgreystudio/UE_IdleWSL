#include "LocationMovementComponent.h"
#include "TeamComponent.h"
#include "../Managers/LocationDataTableManager.h"
#include "../Types/LocationTypes.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "Engine/World.h"

ULocationMovementComponent::ULocationMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    BaseMovementSpeed = 30.0f;
}

void ULocationMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Manager参照取得
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        LocationManager = GameInstance->GetSubsystem<ULocationDataTableManager>();
        if (!LocationManager)
        {
            UE_LOG(LogTemp, Error, TEXT("MovementComponent: LocationDataTableManager not found!"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("MovementComponent: Initialized"));
}

void ULocationMovementComponent::BeginDestroy()
{
    // 状態クリア
    TeamMovementInfos.Empty();
    TeamCurrentDistanceFromBase.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("MovementComponent: Destroyed"));
    
    Super::BeginDestroy();
}

// === 移動制御 ===

bool ULocationMovementComponent::StartMovement(int32 TeamIndex, const FString& FromLocation, const FString& ToLocation)
{
    if (!IsValidTeam(TeamIndex))
    {
        LogMovementError(FString::Printf(TEXT("StartMovement: Invalid team index %d"), TeamIndex));
        return false;
    }
    
    // 移動速度を計算
    float Speed = CalculateTeamMovementSpeed(TeamIndex);
    if (Speed <= 0.0f)
    {
        LogMovementError(FString::Printf(TEXT("StartMovement: Invalid speed for team %d"), TeamIndex));
        return false;
    }
    
    // 出発地と目的地の拠点からの距離を取得
    float FromDistance = GetLocationDistanceFromBase(FromLocation);
    float ToDistance = GetLocationDistanceFromBase(ToLocation);
    
    if (FromDistance < 0.0f || ToDistance < 0.0f)
    {
        LogMovementError(FString::Printf(TEXT("StartMovement: Invalid locations %s -> %s"), *FromLocation, *ToLocation));
        return false;
    }
    
    // 移動情報を設定
    FMovementInfo& MovementInfo = TeamMovementInfos.FindOrAdd(TeamIndex);
    MovementInfo.FromLocation = FromLocation;
    MovementInfo.ToLocation = ToLocation;
    MovementInfo.Speed = Speed;
    MovementInfo.Progress = 0.0f;
    MovementInfo.CurrentDistanceFromBase = FromDistance;
    MovementInfo.TargetDistanceFromBase = ToDistance;
    
    // 移動方向を決定
    MovementInfo.bMovingAwayFromBase = (ToDistance > FromDistance);
    
    // 総移動距離と時間を計算
    MovementInfo.Distance = FMath::Abs(ToDistance - FromDistance);
    MovementInfo.TotalTime = (MovementInfo.Distance > 0.0f) ? (MovementInfo.Distance / Speed) : 0.0f;
    MovementInfo.RemainingTime = MovementInfo.TotalTime;
    
    // 移動状態を設定
    if (ToLocation == TEXT("base"))
    {
        MovementInfo.State = EMovementState::MovingToBase;
    }
    else
    {
        MovementInfo.State = EMovementState::MovingToDestination;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("MovementComponent: Started movement for team %d from %s (%.1fm) to %s (%.1fm) - Distance: %.1f, Speed: %.1f, Time: %.1fs"), 
        TeamIndex, *FromLocation, FromDistance, *ToLocation, ToDistance, MovementInfo.Distance, Speed, MovementInfo.TotalTime);
    
    // イベント発行
    OnMovementStarted.Broadcast(TeamIndex);
    OnMovementProgressUpdated.Broadcast(TeamIndex, MovementInfo);
    
    return true;
}

bool ULocationMovementComponent::StopMovement(int32 TeamIndex)
{
    if (!TeamMovementInfos.Contains(TeamIndex))
    {
        return false;
    }
    
    // 移動情報をクリア
    TeamMovementInfos.Remove(TeamIndex);
    
    UE_LOG(LogTemp, Log, TEXT("MovementComponent: Stopped movement for team %d"), TeamIndex);
    return true;
}

bool ULocationMovementComponent::StartReturnToBase(int32 TeamIndex, const FString& CurrentLocation)
{
    if (!IsValidTeam(TeamIndex))
    {
        LogMovementError(FString::Printf(TEXT("StartReturnToBase: Invalid team index %d"), TeamIndex));
        return false;
    }
    
    // 拠点への移動を開始
    bool bStarted = StartMovement(TeamIndex, CurrentLocation, TEXT("base"));
    if (bStarted)
    {
        // 状態を帰還中に変更
        if (FMovementInfo* MovementInfo = TeamMovementInfos.Find(TeamIndex))
        {
            MovementInfo->State = EMovementState::MovingToBase;
            OnMovementProgressUpdated.Broadcast(TeamIndex, *MovementInfo);
        }
    }
    
    return bStarted;
}

// === 移動処理（TimeManagerから呼び出し） ===

void ULocationMovementComponent::ProcessMovement(int32 TeamIndex)
{
    if (!TeamMovementInfos.Contains(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessMovement: No movement info for team %d"), TeamIndex);
        return;
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ProcessMovement: Processing movement for team %d"), TeamIndex);
    
    // 1秒分の移動処理
    UpdateMovementInfo(TeamIndex, 1.0f);
}

// === 情報取得 ===

FMovementInfo ULocationMovementComponent::GetMovementInfo(int32 TeamIndex) const
{
    if (TeamMovementInfos.Contains(TeamIndex))
    {
        return TeamMovementInfos[TeamIndex];
    }
    
    // デフォルト情報を返す
    FMovementInfo DefaultInfo;
    DefaultInfo.State = EMovementState::Stationary;
    return DefaultInfo;
}

EMovementState ULocationMovementComponent::GetMovementState(int32 TeamIndex) const
{
    if (TeamMovementInfos.Contains(TeamIndex))
    {
        return TeamMovementInfos[TeamIndex].State;
    }
    return EMovementState::Stationary;
}

float ULocationMovementComponent::GetMovementProgress(int32 TeamIndex) const
{
    if (TeamMovementInfos.Contains(TeamIndex))
    {
        return TeamMovementInfos[TeamIndex].Progress;
    }
    return 0.0f;
}

float ULocationMovementComponent::GetRemainingTime(int32 TeamIndex) const
{
    if (TeamMovementInfos.Contains(TeamIndex))
    {
        return TeamMovementInfos[TeamIndex].RemainingTime;
    }
    return 0.0f;
}

FString ULocationMovementComponent::GetRemainingTimeFormatted(int32 TeamIndex) const
{
    float RemainingTime = GetRemainingTime(TeamIndex);
    
    int32 Minutes = FMath::FloorToInt(RemainingTime / 60.0f);
    int32 Seconds = FMath::FloorToInt(RemainingTime) % 60;
    
    return FString::Printf(TEXT("%02d：%02d"), Minutes, Seconds);
}

float ULocationMovementComponent::GetCurrentDistanceFromBase(int32 TeamIndex) const
{
    if (TeamMovementInfos.Contains(TeamIndex))
    {
        // 移動中の場合、現在の距離を返す
        return TeamMovementInfos[TeamIndex].CurrentDistanceFromBase;
    }
    
    // 移動中でない場合、永続化された現在距離を返す
    if (TeamCurrentDistanceFromBase.Contains(TeamIndex))
    {
        return TeamCurrentDistanceFromBase[TeamIndex];
    }
    
    // まだ記録がない場合は拠点（距離0）
    return 0.0f;
}

void ULocationMovementComponent::SetCurrentDistanceFromBase(int32 TeamIndex, float Distance)
{
    // ターンベース移動用：直接距離を設定
    if (TeamMovementInfos.Contains(TeamIndex))
    {
        // 移動中の場合、MovementInfo内の距離を更新
        TeamMovementInfos[TeamIndex].CurrentDistanceFromBase = Distance;
    }
    
    // 永続化された距離も更新
    TeamCurrentDistanceFromBase.Add(TeamIndex, Distance);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MovementComponent: Team %d distance set to %.1fm"), TeamIndex, Distance);
}

// === ヘルパー関数 ===

float ULocationMovementComponent::CalculateTeamMovementSpeed(int32 TeamIndex) const
{
    if (!TeamComponent || !IsValidTeam(TeamIndex))
    {
        return BaseMovementSpeed;
    }
    
    // 基本的には基本速度を返す（将来的にはチームメンバーの能力で調整）
    return BaseMovementSpeed;
}

float ULocationMovementComponent::GetDistanceBetweenLocations(const FString& FromLocation, const FString& ToLocation) const
{
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Error, TEXT("MovementComponent::GetDistanceBetweenLocations - LocationManager is null"));
        return -1.0f;
    }
    
    // 同じ場所なら距離0
    if (FromLocation == ToLocation)
    {
        return 0.0f;
    }
    
    // 拠点からの移動または拠点への移動
    if (FromLocation == TEXT("base") || ToLocation == TEXT("base"))
    {
        FString LocationId = (FromLocation == TEXT("base")) ? ToLocation : FromLocation;
        
        FLocationDataRow LocationData;
        if (LocationManager->GetLocationData(LocationId, LocationData))
        {
            return LocationData.Distance;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MovementComponent::GetDistanceBetweenLocations - Location %s not found"), *LocationId);
            return -1.0f;
        }
    }
    
    // 場所間の移動（将来的に実装）
    UE_LOG(LogTemp, Warning, TEXT("MovementComponent::GetDistanceBetweenLocations - Location-to-location movement not yet implemented"));
    return -1.0f;
}

float ULocationMovementComponent::GetLocationDistanceFromBase(const FString& LocationId) const
{
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Error, TEXT("MovementComponent::GetLocationDistanceFromBase - LocationManager is null"));
        return -1.0f;
    }
    
    // 拠点なら距離0
    if (LocationId == TEXT("base"))
    {
        return 0.0f;
    }
    
    // 場所データを取得
    FLocationDataRow LocationData;
    if (LocationManager->GetLocationData(LocationId, LocationData))
    {
        return LocationData.Distance;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MovementComponent::GetLocationDistanceFromBase - Location %s not found"), *LocationId);
        return -1.0f;
    }
}

// === コンポーネント登録 ===

void ULocationMovementComponent::RegisterTeamComponent(UTeamComponent* InTeamComponent)
{
    TeamComponent = InTeamComponent;
    UE_LOG(LogTemp, Log, TEXT("MovementComponent: Registered TeamComponent"));
}

void ULocationMovementComponent::InitializeTeamAtBase(int32 TeamIndex)
{
    // チームを拠点（距離0）に初期化
    TeamCurrentDistanceFromBase.Add(TeamIndex, 0.0f);
    UE_LOG(LogTemp, Log, TEXT("MovementComponent: Initialized team %d at base (distance 0)"), TeamIndex);
}

// === 内部ヘルパー ===

void ULocationMovementComponent::UpdateMovementInfo(int32 TeamIndex, float DeltaTime)
{
    FMovementInfo* MovementInfo = TeamMovementInfos.Find(TeamIndex);
    if (!MovementInfo)
    {
        return;
    }
    
    // 即座移動の場合
    if (MovementInfo->TotalTime <= 0.0f)
    {
        MovementInfo->Progress = 1.0f;
        MovementInfo->RemainingTime = 0.0f;
        MovementInfo->CurrentDistanceFromBase = MovementInfo->TargetDistanceFromBase;
        CompleteMovement(TeamIndex);
        return;
    }
    
    // このティックでの移動距離を計算
    float DistanceThisTick = MovementInfo->Speed * DeltaTime;
    
    // 移動方向に応じて現在距離を更新
    if (MovementInfo->bMovingAwayFromBase)
    {
        // 拠点から離れる方向
        MovementInfo->CurrentDistanceFromBase += DistanceThisTick;
        
        // 目的地に到達したかチェック
        if (MovementInfo->CurrentDistanceFromBase >= MovementInfo->TargetDistanceFromBase)
        {
            MovementInfo->CurrentDistanceFromBase = MovementInfo->TargetDistanceFromBase;
            MovementInfo->Progress = 1.0f;
            MovementInfo->RemainingTime = 0.0f;
            CompleteMovement(TeamIndex);
            return;
        }
    }
    else
    {
        // 拠点に向かう方向
        MovementInfo->CurrentDistanceFromBase -= DistanceThisTick;
        
        // 拠点に到達したかチェック
        if (MovementInfo->CurrentDistanceFromBase <= MovementInfo->TargetDistanceFromBase)
        {
            MovementInfo->CurrentDistanceFromBase = MovementInfo->TargetDistanceFromBase;
            MovementInfo->Progress = 1.0f;
            MovementInfo->RemainingTime = 0.0f;
            CompleteMovement(TeamIndex);
            return;
        }
    }
    
    // 進捗と残り時間を再計算
    float TraveledDistance = FMath::Abs(MovementInfo->CurrentDistanceFromBase - 
                                       (MovementInfo->bMovingAwayFromBase ? 
                                        MovementInfo->TargetDistanceFromBase - MovementInfo->Distance :
                                        MovementInfo->TargetDistanceFromBase + MovementInfo->Distance));
    MovementInfo->Progress = (MovementInfo->Distance > 0.0f) ? (TraveledDistance / MovementInfo->Distance) : 1.0f;
    
    float RemainingDistance = FMath::Abs(MovementInfo->TargetDistanceFromBase - MovementInfo->CurrentDistanceFromBase);
    MovementInfo->RemainingTime = (MovementInfo->Speed > 0.0f) ? (RemainingDistance / MovementInfo->Speed) : 0.0f;
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MovementComponent: Team %d at %.1fm, progress %.2f, remaining %.1fs"), 
        TeamIndex, MovementInfo->CurrentDistanceFromBase, MovementInfo->Progress, MovementInfo->RemainingTime);
    
    // UI更新イベント発行
    OnMovementProgressUpdated.Broadcast(TeamIndex, *MovementInfo);
}

void ULocationMovementComponent::CompleteMovement(int32 TeamIndex)
{
    FMovementInfo* MovementInfo = TeamMovementInfos.Find(TeamIndex);
    if (!MovementInfo)
    {
        return;
    }
    
    FString ArrivedLocation = MovementInfo->ToLocation;
    float FinalDistance = MovementInfo->CurrentDistanceFromBase;
    MovementInfo->State = EMovementState::Arrived;
    MovementInfo->Progress = 1.0f;
    MovementInfo->RemainingTime = 0.0f;
    
    // 現在距離を永続化
    TeamCurrentDistanceFromBase.Add(TeamIndex, FinalDistance);
    
    UE_LOG(LogTemp, Warning, TEXT("MovementComponent: Team %d completed movement to %s at distance %.1fm"), TeamIndex, *ArrivedLocation, FinalDistance);
    
    // 移動完了イベント発行
    OnMovementCompleted.Broadcast(TeamIndex, ArrivedLocation);
    
    // 移動情報をクリア（到着後は静止状態）
    TeamMovementInfos.Remove(TeamIndex);
}

bool ULocationMovementComponent::IsValidTeam(int32 TeamIndex) const
{
    return TeamComponent && TeamComponent->GetAllTeams().IsValidIndex(TeamIndex);
}

void ULocationMovementComponent::LogMovementError(const FString& ErrorMessage) const
{
    UE_LOG(LogTemp, Error, TEXT("MovementComponent: %s"), *ErrorMessage);
}