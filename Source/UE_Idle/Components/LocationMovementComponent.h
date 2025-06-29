#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "LocationMovementComponent.generated.h"

// Forward declarations
class UTeamComponent;
class ULocationDataTableManager;

// 移動状態
UENUM(BlueprintType)
enum class EMovementState : uint8
{
    Stationary          UMETA(DisplayName = "静止"),
    MovingToDestination UMETA(DisplayName = "目的地へ移動中"),
    MovingToBase        UMETA(DisplayName = "拠点へ帰還中"),
    Arrived             UMETA(DisplayName = "到着")
};

// 移動情報構造体
USTRUCT(BlueprintType)
struct FMovementInfo
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float Progress = 0.0f;          // 進捗 0.0-1.0
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float TotalTime = 0.0f;         // 総移動時間（秒）
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float RemainingTime = 0.0f;     // 残り時間（秒）
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    FString FromLocation;           // 出発地
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    FString ToLocation;             // 目的地
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    EMovementState State;           // 移動状態
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float Distance = 0.0f;          // 移動距離
    
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float Speed = 0.0f;             // 移動速度
    
    // 拠点からの現在距離
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float CurrentDistanceFromBase = 0.0f;
    
    // 移動方向（true: 拠点から離れる, false: 拠点に向かう）
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool bMovingAwayFromBase = true;
    
    // 目的地の拠点からの距離
    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float TargetDistanceFromBase = 0.0f;
    
    FMovementInfo()
    {
        Progress = 0.0f;
        TotalTime = 0.0f;
        RemainingTime = 0.0f;
        FromLocation = TEXT("base");
        ToLocation = TEXT("");
        State = EMovementState::Stationary;
        Distance = 0.0f;
        Speed = 0.0f;
        CurrentDistanceFromBase = 0.0f;
        bMovingAwayFromBase = true;
        TargetDistanceFromBase = 0.0f;
    }
};

// イベントデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementProgressUpdated, int32, TeamIndex, const FMovementInfo&, MovementInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementCompleted, int32, TeamIndex, const FString&, ArrivedLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementStarted, int32, TeamIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API ULocationMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULocationMovementComponent();

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // === 設定 ===
    
    // 基本移動速度（m/秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Settings")
    float BaseMovementSpeed = 30.0f;
    
    // === 内部状態 ===
    
    // チーム別移動情報
    UPROPERTY()
    TMap<int32, FMovementInfo> TeamMovementInfos;
    
    // チーム別現在距離（永続管理）
    UPROPERTY()
    TMap<int32, float> TeamCurrentDistanceFromBase;
    
    // === 参照コンポーネント ===
    
    UPROPERTY()
    UTeamComponent* TeamComponent = nullptr;
    
    UPROPERTY()
    ULocationDataTableManager* LocationManager = nullptr;

public:
    // === 移動制御 ===
    
    // 移動開始
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool StartMovement(int32 TeamIndex, const FString& FromLocation, const FString& ToLocation);
    
    // 移動停止
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool StopMovement(int32 TeamIndex);
    
    // 拠点への帰還開始
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool StartReturnToBase(int32 TeamIndex, const FString& CurrentLocation);
    
    // === 移動処理（TimeManagerから呼び出し） ===
    
    // 移動処理更新
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void ProcessMovement(int32 TeamIndex);
    
    // === 情報取得 ===
    
    // 移動情報取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    FMovementInfo GetMovementInfo(int32 TeamIndex) const;
    
    // 移動状態取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    EMovementState GetMovementState(int32 TeamIndex) const;
    
    // 移動進捗取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetMovementProgress(int32 TeamIndex) const;
    
    // 残り時間取得（秒）
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetRemainingTime(int32 TeamIndex) const;
    
    // 残り時間取得（分:秒形式）
    UFUNCTION(BlueprintPure, Category = "Movement")
    FString GetRemainingTimeFormatted(int32 TeamIndex) const;
    
    // チームの現在の拠点からの距離取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetCurrentDistanceFromBase(int32 TeamIndex) const;
    
    // チームの現在の拠点からの距離設定（ターンベース移動用）
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetCurrentDistanceFromBase(int32 TeamIndex, float Distance);
    
    // === ヘルパー関数 ===
    
    // チーム移動速度計算
    UFUNCTION(BlueprintPure, Category = "Movement")
    float CalculateTeamMovementSpeed(int32 TeamIndex) const;
    
    // 場所間距離取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetDistanceBetweenLocations(const FString& FromLocation, const FString& ToLocation) const;
    
    // 場所の拠点からの距離取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetLocationDistanceFromBase(const FString& LocationId) const;
    
    // === コンポーネント登録 ===
    
    // TeamComponent登録
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void RegisterTeamComponent(UTeamComponent* InTeamComponent);
    
    // チーム初期化（拠点に配置）
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void InitializeTeamAtBase(int32 TeamIndex);

    // === イベントディスパッチャー ===
    
    UPROPERTY(BlueprintAssignable, Category = "Movement Events")
    FOnMovementProgressUpdated OnMovementProgressUpdated;
    
    UPROPERTY(BlueprintAssignable, Category = "Movement Events")
    FOnMovementCompleted OnMovementCompleted;
    
    UPROPERTY(BlueprintAssignable, Category = "Movement Events")
    FOnMovementStarted OnMovementStarted;

private:
    // === 内部ヘルパー ===
    
    // 移動情報更新
    void UpdateMovementInfo(int32 TeamIndex, float DeltaTime);
    
    // 移動完了処理
    void CompleteMovement(int32 TeamIndex);
    
    // 有効性チェック
    bool IsValidTeam(int32 TeamIndex) const;
    
    // エラーログ出力
    void LogMovementError(const FString& ErrorMessage) const;
};