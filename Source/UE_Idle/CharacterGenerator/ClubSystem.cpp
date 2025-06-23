#include "ClubSystem.h"
#include "Engine/Engine.h"

EClubType UClubSystem::GetRandomClub()
{
    // 重み付き確率分布
    // 通常部活: 重み6
    // 相撲・看護: 重み2 (1/3の確率)
    // 馬術・医学・ロボット: 重み1 (1/6の確率)
    
    struct ClubWeight
    {
        EClubType Club;
        int32 Weight;
    };
    
    const ClubWeight ClubWeights[] = {
        { EClubType::Baseball, 6 },
        { EClubType::Kendo, 6 },
        { EClubType::Chemistry, 6 },
        { EClubType::Archery, 6 },
        { EClubType::Karate, 6 },
        { EClubType::AmericanFootball, 6 },
        { EClubType::Golf, 6 },
        { EClubType::TrackAndField, 6 },
        { EClubType::Drama, 6 },
        { EClubType::TeaCeremony, 6 },
        { EClubType::Equestrian, 1 },      // レア部活
        { EClubType::Robotics, 1 },        // レア部活
        { EClubType::Gardening, 6 },
        { EClubType::Astronomy, 6 },
        { EClubType::TableTennis, 6 },
        { EClubType::Basketball, 6 },
        { EClubType::Badminton, 6 },
        { EClubType::Tennis, 6 },
        { EClubType::Volleyball, 6 },
        { EClubType::Soccer, 6 },
        { EClubType::Sumo, 2 },            // 低確率
        { EClubType::Cooking, 6 },
        { EClubType::Medical, 1 },         // レア部活
        { EClubType::Nursing, 2 }          // 低確率
    };
    
    // 重みの合計を計算
    int32 TotalWeight = 0;
    for (const auto& ClubWeight : ClubWeights)
    {
        TotalWeight += ClubWeight.Weight;
    }
    
    // ランダム値を生成
    int32 RandomValue = FMath::RandRange(0, TotalWeight - 1);
    
    // 重み付き選択
    int32 AccumulatedWeight = 0;
    for (const auto& ClubWeight : ClubWeights)
    {
        AccumulatedWeight += ClubWeight.Weight;
        if (RandomValue < AccumulatedWeight)
        {
            return ClubWeight.Club;
        }
    }
    
    // フォールバック（通常は到達しない）
    return EClubType::Baseball;
}