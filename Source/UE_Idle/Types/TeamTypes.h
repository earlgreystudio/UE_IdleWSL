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

// 運搬手段
UENUM(BlueprintType)
enum class ECarrierType : uint8
{
    Bag             UMETA(DisplayName = "袋"),            // 20kg * メンバー数
    HandCart        UMETA(DisplayName = "手押し車"),      // 50kg * メンバー数
    Wagon           UMETA(DisplayName = "荷車")           // 100kg * メンバー数
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
        CarrierType = ECarrierType::Bag;
        BaseCarryingCapacity = 0.0f;
    }

    // 総積載量計算（運搬手段 * メンバー数）
    float GetTotalCarryingCapacity() const
    {
        int32 MemberCount = Members.Num();
        if (MemberCount == 0) MemberCount = 1; // 最低1人分として計算
        
        float CarrierCapacityPerMember = 0.0f;
        switch (CarrierType)
        {
            case ECarrierType::Bag:
                CarrierCapacityPerMember = 20.0f;
                break;
            case ECarrierType::HandCart:
                CarrierCapacityPerMember = 50.0f;
                break;
            case ECarrierType::Wagon:
                CarrierCapacityPerMember = 100.0f;
                break;
            default:
                CarrierCapacityPerMember = 20.0f; // デフォルトは袋
                break;
        }
        return CarrierCapacityPerMember * MemberCount;
    }
    
    // 運搬手段の表示名を取得
    FString GetCarrierDisplayName() const
    {
        switch (CarrierType)
        {
            case ECarrierType::Bag:
                return TEXT("袋");
            case ECarrierType::HandCart:
                return TEXT("手押し車");
            case ECarrierType::Wagon:
                return TEXT("荷車");
            default:
                return TEXT("袋");
        }
    }
};