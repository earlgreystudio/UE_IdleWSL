#pragma once

#include "CoreMinimal.h"
#include "CommonTypes.generated.h"

// 共通で使用される列挙型
// 循環依存を避けるために独立したファイルに定義

// チームアクション状態
UENUM(BlueprintType)
enum class ETeamActionState : uint8
{
    Idle            UMETA(DisplayName = "待機"),
    Moving          UMETA(DisplayName = "移動中"),       // 中断可能
    Returning       UMETA(DisplayName = "帰還中"),       // 中断可能
    Working         UMETA(DisplayName = "作業中"),       // 中断可能
    InCombat        UMETA(DisplayName = "戦闘中"),       // 中断不可
    Locked          UMETA(DisplayName = "アクション中")   // 中断不可
};