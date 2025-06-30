#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/LocationTypes.h"
#include "../Types/TeamTypes.h"
#include "../Types/TaskTypes.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GatheringComponent.generated.h"

// Forward declarations
class UTeamComponent;
class UInventoryComponent;
class UItemDataTableManager;
class ULocationDataTableManager;
class UTaskManagerComponent;
class AC_IdleCharacter;

// 採集状態
UENUM(BlueprintType)
enum class EGatheringState : uint8
{
    Inactive        UMETA(DisplayName = "非アクティブ"),
    MovingToSite    UMETA(DisplayName = "採集地へ移動中"),
    Gathering       UMETA(DisplayName = "採集中"),
    MovingToBase    UMETA(DisplayName = "拠点へ帰還中"),
    Unloading       UMETA(DisplayName = "荷下ろし中")
};

// 採集結果
USTRUCT(BlueprintType)
struct FGatheringResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    FString ItemId;

    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    int32 Quantity;

    UPROPERTY(BlueprintReadWrite, Category = "Gathering")
    FString CharacterName;

    FGatheringResult()
    {
        ItemId = TEXT("");
        Quantity = 0;
        CharacterName = TEXT("");
    }
};

// イベントデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGatheringStateChanged, int32, TeamIndex, EGatheringState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemGathered, int32, TeamIndex, const FGatheringResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementProgress, int32, TeamIndex, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryFull, int32, TeamIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAutoUnloadCompleted, int32, TeamIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UGatheringComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGatheringComponent();

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // === 採集設定 ===

    // 採集更新間隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gathering Settings")
    float GatheringUpdateInterval = 1.0f;

    // 基本移動速度（m/tick）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Settings")
    float BaseMovementSpeed = 30.0f;

    // 採取効率係数（調整用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gathering Settings")
    float GatheringEfficiencyMultiplier = 40.0f;

    // 運搬キャラ判定の閾値
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carrier Settings")
    float CarrierGatheringThreshold = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carrier Settings")
    float CarrierCapacityThreshold = 50.0f;

    // === 内部状態 ===

    // チーム別採集状態
    UPROPERTY()
    TMap<int32, EGatheringState> TeamGatheringStates;

    // チーム別移動進捗（0.0-1.0）
    UPROPERTY()
    TMap<int32, float> TeamMovementProgress;

    // チーム別目的地
    UPROPERTY()
    TMap<int32, FString> TeamTargetLocations;

    // タイマーハンドル
    UPROPERTY()
    FTimerHandle GatheringUpdateTimerHandle;

    // === 参照コンポーネント ===

    UPROPERTY()
    UTeamComponent* TeamComponent = nullptr;

    UPROPERTY()
    UItemDataTableManager* ItemManager = nullptr;

    UPROPERTY()
    ULocationDataTableManager* LocationManager = nullptr;

    UPROPERTY()
    UTaskManagerComponent* TaskManager = nullptr;

