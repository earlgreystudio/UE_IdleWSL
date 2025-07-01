#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "AttributeTypes.generated.h"

// Modifier種別
UENUM(BlueprintType)
enum class EModifierType : uint8
{
    Additive        UMETA(DisplayName = "加算"),
    Multiplicative  UMETA(DisplayName = "乗算"),
    Override        UMETA(DisplayName = "上書き"),
    Max             UMETA(DisplayName = "最大値制限"),
    Min             UMETA(DisplayName = "最小値制限")
};

// 属性Modifier構造体
USTRUCT(BlueprintType)
struct UE_IDLE_API FAttributeModifier
{
    GENERATED_BODY()

    // 一意識別子
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FString ModifierId;

    // 表示名
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    FString DisplayName;

    // Modifier種別
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    EModifierType Type = EModifierType::Additive;

    // 固定属性への修正値（FCharacterStatus、FCharacterTalent用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    TMap<FString, float> StatModifiers;

    // GameplayTag属性への修正値（拡張属性用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    TMap<FGameplayTag, float> TagModifiers;

    // 持続時間（秒、-1で永続）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    float Duration = -1.0f;

    // 適用開始時刻（内部管理用）
    UPROPERTY(BlueprintReadOnly, Category = "Modifier")
    float StartTime = 0.0f;

    // スタック可能性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    bool bStackable = false;

    // 現在のスタック数
    UPROPERTY(BlueprintReadOnly, Category = "Modifier")
    int32 StackCount = 1;

    // 最大スタック数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    int32 MaxStack = 1;

    FAttributeModifier()
    {
        ModifierId = TEXT("");
        DisplayName = TEXT("");
        Type = EModifierType::Additive;
        Duration = -1.0f;
        StartTime = 0.0f;
        bStackable = false;
        StackCount = 1;
        MaxStack = 1;
    }

    // 有効期限チェック
    bool IsExpired(float CurrentTime) const
    {
        if (Duration < 0.0f) return false; // 永続
        return (CurrentTime - StartTime) >= Duration;
    }

    // 同一Modifierかチェック
    bool IsSameModifier(const FAttributeModifier& Other) const
    {
        return ModifierId == Other.ModifierId;
    }
};

// 時限Modifier（タイマー管理用）
USTRUCT(BlueprintType)
struct UE_IDLE_API FTimedModifier
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Timed Modifier")
    FAttributeModifier Modifier;

    UPROPERTY(BlueprintReadWrite, Category = "Timed Modifier")
    FTimerHandle TimerHandle;

    FTimedModifier()
    {
        // デフォルト値は不要
    }

    FTimedModifier(const FAttributeModifier& InModifier)
        : Modifier(InModifier)
    {
    }
};