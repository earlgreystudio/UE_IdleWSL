#include "CharacterTalent.h"
#include "Engine/Engine.h"

// 静的変数の初期化（爪牙システム推奨値）
float UCharacterTalentGenerator::BaseStatRangeMultiplier = 0.4f;   // 1-12 (30 * 0.4 = 12)
float UCharacterTalentGenerator::SpecialtyBonusRangeMultiplier = 0.375f; // 37.5%

FCharacterTalent UCharacterTalentGenerator::GenerateRandomTalent()
{
    FCharacterTalent Talent;
    
    // 係数ベースの基本能力値計算（調整可能）
    float MaxBaseStat = 30.0f * BaseStatRangeMultiplier; // デフォルト: 30 * 0.4 = 12
    
    Talent.Strength = FMath::RandRange(1.0f, MaxBaseStat);
    Talent.Toughness = FMath::RandRange(1.0f, MaxBaseStat);
    Talent.Intelligence = FMath::RandRange(1.0f, MaxBaseStat);
    Talent.Dexterity = FMath::RandRange(1.0f, MaxBaseStat);
    Talent.Agility = FMath::RandRange(1.0f, MaxBaseStat);
    Talent.Willpower = FMath::RandRange(1.0f, MaxBaseStat);
    
    // スキルの個数をランダムに決定 (1-4)
    int32 SkillCount = FMath::RandRange(1, 4);
    
    // 選択済みスキルを追跡
    TArray<ESkillType> SelectedSkills;
    
    // 指定個数分のスキルを選択
    for (int32 i = 0; i < SkillCount; i++)
    {
        ESkillType NewSkill;
        bool bValidSkill = false;
        
        // 重複しないスキルが見つかるまでループ
        while (!bValidSkill)
        {
            // ランダムにスキルを選択
            NewSkill = static_cast<ESkillType>(FMath::RandRange(0, static_cast<int32>(ESkillType::Count) - 1));
            
            // 既に選択されていないか確認
            if (!SelectedSkills.Contains(NewSkill))
            {
                bValidSkill = true;
                SelectedSkills.Add(NewSkill);
            }
        }
        
        // スキルと値を設定
        FSkillTalent SkillTalent;
        SkillTalent.SkillType = NewSkill;
        SkillTalent.Value = FMath::RandRange(1.0f, 20.0f);
        
        Talent.Skills.Add(SkillTalent);
    }
    
    return Talent;
}

FCharacterTalent UCharacterTalentGenerator::ApplySpecialtyBonus(const FCharacterTalent& BaseTalent, ESpecialtyType SpecialtyType)
{
    FCharacterTalent ResultTalent = BaseTalent;
    FSpecialtyBonus SpecialtyBonus = GetSpecialtyBonus(SpecialtyType);
    
    // 係数ベースの基本能力値ボーナス加算（調整可能）
    if (SpecialtyBonus.Strength > 0.0f)
    {
        float AdjustedBonus = SpecialtyBonus.Strength * SpecialtyBonusRangeMultiplier;
        float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
        ResultTalent.Strength = FMath::Clamp(ResultTalent.Strength + RandomBonus, 1.0f, 100.0f);
    }
    
    if (SpecialtyBonus.Toughness > 0.0f)
    {
        float AdjustedBonus = SpecialtyBonus.Toughness * SpecialtyBonusRangeMultiplier;
        float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
        ResultTalent.Toughness = FMath::Clamp(ResultTalent.Toughness + RandomBonus, 1.0f, 100.0f);
    }
    
    if (SpecialtyBonus.Intelligence > 0.0f)
    {
        float AdjustedBonus = SpecialtyBonus.Intelligence * SpecialtyBonusRangeMultiplier;
        float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
        ResultTalent.Intelligence = FMath::Clamp(ResultTalent.Intelligence + RandomBonus, 1.0f, 100.0f);
    }
    
    if (SpecialtyBonus.Dexterity > 0.0f)
    {
        float AdjustedBonus = SpecialtyBonus.Dexterity * SpecialtyBonusRangeMultiplier;
        float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
        ResultTalent.Dexterity = FMath::Clamp(ResultTalent.Dexterity + RandomBonus, 1.0f, 100.0f);
    }
    
    if (SpecialtyBonus.Agility > 0.0f)
    {
        float AdjustedBonus = SpecialtyBonus.Agility * SpecialtyBonusRangeMultiplier;
        float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
        ResultTalent.Agility = FMath::Clamp(ResultTalent.Agility + RandomBonus, 1.0f, 100.0f);
    }
    
    if (SpecialtyBonus.Willpower > 0.0f)
    {
        float AdjustedBonus = SpecialtyBonus.Willpower * SpecialtyBonusRangeMultiplier;
        float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
        ResultTalent.Willpower = FMath::Clamp(ResultTalent.Willpower + RandomBonus, 1.0f, 100.0f);
    }
    
    // スキルボーナスを加算
    for (const FSkillTalent& SkillBonus : SpecialtyBonus.SkillBonuses)
    {
        bool bSkillFound = false;
        
        // 既存スキルに係数ベースボーナス加算
        for (FSkillTalent& ExistingSkill : ResultTalent.Skills)
        {
            if (ExistingSkill.SkillType == SkillBonus.SkillType)
            {
                float AdjustedBonus = SkillBonus.Value * SpecialtyBonusRangeMultiplier;
                float RandomBonus = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
                ExistingSkill.Value = FMath::Clamp(ExistingSkill.Value + RandomBonus, 1.0f, 100.0f);
                bSkillFound = true;
                break;
            }
        }
        
        // 新しいスキルとして係数ベース追加
        if (!bSkillFound)
        {
            FSkillTalent NewSkill;
            NewSkill.SkillType = SkillBonus.SkillType;
            float AdjustedBonus = SkillBonus.Value * SpecialtyBonusRangeMultiplier;
            NewSkill.Value = FMath::RandRange(AdjustedBonus * 0.5f, AdjustedBonus);
            ResultTalent.Skills.Add(NewSkill);
        }
    }
    
    return ResultTalent;
}

