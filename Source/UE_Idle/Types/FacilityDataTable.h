#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FacilityDataTable.generated.h"

UENUM(BlueprintType)
enum class EFacilityType : uint8
{
    Housing         UMETA(DisplayName = "住居"),
    Production      UMETA(DisplayName = "生産施設"),
    Military        UMETA(DisplayName = "軍事施設"),
    Research        UMETA(DisplayName = "研究施設"),
    Storage         UMETA(DisplayName = "貯蔵施設"),
    Utility         UMETA(DisplayName = "ユーティリティ")
};

UENUM(BlueprintType)
enum class EFacilitySubType : uint8
{
    // Housing
    SmallHouse      UMETA(DisplayName = "小屋"),
    House           UMETA(DisplayName = "家"),
    Mansion         UMETA(DisplayName = "大邸宅"),
    
    // Production
    Kitchen         UMETA(DisplayName = "調理場"),
    Workshop        UMETA(DisplayName = "作業台"),
    Forge           UMETA(DisplayName = "鍛冶場"),
    TailorShop      UMETA(DisplayName = "裁縫台"),
    AlchemyLab      UMETA(DisplayName = "錬金術工房"),
    
    // Military
    TrainingGround  UMETA(DisplayName = "訓練所"),
    Barracks        UMETA(DisplayName = "兵舎"),
    DefenseTower    UMETA(DisplayName = "防衛塔"),
    
    // Research
    Library         UMETA(DisplayName = "図書館"),
    Laboratory      UMETA(DisplayName = "研究所"),
    
    // Storage
    Warehouse       UMETA(DisplayName = "倉庫"),
    Granary         UMETA(DisplayName = "穀物庫"),
    
    // Utility
    Well            UMETA(DisplayName = "井戸"),
    Field           UMETA(DisplayName = "畑"),
    Mine            UMETA(DisplayName = "鉱山")
};

UENUM(BlueprintType)
enum class EFacilityState : uint8
{
    Planning        UMETA(DisplayName = "計画中"),
    Constructing    UMETA(DisplayName = "建設中"),
    Active          UMETA(DisplayName = "稼働中"),
    Damaged         UMETA(DisplayName = "損傷"),
    Broken          UMETA(DisplayName = "破壊"),
    Upgrading       UMETA(DisplayName = "アップグレード中")
};

UENUM(BlueprintType)
enum class EFacilityEffectType : uint8
{
    // 即時効果
    UnlockRecipe        UMETA(DisplayName = "レシピ解放"),
    UnlockItem          UMETA(DisplayName = "アイテム解放"),
    UnlockFacility      UMETA(DisplayName = "施設解放"),
    
    // 継続効果
    ProductionSpeed     UMETA(DisplayName = "生産速度"),
    ResourceGain        UMETA(DisplayName = "資源獲得量"),
    StorageCapacity     UMETA(DisplayName = "貯蔵容量"),
    PopulationCap       UMETA(DisplayName = "人口上限"),
    MoraleBonus         UMETA(DisplayName = "士気ボーナス"),
    DefenseBonus        UMETA(DisplayName = "防御ボーナス"),
    ResearchSpeed       UMETA(DisplayName = "研究速度"),
    TrainingSpeed       UMETA(DisplayName = "訓練速度"),
    
    // 特殊効果
    AutoProduction      UMETA(DisplayName = "自動生産"),
    ResourceConversion  UMETA(DisplayName = "資源変換")
};

// 建設/アップグレード必要資源
USTRUCT(BlueprintType)
struct FFacilityResourceCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    int32 BaseAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float LevelExponent = 1.5f;  // レベルごとの増加率
};

// 施設効果
USTRUCT(BlueprintType)
struct FFacilityEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EFacilityEffectType EffectType = EFacilityEffectType::UnlockRecipe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString TargetId;  // レシピID、アイテムID、施設ID等

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float BaseValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float ValuePerLevel = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 RequiredLevel = 1;  // この効果が発動する最小レベル
};

// 施設依存関係
USTRUCT(BlueprintType)
struct FFacilityDependency
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dependency")
    FString RequiredFacilityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dependency")
    int32 RequiredLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dependency")
    bool bMustBeAdjacent = false;
};

// DataTable行構造
USTRUCT(BlueprintType)
struct UE_IDLE_API FFacilityDataRow : public FTableRowBase
{
    GENERATED_BODY()

    // 基本情報
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EFacilityType FacilityType = EFacilityType::Production;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EFacilitySubType SubType = EFacilitySubType::Workshop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 MaxLevel = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    int32 BaseDurability = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float DurabilityPerLevel = 500.0f;

    // 建設パラメータ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
    float BaseConstructionTime = 60.0f;  // 秒

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
    float TimePerLevel = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
    int32 RequiredWorkers = 1;

    // コスト
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    TArray<FFacilityResourceCost> ConstructionCosts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    TArray<FFacilityResourceCost> UpgradeCosts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    TArray<FFacilityResourceCost> MaintenanceCosts;  // 定期メンテナンス

    // 効果
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FFacilityEffect> Effects;

    // 依存関係
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dependencies")
    TArray<FFacilityDependency> Dependencies;

    // ビジュアル
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FString MeshPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FString IconPath;

    // ヘルパー関数
    int32 GetMaxDurabilityAtLevel(int32 Level) const
    {
        return BaseDurability + FMath::RoundToInt(DurabilityPerLevel * (Level - 1));
    }

    float GetConstructionTimeAtLevel(int32 Level) const
    {
        return BaseConstructionTime + TimePerLevel * (Level - 1);
    }

    int32 GetResourceCostAtLevel(const FFacilityResourceCost& BaseCost, int32 Level) const
    {
        return FMath::RoundToInt(BaseCost.BaseAmount * FMath::Pow(Level, BaseCost.LevelExponent));
    }

    float GetEffectValueAtLevel(const FFacilityEffect& Effect, int32 Level) const
    {
        if (Level < Effect.RequiredLevel) return 0.0f;
        return Effect.BaseValue + Effect.ValuePerLevel * (Level - 1);
    }

    bool CanBeConstructed(int32 PlayerLevel) const
    {
        // プレイヤーレベルによる制限（後で実装）
        return true;
    }
};

// 個別の施設インスタンス
USTRUCT(BlueprintType)
struct FFacilityInstance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    FGuid InstanceId;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    FString FacilityId;  // DataTable RowName

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    int32 Level = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    EFacilityState State = EFacilityState::Planning;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    int32 CurrentDurability = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    float CompletionProgress = 0.0f;  // 0-100 建設/アップグレード進捗

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    FVector Location;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    int32 AssignedWorkers = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    float LastMaintenanceTime = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    TMap<FString, float> CustomData;  // 拡張用

    FFacilityInstance()
    {
        InstanceId = FGuid::NewGuid();
    }
};