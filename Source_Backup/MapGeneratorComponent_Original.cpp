#include "MapGeneratorComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMathLibrary.h"
#include "../Managers/LocationDataTableManager.h"

UMapGeneratorComponent::UMapGeneratorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: Created"));
}

void UMapGeneratorComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: Ready for map generation"));
}

void UMapGeneratorComponent::InitializeDefaultBiomeRules()
{
    BiomeRules.Empty();
    
    // 平原バイオーム（拠点周囲）
    FBiomeGenerationRule PlainsRule;
    PlainsRule.BiomeName = "plains";
    PlainsRule.MinDistanceFromBase = 1;
    PlainsRule.MaxDistanceFromBase = 4;
    PlainsRule.GenerationChance = 0.8f;
    PlainsRule.BiomeRadius = 2;
    PlainsRule.MaxBiomeCount = 4;
    BiomeRules.Add(PlainsRule);
    
    // 森林バイオーム（3マス離れた位置）
    FBiomeGenerationRule ForestRule;
    ForestRule.BiomeName = "forest";
    ForestRule.MinDistanceFromBase = 3;
    ForestRule.MaxDistanceFromBase = 7;
    ForestRule.GenerationChance = 0.9f;
    ForestRule.BiomeRadius = 2;
    ForestRule.MaxBiomeCount = 3;
    BiomeRules.Add(ForestRule);
    
    // 山地バイオーム（6マス離れた位置）
    FBiomeGenerationRule MountainRule;
    MountainRule.BiomeName = "mountain";
    MountainRule.MinDistanceFromBase = 6;
    MountainRule.MaxDistanceFromBase = 12;
    MountainRule.GenerationChance = 0.7f;
    MountainRule.BiomeRadius = 1;
    MountainRule.MaxBiomeCount = 2;
    BiomeRules.Add(MountainRule);
    
    // 川バイオーム（ランダム配置）
    FBiomeGenerationRule RiverRule;
    RiverRule.BiomeName = "river";
    RiverRule.MinDistanceFromBase = 2;
    RiverRule.MaxDistanceFromBase = 8;
    RiverRule.GenerationChance = 0.3f; // 30%確率
    RiverRule.BiomeRadius = 1;
    RiverRule.MaxBiomeCount = 2;
    BiomeRules.Add(RiverRule);
}

void UMapGeneratorComponent::GenerateMap(UGridMapComponent* TargetGridMap)
{
    if (!TargetGridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("MapGeneratorComponent::GenerateMap - TargetGridMap is null"));
        return;
    }
    
    float StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("🗺️ MapGenerator: Starting map generation with seed %d"), RandomSeed);
    
    // ランダムシード設定
    FMath::RandInit(RandomSeed);
    
    // 生成初期化
    InitializeGeneration();
    
    // 1. 拠点配置
    PlaceBase();
    
    // 2. バイオーム生成
    GenerateBiomes();
    
    // 3. デフォルト地形で埋める
    FillWithDefaultTerrain();
    
    // 4. 難易度スケーリング適用
    ApplyDifficultyScaling();
    
    // 5. 生成されたデータをGridMapに適用
    for (const auto& CellPair : GeneratedMapData)
    {
        TargetGridMap->SetCellData(CellPair.Key, CellPair.Value);
    }
    
    // 生成統計更新
    float EndTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    GenerationStats.GenerationTimeSeconds = EndTime - StartTime;
    GenerationStats.TotalCellsGenerated = GeneratedMapData.Num();
    GenerationStats.BasePlacementX = BasePosition.X;
    GenerationStats.BasePlacementY = BasePosition.Y;
    
    UE_LOG(LogTemp, Log, TEXT("🗺️ MapGenerator: Map generation completed! Generated %d cells in %.3f seconds"), 
        GenerationStats.TotalCellsGenerated, GenerationStats.GenerationTimeSeconds);
}

void UMapGeneratorComponent::SetLocationDataTableManager(ULocationDataTableManager* InLocationManager)
{
    LocationDataTableManager = InLocationManager;
    
    if (LocationDataTableManager)
    {
        UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: LocationDataTableManager set successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MapGeneratorComponent: LocationDataTableManager set to null"));
    }
}

