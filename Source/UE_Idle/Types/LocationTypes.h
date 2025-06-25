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

    FLocationDataRow()
    {
        Name = TEXT("");
        Description = TEXT("");
        LocationType = ELocationType::Base;
        EnemySpawnListString = TEXT("");
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
};