public:
    // === 採集制御 ===

    // 採集開始
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    bool StartGathering(int32 TeamIndex, const FString& LocationId);

    // 採集停止
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    bool StopGathering(int32 TeamIndex);

    // 採集状態取得
    UFUNCTION(BlueprintPure, Category = "Gathering")
    EGatheringState GetGatheringState(int32 TeamIndex) const;

    // 移動進捗取得
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetMovementProgress(int32 TeamIndex) const;

    // === 採集処理 ===

    // 採集更新処理（TimeManagerから呼び出し）
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    void UpdateGathering();
    
    // 指定チーム・指定アイテムの採集処理（TimeManagerから委譲）
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    void ProcessTeamGatheringWithTarget(int32 TeamIndex, const FString& TargetItemId);

    // チーム別採集処理
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    void ProcessTeamGathering(int32 TeamIndex);

    // 移動処理
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void ProcessMovement(int32 TeamIndex);

    // 実際の採集処理
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    void ProcessGatheringExecution(int32 TeamIndex);
    
    // 指定アイテムのみの採集処理（TimeManagerからの委譲用）
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    void ProcessGatheringExecutionWithTarget(int32 TeamIndex, const FString& TargetItemId);
    
    // チームの採集場所を設定
    UFUNCTION(BlueprintCallable, Category = "Gathering")
    void SetTeamTargetLocation(int32 TeamIndex, const FString& LocationId);

    // === アイテム配分システム ===

    // アイテムをチームメンバーに配分
    UFUNCTION(BlueprintCallable, Category = "Item Distribution")
    bool DistributeItemToTeam(int32 TeamIndex, const FString& ItemId, int32 Quantity);

    // 運搬キャラ判定
    UFUNCTION(BlueprintPure, Category = "Carrier")
    bool IsCarrierCharacter(AC_IdleCharacter* Character) const;

    // チームの積載可能量取得
    UFUNCTION(BlueprintPure, Category = "Capacity")
    float GetTeamAvailableCapacity(int32 TeamIndex) const;

    // === 自動荷下ろしシステム ===

    // 拠点到着時の自動荷下ろし
    UFUNCTION(BlueprintCallable, Category = "Auto Unload")
    void AutoUnloadResourceItems(int32 TeamIndex);

    // Resourceカテゴリアイテムかチェック
    UFUNCTION(BlueprintPure, Category = "Auto Unload")
    bool IsResourceItem(const FString& ItemId) const;

    // === ヘルパー関数 ===

    // 場所データ取得
    UFUNCTION(BlueprintPure, Category = "Location")
    FLocationDataRow GetLocationData(const FString& LocationId) const;

    // チーム移動速度計算
    UFUNCTION(BlueprintPure, Category = "Movement")
    float CalculateTeamMovementSpeed(int32 TeamIndex) const;

    // チーム採集力計算
    UFUNCTION(BlueprintPure, Category = "Gathering")
    float CalculateTeamGatheringPower(int32 TeamIndex) const;

    // === コンポーネント登録 ===

    // TeamComponent登録
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void RegisterTeamComponent(UTeamComponent* InTeamComponent);

    // TaskManagerComponent登録
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void RegisterTaskManagerComponent(UTaskManagerComponent* InTaskManagerComponent);
    
    // 移動完了時の状態変更（TimeManagerから呼び出し）
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void OnMovementCompleted(int32 TeamIndex, const FString& ArrivedLocation);

    // === イベントディスパッチャー ===

    UPROPERTY(BlueprintAssignable, Category = "Gathering Events")
    FOnGatheringStateChanged OnGatheringStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Gathering Events")
    FOnItemGathered OnItemGathered;

    UPROPERTY(BlueprintAssignable, Category = "Movement Events")
    FOnMovementProgress OnMovementProgress;

    UPROPERTY(BlueprintAssignable, Category = "Gathering Events")
    FOnInventoryFull OnInventoryFull;

    UPROPERTY(BlueprintAssignable, Category = "Auto Unload Events")
    FOnAutoUnloadCompleted OnAutoUnloadCompleted;

private:
    // === 内部ヘルパー ===

    // 採集状態設定
    void SetGatheringState(int32 TeamIndex, EGatheringState NewState);

    // 移動進捗設定
    void SetMovementProgress(int32 TeamIndex, float Progress);

    // エラーログ出力
    void LogGatheringError(const FString& ErrorMessage) const;

    // 有効性チェック
    bool IsValidTeam(int32 TeamIndex) const;
    
    // 拠点ストレージ取得
    UInventoryComponent* GetBaseStorage() const;
    
    // 個数指定タスクの目標量を減らす
    void ReduceSpecifiedTaskQuantity(const FString& ItemId, int32 ReduceAmount);
    
    // 特定アイテムを採集中のチームを停止
    void StopGatheringForItem(const FString& ItemId);
    
    // === 安全性確保 ===
    
    // 処理中フラグ（再入防止）
    bool bProcessingUpdate = false;
};