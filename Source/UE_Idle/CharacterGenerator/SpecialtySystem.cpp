#include "SpecialtySystem.h"
#include "Engine/Engine.h"

ESpecialtyType USpecialtySystem::GetRandomSpecialty()
{
    // 重み付き確率分布
    // 通常専門: 重み6
    // 相撲・看護: 重み2 (1/3の確率)
    // 馬術・医学・ロボット: 重み1 (1/6の確率)
    
    struct SpecialtyWeight
    {
        ESpecialtyType Specialty;
        int32 Weight;
    };
    
    const SpecialtyWeight SpecialtyWeights[] = {
        { ESpecialtyType::Baseball, 6 },
        { ESpecialtyType::Kendo, 6 },
        { ESpecialtyType::Chemistry, 6 },
        { ESpecialtyType::Archery, 6 },
        { ESpecialtyType::Karate, 6 },
        { ESpecialtyType::AmericanFootball, 6 },
        { ESpecialtyType::Golf, 6 },
        { ESpecialtyType::TrackAndField, 6 },
        { ESpecialtyType::Drama, 6 },
        { ESpecialtyType::TeaCeremony, 6 },
        { ESpecialtyType::Equestrian, 1 },      // レア専門
        { ESpecialtyType::Robotics, 1 },        // レア専門
        { ESpecialtyType::Gardening, 6 },
        { ESpecialtyType::Astronomy, 6 },
        { ESpecialtyType::TableTennis, 6 },
        { ESpecialtyType::Basketball, 6 },
        { ESpecialtyType::Badminton, 6 },
        { ESpecialtyType::Tennis, 6 },
        { ESpecialtyType::Volleyball, 6 },
        { ESpecialtyType::Soccer, 6 },
        { ESpecialtyType::Sumo, 2 },            // 低確率
        { ESpecialtyType::Cooking, 6 },
        { ESpecialtyType::Medical, 1 },         // レア専門
        { ESpecialtyType::Nursing, 2 }          // 低確率
    };
    
    // 重みの合計を計算
    int32 TotalWeight = 0;
    for (const auto& SpecialtyWeight : SpecialtyWeights)
    {
        TotalWeight += SpecialtyWeight.Weight;
    }
    
    // ランダム値を生成
    int32 RandomValue = FMath::RandRange(0, TotalWeight - 1);
    
    // 重み付き選択
    int32 AccumulatedWeight = 0;
    for (const auto& SpecialtyWeight : SpecialtyWeights)
    {
        AccumulatedWeight += SpecialtyWeight.Weight;
        if (RandomValue < AccumulatedWeight)
        {
            return SpecialtyWeight.Specialty;
        }
    }
    
    // フォールバック（通常は到達しない）
    return ESpecialtyType::Baseball;
}