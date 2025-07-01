#pragma once

#include "CoreMinimal.h"
#include "GridMapComponent.h"
#include "GridMapComponent_Optimized.generated.h"

// グリッドチャンク構造体
USTRUCT(BlueprintType)
struct UE_IDLE_API FGridChunk
{
    GENERATED_BODY()

    // チャンクの左下グリッド座標
    UPROPERTY(BlueprintReadOnly, Category = "Grid Chunk")
    FIntPoint ChunkOrigin;

    // チャンク内のセルデータ
    UPROPERTY(BlueprintReadOnly, Category = "Grid Chunk")
    TMap<FIntPoint, FGridCellData> CellData;

    // チャンクのアクティブ状態
    UPROPERTY(BlueprintReadOnly, Category = "Grid Chunk")
    bool bIsActive = false;

    // 最終アクセス時間
    UPROPERTY(BlueprintReadOnly, Category = "Grid Chunk")
    float LastAccessTime = 0.0f;

    // チャンク内のアクター（キャッシュ）
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ContainedActors;

    FGridChunk()
    {
        ChunkOrigin = FIntPoint::ZeroValue;
        bIsActive = false;
        LastAccessTime = 0.0f;
    }

    FGridChunk(const FIntPoint& Origin)
    {
        ChunkOrigin = Origin;
        bIsActive = false;
        LastAccessTime = 0.0f;
    }
};

/**
 * 大規模グリッド用の最適化GridMapComponent
 * チャンク分割による空間分割とLOD管理
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UGridMapComponent_Optimized : public UGridMapComponent
{
    GENERATED_BODY()

public:
    UGridMapComponent_Optimized();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === チャンク設定 ===

    // チャンクサイズ（セル数）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Settings")
    int32 ChunkSize = 10;

    // 最大アクティブチャンク数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Settings")
    int32 MaxActiveChunks = 9; // 3x3エリア

    // チャンク非アクティブ化時間（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Settings")
    float ChunkDeactivationTime = 30.0f;

    // === チャンクデータ ===

    // 全チャンクデータ
    UPROPERTY()
    TMap<FIntPoint, FGridChunk> Chunks;

    // アクティブチャンクリスト
    UPROPERTY()
    TSet<FIntPoint> ActiveChunks;

    // ビューアー位置（チャンク座標）
    UPROPERTY()
    FIntPoint ViewerChunkCoord;

    // === 最適化タイマー ===

    UPROPERTY()
    FTimerHandle ChunkUpdateTimerHandle;

    UPROPERTY()
    FTimerHandle ChunkCleanupTimerHandle;

public:
    // === チャンク管理 ===

    // 指定位置のチャンクをアクティブ化
    UFUNCTION(BlueprintCallable, Category = "Chunk Management")
    void ActivateChunk(const FIntPoint& ChunkCoord);

    // 指定位置のチャンクを非アクティブ化
    UFUNCTION(BlueprintCallable, Category = "Chunk Management")
    void DeactivateChunk(const FIntPoint& ChunkCoord);

    // ビューアー位置更新
    UFUNCTION(BlueprintCallable, Category = "Chunk Management")
    void UpdateViewerPosition(const FVector& ViewerPosition);

    // 視界内チャンクのみアクティブ化
    UFUNCTION(BlueprintCallable, Category = "Chunk Management")
    void UpdateVisibleChunks(const FVector& ViewerPosition);

    // === 座標変換 ===

    // グリッド座標からチャンク座標取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chunk Coordinates")
    FIntPoint GetChunkCoord(const FIntPoint& GridPosition) const;

    // チャンク座標からワールド座標取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chunk Coordinates")
    FVector GetChunkWorldCenter(const FIntPoint& ChunkCoord) const;

    // チャンク内のローカル座標取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chunk Coordinates")
    FIntPoint GetLocalCoordInChunk(const FIntPoint& GridPosition) const;

    // === 最適化アクセス ===

    // セルデータ取得（最適化版）
    virtual FGridCellData GetCellData(const FIntPoint& GridPos) const; // override一時削除

    // セルデータ設定（最適化版）
    UFUNCTION(BlueprintCallable, Category = "Grid Optimized")
    void SetCellDataOptimized(const FIntPoint& GridPos, const FGridCellData& CellData);

    // === アクター管理 ===

    // アクターをチャンクに登録
    UFUNCTION(BlueprintCallable, Category = "Actor Management")
    void RegisterActorToChunk(AActor* Actor, const FIntPoint& GridPosition);

    // アクターをチャンクから削除
    UFUNCTION(BlueprintCallable, Category = "Actor Management")
    void UnregisterActorFromChunk(AActor* Actor, const FIntPoint& GridPosition);

    // チャンク内のアクター取得
    UFUNCTION(BlueprintCallable, Category = "Actor Management")
    TArray<AActor*> GetActorsInChunk(const FIntPoint& ChunkCoord) const;

    // === 統計情報 ===

    // アクティブチャンク数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chunk Statistics")
    int32 GetActiveChunkCount() const { return ActiveChunks.Num(); }

    // 総チャンク数取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chunk Statistics")
    int32 GetTotalChunkCount() const { return Chunks.Num(); }

    // メモリ使用量取得（概算）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chunk Statistics")
    float GetEstimatedMemoryUsage() const;

    // === デバッグ ===

    // チャンク境界描画切り替え
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void SetChunkBoundsVisible(bool bVisible);

private:
    // === 内部実装 ===

    // チャンク更新処理
    UFUNCTION()
    void ProcessChunkUpdate();

    // 非使用チャンククリーンアップ
    UFUNCTION()
    void ProcessChunkCleanup();

    // チャンク初期化
    void InitializeChunk(const FIntPoint& ChunkCoord);

    // チャンクが存在するかチェック
    bool DoesChunkExist(const FIntPoint& ChunkCoord) const;

    // 周辺チャンクのアクティブ化
    void ActivateNeighboringChunks(const FIntPoint& CenterChunkCoord, int32 Radius = 1);

    // 古いチャンクの非アクティブ化
    void DeactivateOldChunks();

    // チャンクの境界描画
    void DrawChunkBounds(bool bVisible);

    // === デバッグフラグ ===
    UPROPERTY()
    bool bShowChunkBounds = false;
};