int32 UMapGeneratorComponent::CalculateMovementRange(const FIntPoint& FromPosition, int32 BaseMoveDistance) const
{
    float MovementDifficulty = GetMovementDifficultyAt(FromPosition);
    
    // 移動可能マス数計算: BaseMoveDistance * MovementDifficulty
    float RawMovement = BaseMoveDistance * MovementDifficulty;
    
    // 1未満は1、小数点以下切り捨て
    int32 ActualMovement = FMath::Max(1, FMath::FloorToInt(RawMovement));
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MapGenerator: Movement at (%d,%d): Base=%d, Difficulty=%.2f, Actual=%d"),
        FromPosition.X, FromPosition.Y, BaseMoveDistance, MovementDifficulty, ActualMovement);
    
    return ActualMovement;
}

float UMapGeneratorComponent::GetMovementDifficultyAt(const FIntPoint& Position) const
{
    if (!LocationDataTableManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("MapGenerator: LocationDataTableManager not set, using default difficulty"));
        return 1.0f;
    }

    if (const FGridCellData* CellData = GeneratedMapData.Find(Position))
    {
        // LocationTypeから難易度を取得
        FString LocationTypeName = CellData->LocationType.GetTagName().ToString();
        
        // "Location.Forest" → "forest" に変換
        FString SimplifiedName;
        if (LocationTypeName.Split(TEXT("."), nullptr, &SimplifiedName))
        {
            LocationTypeName = SimplifiedName.ToLower();
        }
        
        return LocationDataTableManager->GetLocationMovementDifficulty(LocationTypeName);
    }
    
    // デフォルト値
    return 1.0f;
}

int32 UMapGeneratorComponent::GetDistanceFromBase(const FIntPoint& Position) const
{
    return CalculateDistance(Position, BasePosition);
}

float UMapGeneratorComponent::GetEffectiveDifficultyAt(const FIntPoint& Position) const
{
    if (!LocationDataTableManager)
    {
        return 1.0f;
    }

    int32 DistanceFromBase = GetDistanceFromBase(Position);
    
    // 基本難易度
    float BaseDifficulty = 1.0f;
    if (const FGridCellData* CellData = GeneratedMapData.Find(Position))
    {
        FString LocationTypeName = CellData->LocationType.GetTagName().ToString();
        FString SimplifiedName;
        if (LocationTypeName.Split(TEXT("."), nullptr, &SimplifiedName))
        {
            LocationTypeName = SimplifiedName.ToLower();
        }
        
        BaseDifficulty = LocationDataTableManager->GetLocationDifficultyLevel(LocationTypeName);
    }
    
    // 距離による難易度増加
    float DistanceMultiplier = 1.0f + (DistanceFromBase * DifficultyMultiplierPerDistance);
    DistanceMultiplier = FMath::Min(DistanceMultiplier, MaxDifficultyMultiplier);
    
    return BaseDifficulty * DistanceMultiplier;
}

// === 内部生成アルゴリズム ===

void UMapGeneratorComponent::InitializeGeneration()
{
    GeneratedMapData.Empty();
    GenerationStats = FMapGenerationStats();
    GenerationStats.BiomeCounts.Empty();
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MapGenerator: Generation initialized"));
}

void UMapGeneratorComponent::PlaceBase()
{
    PlaceLocationTypeAt(BasePosition, "base");
    
    UE_LOG(LogTemp, Log, TEXT("🏠 MapGenerator: Base placed at (%d, %d)"), BasePosition.X, BasePosition.Y);
}

void UMapGeneratorComponent::GenerateBiomes()
{
    for (const FBiomeGenerationRule& Rule : BiomeRules)
    {
        PlaceBiome(Rule);
    }
}

