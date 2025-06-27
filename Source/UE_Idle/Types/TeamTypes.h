#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "../Types/ItemTypes.h"
#include "TeamTypes.generated.h"

// タスクタイプ
UENUM(BlueprintType)
enum class ETaskType : uint8
{
    Idle        UMETA(DisplayName = "待機"),
    Free        UMETA(DisplayName = "自由"),
    Adventure   UMETA(DisplayName = "冒険"),
    Cooking     UMETA(DisplayName = "料理")
};

// 運搬手段（後に生産システムで追加予定）
UENUM(BlueprintType)
enum class ECarrierType : uint8
{
    None            UMETA(DisplayName = "なし"),
    HandCart        UMETA(DisplayName = "手押し車"),      // +50kg
    SmallWagon      UMETA(DisplayName = "小型馬車"),      // +100kg
    LargeWagon      UMETA(DisplayName = "大型馬車"),      // +200kg
    MagicBag        UMETA(DisplayName = "魔法の袋")       // +500kg
};

class AC_IdleCharacter;

// チーム構造体
USTRUCT(BlueprintType)
struct FTeam
{
    GENERATED_BODY()

    // チームメンバー
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    TArray<AC_IdleCharacter*> Members;

    // 割り当てられたタスク
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    ETaskType AssignedTask;

    // チーム名
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    FString TeamName;

    // チームがアクティブか
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    bool bIsActive;

    // 冒険先の場所ID（AdventureタスクでのみU使用）
    UPROPERTY(BlueprintReadWrite, Category = "Adventure")
    FString AdventureLocationId;

    // 戦闘中かどうか
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bInCombat;

    // チーム運搬手段
    UPROPERTY(BlueprintReadWrite, Category = "Team Carrying")
    ECarrierType CarrierType;

    // チーム基本積載量
    UPROPERTY(BlueprintReadWrite, Category = "Team Carrying")
    float BaseCarryingCapacity;

    FTeam()
    {
        AssignedTask = ETaskType::Idle;  // デフォルトは待機
        bIsActive = true;
        AdventureLocationId = TEXT("");
        bInCombat = false;
        CarrierType = ECarrierType::None;
        BaseCarryingCapacity = 0.0f;
    }

    // 総積載量計算（基本積載量 + 運搬手段ボーナス）
    float GetTotalCarryingCapacity() const
    {
        float CarrierBonus = 0.0f;
        switch (CarrierType)
        {
            case ECarrierType::HandCart:
                CarrierBonus = 50.0f;
                break;
            case ECarrierType::SmallWagon:
                CarrierBonus = 100.0f;
                break;
            case ECarrierType::LargeWagon:
                CarrierBonus = 200.0f;
                break;
            case ECarrierType::MagicBag:
                CarrierBonus = 500.0f;
                break;
            case ECarrierType::None:
            default:
                CarrierBonus = 0.0f;
                break;
        }
        return BaseCarryingCapacity + CarrierBonus;
    }
};