#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Types/LocationTypes.h"
#include "../Types/TeamTypes.h"
#include "LocationEventManager.generated.h"

class AC_IdleCharacter;
class UCharacterPresetManager;
class UCombatComponent;

// デリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLocationEventTriggered, const FString&, LocationId, const FString&, EventType, const TArray<AC_IdleCharacter*>&, ParticipatingTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyTeamCreated, const TArray<AC_IdleCharacter*>&, EnemyTeam, const FString&, LocationId);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API ULocationEventManager : public UActorComponent
{
    GENERATED_BODY()

public:
    ULocationEventManager();

protected:
    virtual void BeginPlay() override;

public:
    // 場所での戦闘イベントトリガー
    UFUNCTION(BlueprintCallable, Category = "Location Events")
    bool TriggerCombatEvent(const FString& LocationId, const TArray<AC_IdleCharacter*>& AllyTeam);

    // 場所での採取イベントトリガー（将来拡張用）
    UFUNCTION(BlueprintCallable, Category = "Location Events")
    bool TriggerGatheringEvent(const FString& LocationId, const TArray<AC_IdleCharacter*>& AllyTeam);

    // 指定場所で敵チーム生成
    UFUNCTION(BlueprintCallable, Category = "Location Events", meta = (WorldContext = "WorldContextObject"))
    TArray<AC_IdleCharacter*> CreateEnemyTeamForLocation(
        UObject* WorldContextObject,
        const FString& LocationId,
        const FVector& SpawnLocation,
        int32 EnemyCount = 1
    );

    // 場所データ取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    FLocationDataRow GetLocationData(const FString& LocationId) const;

    // 場所名取得（日本語表示用）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    FString GetLocationDisplayName(const FString& LocationId) const;

    // 全ての場所ID取得（DataTableから動的に取得）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    TArray<FString> GetAllLocationIds() const;
    
    // 全ての有効な場所ID取得（DataTableの実際のRowNameを返す）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    TArray<FString> GetValidLocationIds() const;

    // LocationTypeをStringに変換
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    static FString LocationTypeToString(ELocationType LocationType);

    // 全ての場所タイプ取得（Blueprint用）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    static TArray<ELocationType> GetAllLocationTypes();

    // 場所でイベントが発生可能かチェック
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
    bool CanTriggerCombatAtLocation(const FString& LocationId) const;

    // 生成された敵チームの管理
    UFUNCTION(BlueprintCallable, Category = "Location Events")
    void RegisterEnemyTeam(const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationId);

    UFUNCTION(BlueprintCallable, Category = "Location Events")
    void UnregisterEnemyTeam(const TArray<AC_IdleCharacter*>& EnemyTeam);

    // 全敵チームクリア
    UFUNCTION(BlueprintCallable, Category = "Location Events")
    void ClearAllEnemyTeams();

    // イベントディスパッチャー
    UPROPERTY(BlueprintAssignable, Category = "Location Events")
    FOnLocationEventTriggered OnLocationEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Location Events")
    FOnEnemyTeamCreated OnEnemyTeamCreated;

    // 設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 MaxEnemiesPerTeam = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float EnemySpawnRadius = 500.0f;

protected:
    // 敵チーム管理
    TMap<FString, TArray<AC_IdleCharacter*>> ActiveEnemyTeams;

    // プリセットマネージャーへの参照
    UPROPERTY()
    TObjectPtr<UCharacterPresetManager> PresetManager;

    // CombatComponentへの参照
    UPROPERTY()
    TObjectPtr<UCombatComponent> CombatComponent;

private:
    // 敵スポーン位置の計算
    FVector CalculateEnemySpawnPosition(const FVector& CenterLocation, float Radius, int32 Index) const;
    
    // 場所からランダムな敵選択
    FString SelectRandomEnemyForLocation(const FString& LocationId) const;
};