void UMapGeneratorComponent::PlaceBiome(const FBiomeGenerationRule& Rule)
{
    int32 PlacedCount = 0;
    int32 Attempts = 0;
    const int32 MaxAttempts = Rule.MaxBiomeCount * 10;
    
    while (PlacedCount < Rule.MaxBiomeCount && Attempts < MaxAttempts)
    {
        Attempts++;
        
        // ランダム位置生成
        FIntPoint CandidatePosition = GetRandomPositionInRange(
            BasePosition, 
            Rule.MinDistanceFromBase, 
            Rule.MaxDistanceFromBase
        );
        
        // 確率チェック
        if (FMath::RandRange(0.0f, 1.0f) > Rule.GenerationChance)
        {
            continue;
        }
        
        // 配置妥当性チェック
        if (!IsValidPlacement(CandidatePosition, Rule.BiomeName))
        {
            continue;
        }
        
        // バイオーム配置
        PlaceInRadius(CandidatePosition, Rule.BiomeRadius, Rule.BiomeName, 0.8f);
        PlacedCount++;
        
        // 統計更新
        int32* Count = GenerationStats.BiomeCounts.Find(Rule.BiomeName);
        if (Count)
        {
            (*Count)++;
        }
        else
        {
            GenerationStats.BiomeCounts.Add(Rule.BiomeName, 1);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("🌍 MapGenerator: Placed %d %s biomes"), PlacedCount, *Rule.BiomeName);
}

void UMapGeneratorComponent::FillWithDefaultTerrain()
{
    // 空いている場所を平原で埋める
    for (int32 x = 0; x < MapWidth; x++)
    {
        for (int32 y = 0; y < MapHeight; y++)
        {
            FIntPoint Position(x, y);
            
            if (!GeneratedMapData.Contains(Position))
            {
                PlaceLocationTypeAt(Position, "plains");
            }
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MapGenerator: Filled empty areas with default terrain"));
}

void UMapGeneratorComponent::PlaceLocationTypeAt(const FIntPoint& Position, const FString& LocationTypeName)
{
    if (!IsInMapBounds(Position))
    {
        return;
    }
    
    FGridCellData CellData = CreateGridCellFromLocationData(LocationTypeName);
    GeneratedMapData.Add(Position, CellData);
}

void UMapGeneratorComponent::PlaceInRadius(const FIntPoint& Center, int32 Radius, const FString& LocationTypeName, float Chance)
{
    for (int32 dx = -Radius; dx <= Radius; dx++)
    {
        for (int32 dy = -Radius; dy <= Radius; dy++)
        {
            FIntPoint Position = Center + FIntPoint(dx, dy);
            
            // 円形範囲チェック
            if (CalculateDistance(Center, Position) > Radius)
            {
                continue;
            }
            
            // 確率チェック
            if (FMath::RandRange(0.0f, 1.0f) > Chance)
            {
                continue;
            }
            
            PlaceLocationTypeAt(Position, LocationTypeName);
        }
    }
}

bool UMapGeneratorComponent::IsValidPlacement(const FIntPoint& Position, const FString& LocationTypeName) const
{
    // 境界チェック
    if (!IsInMapBounds(Position))
    {
        return false;
    }
    
    // 拠点に近すぎる場合はNG（拠点以外）
    if (LocationTypeName != "base" && CalculateDistance(Position, BasePosition) < 2)
    {
        return false;
    }
    
    // 既に配置済みの場合はNG
    if (GeneratedMapData.Contains(Position))
    {
        return false;
    }
    
    return true;
}

void UMapGeneratorComponent::ApplyDifficultyScaling()
{
    for (auto& CellPair : GeneratedMapData)
    {
        const FIntPoint& Position = CellPair.Key;
        FGridCellData& CellData = CellPair.Value;
        
        float DifficultyMultiplier = GetEffectiveDifficultyAt(Position);
        ScaleEnemiesAndResources(CellData, DifficultyMultiplier);
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MapGenerator: Applied difficulty scaling to all cells"));
}

void UMapGeneratorComponent::ScaleEnemiesAndResources(FGridCellData& CellData, float DifficultyMultiplier) const
{
    // 注意：実際の敵・リソース調整は複雑な実装が必要
    // ここでは基本的な仕組みのみ実装
    
    // DifficultyMultiplierに基づいてリソース量や敵の強さを調整
    // 実装例：AvailableResourcesの数量調整、敵のスポーン率調整など
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("MapGenerator: Scaled difficulty by %.2f"), DifficultyMultiplier);
}

// === ユーティリティ関数 ===

FIntPoint UMapGeneratorComponent::GetRandomPositionInRange(const FIntPoint& Center, int32 MinRadius, int32 MaxRadius) const
{
    int32 Attempts = 0;
    const int32 MaxAttempts = 100;
    
    while (Attempts < MaxAttempts)
    {
        Attempts++;
        
        // ランダム角度と距離
        float Angle = FMath::RandRange(0.0f, 2.0f * PI);
        int32 Distance = FMath::RandRange(MinRadius, MaxRadius);
        
        // 位置計算
        int32 dx = FMath::RoundToInt(Distance * FMath::Cos(Angle));
        int32 dy = FMath::RoundToInt(Distance * FMath::Sin(Angle));
        
        FIntPoint Position = Center + FIntPoint(dx, dy);
        
        if (IsInMapBounds(Position))
        {
            return Position;
        }
    }
    
    // フォールバック
    return Center + FIntPoint(MinRadius, 0);
}

int32 UMapGeneratorComponent::CalculateDistance(const FIntPoint& A, const FIntPoint& B) const
{
    int32 dx = FMath::Abs(A.X - B.X);
    int32 dy = FMath::Abs(A.Y - B.Y);
    return FMath::Max(dx, dy); // チェビシェフ距離（8方向移動）
}

bool UMapGeneratorComponent::IsInMapBounds(const FIntPoint& Position) const
{
    return Position.X >= 0 && Position.X < MapWidth && 
           Position.Y >= 0 && Position.Y < MapHeight;
}

FGridCellData UMapGeneratorComponent::CreateGridCellFromLocationData(const FString& LocationTypeName) const
{
    FGridCellData CellData;
    
    // GameplayTag設定
    FString TagName = FString::Printf(TEXT("Location.%s"), *LocationTypeName);
    TagName = TagName.Replace(TEXT("Location.Location."), TEXT("Location.")); // 重複防止
    CellData.LocationType = FGameplayTag::RequestGameplayTag(*TagName);
    
    // LocationDataTableManagerから設定取得
    if (LocationDataTableManager)
    {
        CellData.MovementCost = LocationDataTableManager->GetLocationMovementCost(LocationTypeName);
        CellData.bIsWalkable = LocationDataTableManager->IsLocationWalkable(LocationTypeName);
        
        // AvailableResourcesの設定（GatherableItemsから生成）
        TArray<FGatherableItemInfo> GatherableItems = LocationDataTableManager->GetGatherableItems(LocationTypeName);
        for (const FGatherableItemInfo& ItemInfo : GatherableItems)
        {
            FString ResourceTagName = FString::Printf(TEXT("Resource.%s"), *ItemInfo.ItemId);
            CellData.AvailableResources.Add(FGameplayTag::RequestGameplayTag(*ResourceTagName));
        }
    }
    else
    {
        // フォールバック設定
        CellData.MovementCost = 1.0f;
        CellData.bIsWalkable = true;
        
        UE_LOG(LogTemp, Warning, TEXT("MapGenerator: LocationDataTableManager not set, using fallback values for %s"), *LocationTypeName);
    }
    
    return CellData;
}

void UMapGeneratorComponent::PreviewMapGeneration()
{
    UE_LOG(LogTemp, Warning, TEXT("=== MAP GENERATION PREVIEW ==="));
    UE_LOG(LogTemp, Warning, TEXT("Base Position: (%d, %d)"), BasePosition.X, BasePosition.Y);
    UE_LOG(LogTemp, Warning, TEXT("Map Size: %dx%d"), MapWidth, MapHeight);
    UE_LOG(LogTemp, Warning, TEXT("Random Seed: %d"), RandomSeed);
    UE_LOG(LogTemp, Warning, TEXT("Biome Rules: %d"), BiomeRules.Num());
    
    for (const FBiomeGenerationRule& Rule : BiomeRules)
    {
        UE_LOG(LogTemp, Warning, TEXT("- %s: Distance %d-%d, Chance %.1f%%, Radius %d, Max %d"), 
            *Rule.BiomeName, Rule.MinDistanceFromBase, Rule.MaxDistanceFromBase, 
            Rule.GenerationChance * 100.0f, Rule.BiomeRadius, Rule.MaxBiomeCount);
    }
}

void UMapGeneratorComponent::ManualPlaceLocationType(const FIntPoint& Position, const FString& LocationTypeName, UGridMapComponent* TargetGridMap)
{
    if (!TargetGridMap)
    {
        return;
    }
    
    PlaceLocationTypeAt(Position, LocationTypeName);
    
    if (const FGridCellData* CellData = GeneratedMapData.Find(Position))
    {
        TargetGridMap->SetCellData(Position, *CellData);
        
        UE_LOG(LogTemp, Log, TEXT("MapGenerator: Manually placed %s at (%d, %d)"), 
            *LocationTypeName, Position.X, Position.Y);
    }
}