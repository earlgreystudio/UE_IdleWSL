#include "MapGeneratorComponent.h"

UMapGeneratorComponent::UMapGeneratorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: Created (Simple Version)"));
}

void UMapGeneratorComponent::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: Ready for simple map generation"));
}

void UMapGeneratorComponent::SetLocationDataTableManager(ULocationDataTableManager* InLocationManager)
{
    LocationDataTableManager = InLocationManager;
    UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: LocationDataTableManager set"));
}

void UMapGeneratorComponent::GenerateMap(UGridMapComponent* TargetGridMap)
{
    if (!TargetGridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("MapGeneratorComponent::GenerateMap - TargetGridMap is null"));
        return;
    }
    
    // GridMapComponentを初期化
    TargetGridMap->InitializeGrid();
    
    // テストグリッドを作成（既存のメソッドを活用）
    TargetGridMap->CreateTestGrid();
    
    // 拠点位置を記録
    BasePosition = FIntPoint(TargetGridMap->GridWidth / 2, TargetGridMap->GridHeight / 2);
    
    UE_LOG(LogTemp, Warning, TEXT("MapGeneratorComponent: Map generated successfully!"));
    UE_LOG(LogTemp, Warning, TEXT("- Grid Size: %dx%d"), TargetGridMap->GridWidth, TargetGridMap->GridHeight);
    UE_LOG(LogTemp, Warning, TEXT("- Base Position: (%d, %d)"), BasePosition.X, BasePosition.Y);
}

int32 UMapGeneratorComponent::CalculateMovementRange(const FIntPoint& FromPosition, int32 BaseMoveDistance) const
{
    return BaseMoveDistance; // 簡易実装
}

float UMapGeneratorComponent::GetMovementDifficultyAt(const FIntPoint& Position) const
{
    return 1.0f; // 簡易実装
}

int32 UMapGeneratorComponent::GetDistanceFromBase(const FIntPoint& Position) const
{
    return FMath::Abs(Position.X - BasePosition.X) + FMath::Abs(Position.Y - BasePosition.Y);
}

float UMapGeneratorComponent::GetEffectiveDifficultyAt(const FIntPoint& Position) const
{
    return 1.0f; // 簡易実装
}

// FMapGenerationStats UMapGeneratorComponent::GetGenerationStats() const
// {
//     return GenerationStats;
// }

void UMapGeneratorComponent::PreviewMapGeneration()
{
    UE_LOG(LogTemp, Warning, TEXT("MapGeneratorComponent: Simple preview"));
}

void UMapGeneratorComponent::ManualPlaceLocationType(const FIntPoint& Position, const FString& LocationTypeName, UGridMapComponent* TargetGridMap)
{
    UE_LOG(LogTemp, Log, TEXT("MapGeneratorComponent: Manual placement at (%d, %d)"), Position.X, Position.Y);
}