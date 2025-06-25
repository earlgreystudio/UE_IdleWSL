#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
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

    FTeam()
    {
        AssignedTask = ETaskType::Idle;  // デフォルトは待機
        bIsActive = true;
        AdventureLocationId = TEXT("");
        bInCombat = false;
    }
};