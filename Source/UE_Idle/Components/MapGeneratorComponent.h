#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "GridMapComponent.h"
#include "MapGeneratorComponent.generated.h"

// 前方宣言
class ULocationDataTableManager;
struct FGridCellData;

// バイオーム生成設定
USTRUCT(BlueprintType)
struct UE_IDLE_API FBiomeGenerationRule
{
    GENERATED_BODY()

    // バイオーム名
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString BiomeName;

    // 拠点からの最小距離
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinDistanceFromBase;

    // 拠点からの最大距離
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxDistanceFromBase;

    // 生成確率（0.0-1.0）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GenerationChance;

    // バイオームサイズ（半径）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BiomeRadius;

    // 最大バイオーム数
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxBiomeCount;

    FBiomeGenerationRule()
    {
        BiomeName = TEXT("");
        MinDistanceFromBase = 0;
        MaxDistanceFromBase = 10;
        GenerationChance = 1.0f;
        BiomeRadius = 2;
        MaxBiomeCount = 3;
    }
};

// マップ生成統計
USTRUCT(BlueprintType)
struct UE_IDLE_API FMapGenerationStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalCellsGenerated;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, int32> BiomeCounts;

    UPROPERTY(BlueprintReadOnly)
    float GenerationTimeSeconds;

    UPROPERTY(BlueprintReadOnly)
    int32 BasePlacementX;

    UPROPERTY(BlueprintReadOnly)
    int32 BasePlacementY;

    FMapGenerationStats()
    {
        TotalCellsGenerated = 0;
        GenerationTimeSeconds = 0.0f;
        BasePlacementX = 10;
        BasePlacementY = 10;
    }
};

/**
 * 独立したマップ生成モジュール
 * プロシージャル生成・バイオーム配置・難易度調整を担当
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UMapGeneratorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMapGeneratorComponent();

protected:
    virtual void BeginPlay() override;

    // === マップ生成設定 ===

    // 拠点位置（グリッド座標）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
    FIntPoint BasePosition = FIntPoint(10, 10);

    // マップサイズ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
    int32 MapWidth = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
    int32 MapHeight = 20;

    // バイオーム生成ルール
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
    TArray<FBiomeGenerationRule> BiomeRules;

    // 難易度スケーリング設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Scaling")
    float DifficultyMultiplierPerDistance = 0.2f; // 1マス当たり20%難易度上昇

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty Scaling")
    float MaxDifficultyMultiplier = 3.0f; // 最大3倍

    // ランダムシード
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Generation")
    int32 RandomSeed = 12345;

    // === データ参照 ===

    // 既存LocationDataTableManagerとの統合
    UPROPERTY()
    TObjectPtr<class ULocationDataTableManager> LocationDataTableManager;

    // 生成されたマップデータ
    UPROPERTY()
    TMap<FIntPoint, FGridCellData> GeneratedMapData;

    // 生成統計
    UPROPERTY(BlueprintReadOnly, Category = "Map Generation")
    FMapGenerationStats GenerationStats;

public:
    // === メイン生成API ===

    // マップ生成実行
    UFUNCTION(BlueprintCallable, Category = "Map Generation")
    void GenerateMap(UGridMapComponent* TargetGridMap);

    // LocationDataTableManager設定（既存システムとの統合）
    UFUNCTION(BlueprintCallable, Category = "Map Generation")
    void SetLocationDataTableManager(class ULocationDataTableManager* InLocationManager);

    // === 移動難易度システム ===

    // 移動可能マス数計算
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement System")
    int32 CalculateMovementRange(const FIntPoint& FromPosition, int32 BaseMoveDistance) const;

    // 特定位置の移動難易度取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement System")
    float GetMovementDifficultyAt(const FIntPoint& Position) const;

    // === 情報取得 ===

    // 拠点からの距離取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Map Info")
    int32 GetDistanceFromBase(const FIntPoint& Position) const;

    // 位置の難易度レベル取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Map Info")
    float GetEffectiveDifficultyAt(const FIntPoint& Position) const;

    // 生成統計取得
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Map Info")
    FMapGenerationStats GetGenerationStats() const { return GenerationStats; }

    // === デバッグ・テスト ===

    // マップ生成プレビュー
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void PreviewMapGeneration();

    // 特定位置に手動配置
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ManualPlaceLocationType(const FIntPoint& Position, const FString& LocationTypeName, UGridMapComponent* TargetGridMap);

private:
    // === 内部生成アルゴリズム ===

    // 初期化・クリア
    void InitializeGeneration();

    // 拠点配置
    void PlaceBase();

    // バイオーム配置
    void GenerateBiomes();

    // 個別バイオーム配置
    void PlaceBiome(const FBiomeGenerationRule& Rule);

    // デフォルトで平原配置
    void FillWithDefaultTerrain();

    // === 配置ヘルパー ===

    // 指定位置に場所タイプ配置
    void PlaceLocationTypeAt(const FIntPoint& Position, const FString& LocationTypeName);

    // 円形範囲に配置
    void PlaceInRadius(const FIntPoint& Center, int32 Radius, const FString& LocationTypeName, float Chance = 1.0f);

    // 配置妥当性チェック
    bool IsValidPlacement(const FIntPoint& Position, const FString& LocationTypeName) const;

    // === 難易度調整 ===

    // 距離による難易度調整適用
    void ApplyDifficultyScaling();

    // 敵・リソース調整
    void ScaleEnemiesAndResources(FGridCellData& CellData, float DifficultyMultiplier) const;

    // === ユーティリティ ===

    // ランダム位置生成
    FIntPoint GetRandomPositionInRange(const FIntPoint& Center, int32 MinRadius, int32 MaxRadius) const;

    // 距離計算
    int32 CalculateDistance(const FIntPoint& A, const FIntPoint& B) const;

    // バウンダリチェック
    bool IsInMapBounds(const FIntPoint& Position) const;

    // LocationTypeデータからFGridCellData作成
    FGridCellData CreateGridCellFromLocationData(const FString& LocationTypeName) const;
};