void UCharacterTalentGenerator::SetBalanceMultipliers(float BaseStatMultiplier, float SpecialtyBonusMultiplier)
{
    BaseStatRangeMultiplier = FMath::Clamp(BaseStatMultiplier, 0.1f, 2.0f);
    SpecialtyBonusRangeMultiplier = FMath::Clamp(SpecialtyBonusMultiplier, 0.1f, 2.0f);
    
    UE_LOG(LogTemp, Log, TEXT("Balance Multipliers Updated: BaseStat=%.3f, SpecialtyBonus=%.3f"), 
           BaseStatRangeMultiplier, SpecialtyBonusRangeMultiplier);
}

FSpecialtyBonus UCharacterTalentGenerator::GetSpecialtyBonus(ESpecialtyType SpecialtyType)
{
    FSpecialtyBonus Bonus;
    
    switch (SpecialtyType)
    {
        case ESpecialtyType::Kendo:
        {
            Bonus.Strength = 10.0f;
            Bonus.Toughness = 10.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Agility = 15.0f;
            Bonus.Willpower = 20.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::TwoHandedWeapons;
            SkillBonus2.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Parry;
            SkillBonus3.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Baseball:
        {
            Bonus.Strength = 15.0f;
            Bonus.Toughness = 15.0f;
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 15.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Throwing;
            SkillBonus2.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Evasion;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            FSkillTalent SkillBonus4;
            SkillBonus4.SkillType = ESkillType::Combat;
            SkillBonus4.Value = 5.0f; // 推定ボーナス（シミュレーター対応）
            Bonus.SkillBonuses.Add(SkillBonus4);
            break;
        }
            
        case ESpecialtyType::Chemistry:
        {
            Bonus.Intelligence = 20.0f;
            Bonus.Dexterity = 20.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Chemistry;
            SkillBonus1.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Medicine;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Engineering;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Archery:
        {
            Bonus.Strength = 5.0f;
            Bonus.Intelligence = 10.0f;
            Bonus.Dexterity = 25.0f;
            Bonus.Agility = 10.0f;
            Bonus.Willpower = 20.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Archery;
            SkillBonus1.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Firearms;
            SkillBonus2.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Survival;
            SkillBonus3.Value = 5.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Karate:
        {
            Bonus.Strength = 15.0f;
            Bonus.Toughness = 10.0f;
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Agility = 25.0f;
            Bonus.Willpower = 20.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Shield;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Combat;
            SkillBonus3.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            FSkillTalent SkillBonus4;
            SkillBonus4.SkillType = ESkillType::Evasion;
            SkillBonus4.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus4);
            break;
        }
            
        case ESpecialtyType::AmericanFootball:
        {
            Bonus.Strength = 30.0f;
            Bonus.Toughness = 30.0f;
            Bonus.Agility = 10.0f;
            Bonus.Willpower = 15.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Shield;
            SkillBonus1.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Construction;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Combat;
            SkillBonus3.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Golf:
        {
            Bonus.Strength = 10.0f;
            Bonus.Toughness = 5.0f;
            Bonus.Intelligence = 15.0f;
            Bonus.Dexterity = 20.0f;
            Bonus.Willpower = 20.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::PolearmWeapons;
            SkillBonus2.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            break;
        }
            
        case ESpecialtyType::TrackAndField:
        {
            Bonus.Strength = 10.0f;
            Bonus.Toughness = 15.0f;
            Bonus.Dexterity = 5.0f;
            Bonus.Agility = 30.0f;
            Bonus.Willpower = 15.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Swimming;
            SkillBonus1.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Survival;
            SkillBonus2.Value = 5.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Evasion;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Drama:
        {
            Bonus.Intelligence = 10.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 10.0f;
            Bonus.Willpower = 15.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Music;
            SkillBonus1.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Tailoring;
            SkillBonus2.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Cooking;
            SkillBonus3.Value = 5.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            FSkillTalent SkillBonus4;
            SkillBonus4.SkillType = ESkillType::Evasion;
            SkillBonus4.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus4);
            break;
        }
            
        case ESpecialtyType::TeaCeremony:
        {
            Bonus.Intelligence = 15.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Willpower = 30.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Cooking;
            SkillBonus1.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Music;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Agriculture;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Equestrian:
        {
            Bonus.Toughness = 10.0f;
            Bonus.Intelligence = 10.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Agility = 15.0f;
            Bonus.Willpower = 15.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::AnimalHandling;
            SkillBonus1.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus2.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Archery;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Robotics:
        {
            Bonus.Intelligence = 25.0f;
            Bonus.Dexterity = 25.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Engineering;
            SkillBonus1.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Mechanics;
            SkillBonus2.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Crafting;
            SkillBonus3.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Gardening:
        {
            Bonus.Strength = 5.0f;
            Bonus.Toughness = 10.0f;
            Bonus.Intelligence = 10.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Agriculture;
            SkillBonus1.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Cooking;
            SkillBonus2.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Survival;
            SkillBonus3.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Astronomy:
        {
            Bonus.Intelligence = 20.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Willpower = 20.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Engineering;
            SkillBonus1.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Survival;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            break;
        }
            
        case ESpecialtyType::TableTennis:
        {
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 10.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Shield;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Evasion;
            SkillBonus3.Value = 5.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Basketball:
        {
            Bonus.Strength = 15.0f;
            Bonus.Toughness = 15.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 20.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Throwing;
            SkillBonus1.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Evasion;
            SkillBonus2.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            break;
        }
            
        case ESpecialtyType::Badminton:
        {
            Bonus.Toughness = 5.0f;
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 20.0f;
            Bonus.Willpower = 5.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Evasion;
            SkillBonus2.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            break;
        }
            
        case ESpecialtyType::Tennis:
        {
            Bonus.Strength = 5.0f;
            Bonus.Toughness = 10.0f;
            Bonus.Intelligence = 10.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 15.0f;
            Bonus.Willpower = 15.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus1.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Evasion;
            SkillBonus2.Value = 15.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            break;
        }
            
        case ESpecialtyType::Volleyball:
        {
            Bonus.Strength = 15.0f;
            Bonus.Toughness = 10.0f;
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Agility = 15.0f;
            Bonus.Willpower = 10.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Throwing;
            SkillBonus1.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            break;
        }
            
        case ESpecialtyType::Soccer:
        {
            Bonus.Strength = 10.0f;
            Bonus.Toughness = 15.0f;
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Agility = 20.0f;
            Bonus.Willpower = 15.0f;
            break;
        }
            
        case ESpecialtyType::Sumo:
        {
            Bonus.Strength = 30.0f;
            Bonus.Toughness = 30.0f;
            Bonus.Willpower = 15.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Shield;
            SkillBonus1.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Cooking;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Combat;
            SkillBonus3.Value = 20.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Cooking:
        {
            Bonus.Intelligence = 10.0f;
            Bonus.Dexterity = 20.0f;
            Bonus.Willpower = 5.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Cooking;
            SkillBonus1.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Chemistry;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Agriculture;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            FSkillTalent SkillBonus4;
            SkillBonus4.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus4.Value = 5.0f;
            Bonus.SkillBonuses.Add(SkillBonus4);
            break;
        }
            
        case ESpecialtyType::Medical:
        {
            Bonus.Intelligence = 30.0f;
            Bonus.Dexterity = 25.0f;
            Bonus.Willpower = 20.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Medicine;
            SkillBonus1.Value = 30.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Chemistry;
            SkillBonus2.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::OneHandedWeapons;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        case ESpecialtyType::Nursing:
        {
            Bonus.Strength = 5.0f;
            Bonus.Toughness = 20.0f;
            Bonus.Intelligence = 20.0f;
            Bonus.Dexterity = 15.0f;
            Bonus.Agility = 10.0f;
            Bonus.Willpower = 25.0f;
            FSkillTalent SkillBonus1;
            SkillBonus1.SkillType = ESkillType::Medicine;
            SkillBonus1.Value = 25.0f;
            Bonus.SkillBonuses.Add(SkillBonus1);
            FSkillTalent SkillBonus2;
            SkillBonus2.SkillType = ESkillType::Cooking;
            SkillBonus2.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus2);
            FSkillTalent SkillBonus3;
            SkillBonus3.SkillType = ESkillType::Survival;
            SkillBonus3.Value = 10.0f;
            Bonus.SkillBonuses.Add(SkillBonus3);
            break;
        }
            
        default:
            break;
    }
    
    return Bonus;
}

