#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "LODManagerComponent.generated.h"

// LODレベル定義
UENUM(BlueprintType)
enum class ELODLevel : uint8
{
    High        UMETA(DisplayName = "高品質"),      // 近距離：詳細表示
    Medium      UMETA(DisplayName = "中品質"),      // 中距離：簡易表示
    Low         UMETA(DisplayName = "低品質"),      // 遠距離：最小表示
    Hidden      UMETA(DisplayName = "非表示")       // 最遠距離：非表示
};

// LOD設定構造体
USTRUCT(BlueprintType)
struct UE_IDLE_API FLODSettings
{
    GENERATED_BODY()

    // 高品質表示の距離閾値
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float HighQualityDistance = 2000.0f;

    // 中品質表示の距離閾値
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float MediumQualityDistance = 5000.0f;

    // 低品質表示の距離閾値
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float LowQualityDistance = 10000.0f;

    // Tick間隔の調整
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float HighQualityTickInterval = 0.0f;    // 毎フレーム

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float MediumQualityTickInterval = 0.1f;  // 0.1秒

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float LowQualityTickInterval = 0.5f;     // 0.5秒

    FLODSettings()
    {
        HighQualityDistance = 2000.0f;
        MediumQualityDistance = 5000.0f;
        LowQualityDistance = 10000.0f;
        HighQualityTickInterval = 0.0f;
        MediumQualityTickInterval = 0.1f;
        LowQualityTickInterval = 0.5f;
    }
};

// LOD対象インターフェース
UINTERFACE(BlueprintType)
class UE_IDLE_API ULODTarget : public UInterface
{
    GENERATED_BODY()
};

class UE_IDLE_API ILODTarget
{
    GENERATED_BODY()

public:
    // LODレベル変更時のコールバック
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LOD")
    void OnLODLevelChanged(ELODLevel NewLODLevel, float DistanceToViewer);
    virtual void OnLODLevelChanged_Implementation(ELODLevel NewLODLevel, float DistanceToViewer) {}

    // 現在のLODレベル取得
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LOD")
    ELODLevel GetCurrentLODLevel() const;
    virtual ELODLevel GetCurrentLODLevel_Implementation() const { return ELODLevel::High; }
};

/**
 * モバイル向けLOD管理コンポーネント
 * カメラからの距離に応じて表示品質を動的調整
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API ULODManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULODManagerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === LOD設定 ===

    // LOD設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    FLODSettings LODSettings;

    // LOD更新間隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    float LODUpdateInterval = 0.5f;

    // モバイル最適化モード
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Settings")
    bool bMobileOptimizationMode = true;

    // === 管理対象 ===

    // 登録されたLOD対象アクター
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> LODTargets;

    // LODタイマーハンドル
    UPROPERTY()
    FTimerHandle LODUpdateTimerHandle;

public:
    // === LOD管理 ===

    // LOD対象アクターを登録
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void RegisterLODTarget(AActor* Actor);

    // LOD対象アクターの登録解除
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UnregisterLODTarget(AActor* Actor);

    // 全LOD対象の更新
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void UpdateAllLODLevels();

    // 手動LOD更新開始/停止
    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void StartLODUpdates();

    UFUNCTION(BlueprintCallable, Category = "LOD Management")
    void StopLODUpdates();

    // === ビューアー取得 ===

    // 現在のビューアー位置取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LOD Management")
    FVector GetViewerLocation() const;

    // 指定アクターのLODレベル計算
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LOD Management")
    ELODLevel CalculateLODLevel(AActor* Actor) const;

    // === 統計情報 ===

    // 登録アクター数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LOD Statistics")
    int32 GetRegisteredActorCount() const;

    // LODレベル別アクター数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LOD Statistics")
    void GetLODStatistics(int32& OutHigh, int32& OutMedium, int32& OutLow, int32& OutHidden) const;

private:
    // === 内部実装 ===

    // LOD更新処理
    UFUNCTION()
    void ProcessLODUpdate();

    // 個別アクターのLOD更新
    void UpdateActorLOD(AActor* Actor);

    // 距離からLODレベル計算
    ELODLevel CalculateLODLevelFromDistance(float Distance) const;

    // 無効なアクターのクリーンアップ
    void CleanupInvalidTargets();

    // アクターのTick間隔調整
    void AdjustActorTickInterval(AActor* Actor, ELODLevel LODLevel);

    // デフォルトLOD適用
    void ApplyDefaultLOD(AActor* Actor, ELODLevel LODLevel);
};