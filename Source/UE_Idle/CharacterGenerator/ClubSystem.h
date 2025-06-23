#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ClubSystem.generated.h"

UENUM(BlueprintType)
enum class EClubType : uint8
{
    Baseball        UMETA(DisplayName = "野球部"),
    Kendo          UMETA(DisplayName = "剣道部"),
    Chemistry      UMETA(DisplayName = "化学部"),
    Archery        UMETA(DisplayName = "アーチェリー部"),
    Karate         UMETA(DisplayName = "空手部"),
    AmericanFootball UMETA(DisplayName = "アメフト部"),
    Golf           UMETA(DisplayName = "ゴルフ部"),
    TrackAndField  UMETA(DisplayName = "陸上部"),
    Drama          UMETA(DisplayName = "演劇部"),
    TeaCeremony    UMETA(DisplayName = "茶道部"),
    Equestrian     UMETA(DisplayName = "馬術部"),
    Robotics       UMETA(DisplayName = "ロボット研究会"),
    Gardening      UMETA(DisplayName = "園芸部"),
    Astronomy      UMETA(DisplayName = "天文部"),
    TableTennis    UMETA(DisplayName = "卓球部"),
    Basketball     UMETA(DisplayName = "バスケ部"),
    Badminton      UMETA(DisplayName = "バドミントン部"),
    Tennis         UMETA(DisplayName = "テニス部"),
    Volleyball     UMETA(DisplayName = "バレー部"),
    Soccer         UMETA(DisplayName = "サッカー部"),
    Sumo           UMETA(DisplayName = "相撲部"),
    Cooking        UMETA(DisplayName = "料理部"),
    Medical        UMETA(DisplayName = "医学部"),
    Nursing        UMETA(DisplayName = "看護部"),
    
    Count          UMETA(Hidden)
};

UCLASS()
class UE_IDLE_API UClubSystem : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Club System")
    static EClubType GetRandomClub();
};