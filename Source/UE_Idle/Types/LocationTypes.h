#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LocationTypes.generated.h"

// 場所タイプ
UENUM(BlueprintType)
enum class ELocationType : uint8
{
    Base        UMETA(DisplayName = "拠点"),
    Plains      UMETA(DisplayName = "平野"),
    Swamp       UMETA(DisplayName = "沼地"),
    Cave        UMETA(DisplayName = "洞窟"),
    
    Count       UMETA(Hidden)
};

// 敵出現情報
USTRUCT(BlueprintType)
struct FEnemySpawnInfo
{
    GENERATED_BODY()

    // 敵のプリセットID (CharacterPresets.csvのRowName)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    FString PresetId;

    // 出現確率 (0.0 - 1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnProbability;

    FEnemySpawnInfo()
    {
        PresetId = TEXT("");
        SpawnProbability = 0.0f;
    }
};

// 採集可能アイテム情報
USTRUCT(BlueprintType)
struct FGatherableItemInfo
{
    GENERATED_BODY()

    // アイテムID (ItemData.csvのRowName)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gathering")
    FString ItemId;

    // 採取係数 (GatheringPower×係数÷40 = 秒あたり採取量)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gathering")
    float GatheringCoefficient;

    FGatherableItemInfo()
    {
        ItemId = TEXT("");
        GatheringCoefficient = 0.0f;
    }
};

// 場所データ構造体
USTRUCT(BlueprintType)
struct FLocationDataRow : public FTableRowBase
{
    GENERATED_BODY()

    // 場所名
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FString Name;

    // 説明
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FString Description;

    // 場所タイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    ELocationType LocationType;

    // 出現する敵リスト文字列 (CSV用: "rat:0.8|giant_frog:0.2")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies")
    FString EnemySpawnListString;

    // 移動コスト
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementCost;

    // 歩行可能フラグ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bIsWalkable;

    // 移動時の係数（新マップ生成システム用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementDifficulty;

    // 基本難易度レベル（新マップ生成システム用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    int32 DifficultyLevel;

    // 採集可能アイテムリスト文字列 (CSV用: "wood:2|stone:1")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gathering")
    FString GatherableItemsString;

    FLocationDataRow()
    {
        Name = TEXT("");
        Description = TEXT("");
        LocationType = ELocationType::Base;
        EnemySpawnListString = TEXT("");
        MovementCost = 1.0f;
        bIsWalkable = true;
        MovementDifficulty = 1.0f;
        DifficultyLevel = 1;
        GatherableItemsString = TEXT("");
    }

    // CSV文字列から敵出現情報を解析するヘルパー関数
    TArray<FEnemySpawnInfo> ParseEnemySpawnList() const
    {
        TArray<FEnemySpawnInfo> SpawnList;
        
        if (!EnemySpawnListString.IsEmpty())
        {
            TArray<FString> EnemyPairs;
            EnemySpawnListString.ParseIntoArray(EnemyPairs, TEXT("|"), true);

            for (const FString& EnemyPair : EnemyPairs)
            {
                FString PresetId;
                FString ProbabilityStr;
                if (EnemyPair.Split(TEXT(":"), &PresetId, &ProbabilityStr))
                {
                    FEnemySpawnInfo SpawnInfo;
                    SpawnInfo.PresetId = PresetId;
                    SpawnInfo.SpawnProbability = FCString::Atof(*ProbabilityStr);
                    SpawnList.Add(SpawnInfo);
                }
            }
        }

        return SpawnList;
    }

    // ランダムに敵を選択するヘルパー関数
    FString GetRandomEnemyPreset() const
    {
        TArray<FEnemySpawnInfo> SpawnList = ParseEnemySpawnList();
        
        if (SpawnList.Num() == 0)
        {
            return TEXT("");
        }

        // 累積確率を計算
        float TotalProbability = 0.0f;
        for (const FEnemySpawnInfo& Info : SpawnList)
        {
            TotalProbability += Info.SpawnProbability;
        }

        // ランダム値生成
        float RandomValue = FMath::FRand() * TotalProbability;

        // 累積確率で選択
        float CurrentSum = 0.0f;
        for (const FEnemySpawnInfo& Info : SpawnList)
        {
            CurrentSum += Info.SpawnProbability;
            if (RandomValue <= CurrentSum)
            {
                return Info.PresetId;
            }
        }

        // 念のため最後の要素を返す
        return SpawnList.Last().PresetId;
    }

    // CSV文字列から採集可能アイテム情報を解析するヘルパー関数
    TArray<FGatherableItemInfo> ParseGatherableItemsList() const
    {
        TArray<FGatherableItemInfo> GatherableList;
        
        if (!GatherableItemsString.IsEmpty())
        {
            TArray<FString> ItemPairs;
            GatherableItemsString.ParseIntoArray(ItemPairs, TEXT("|"), true);

            for (const FString& ItemPair : ItemPairs)
            {
                FString ItemId;
                FString CoefficientStr;
                if (ItemPair.Split(TEXT(":"), &ItemId, &CoefficientStr))
                {
                    FGatherableItemInfo GatherableInfo;
                    GatherableInfo.ItemId = ItemId;
                    GatherableInfo.GatheringCoefficient = FCString::Atof(*CoefficientStr);
                    GatherableList.Add(GatherableInfo);
                }
            }
        }

        return GatherableList;
    }

    // 採集可能アイテムが存在するかチェック
    bool HasGatherableItems() const
    {
        return !GatherableItemsString.IsEmpty();
    }

    // 採集可能アイテムIDリストを取得
    void GetGatherableItemIds(TArray<FString>& OutItemIds) const
    {
        OutItemIds.Empty();
        TArray<FGatherableItemInfo> GatherableList = ParseGatherableItemsList();
        
        for (const FGatherableItemInfo& ItemInfo : GatherableList)
        {
            OutItemIds.Add(ItemInfo.ItemId);
        }
    }
};