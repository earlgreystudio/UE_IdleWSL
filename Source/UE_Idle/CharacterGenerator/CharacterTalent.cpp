#include "CharacterTalent.h"
#include "Engine/Engine.h"

FCharacterTalent UCharacterTalentGenerator::GenerateRandomTalent()
{
    FCharacterTalent Talent;
    
    // 基本能力値をランダムに設定 (1-30)
    Talent.Strength = FMath::RandRange(1.0f, 30.0f);
    Talent.Toughness = FMath::RandRange(1.0f, 30.0f);
    Talent.Intelligence = FMath::RandRange(1.0f, 30.0f);
    Talent.Dexterity = FMath::RandRange(1.0f, 30.0f);
    Talent.Agility = FMath::RandRange(1.0f, 30.0f);
    Talent.Willpower = FMath::RandRange(1.0f, 30.0f);
    
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

FCharacterTalent UCharacterTalentGenerator::ApplyClubBonus(const FCharacterTalent& BaseTalent, EClubType ClubType)
{
    FCharacterTalent ResultTalent = BaseTalent;
    FClubBonus ClubBonus = GetClubBonus(ClubType);
    
    // 基本能力値にランダムボーナスを加算（半分から最大値まで）
    if (ClubBonus.Strength > 0.0f)
    {
        float RandomBonus = FMath::RandRange(ClubBonus.Strength * 0.5f, ClubBonus.Strength);
        ResultTalent.Strength = FMath::Clamp(ResultTalent.Strength + RandomBonus, 1.0f, 100.0f);
    }
    
    if (ClubBonus.Toughness > 0.0f)
    {
        float RandomBonus = FMath::RandRange(ClubBonus.Toughness * 0.5f, ClubBonus.Toughness);
        ResultTalent.Toughness = FMath::Clamp(ResultTalent.Toughness + RandomBonus, 1.0f, 100.0f);
    }
    
    if (ClubBonus.Intelligence > 0.0f)
    {
        float RandomBonus = FMath::RandRange(ClubBonus.Intelligence * 0.5f, ClubBonus.Intelligence);
        ResultTalent.Intelligence = FMath::Clamp(ResultTalent.Intelligence + RandomBonus, 1.0f, 100.0f);
    }
    
    if (ClubBonus.Dexterity > 0.0f)
    {
        float RandomBonus = FMath::RandRange(ClubBonus.Dexterity * 0.5f, ClubBonus.Dexterity);
        ResultTalent.Dexterity = FMath::Clamp(ResultTalent.Dexterity + RandomBonus, 1.0f, 100.0f);
    }
    
    if (ClubBonus.Agility > 0.0f)
    {
        float RandomBonus = FMath::RandRange(ClubBonus.Agility * 0.5f, ClubBonus.Agility);
        ResultTalent.Agility = FMath::Clamp(ResultTalent.Agility + RandomBonus, 1.0f, 100.0f);
    }
    
    if (ClubBonus.Willpower > 0.0f)
    {
        float RandomBonus = FMath::RandRange(ClubBonus.Willpower * 0.5f, ClubBonus.Willpower);
        ResultTalent.Willpower = FMath::Clamp(ResultTalent.Willpower + RandomBonus, 1.0f, 100.0f);
    }
    
    // スキルボーナスを加算
    for (const FSkillTalent& SkillBonus : ClubBonus.SkillBonuses)
    {
        bool bSkillFound = false;
        
        // 既存スキルに加算
        for (FSkillTalent& ExistingSkill : ResultTalent.Skills)
        {
            if (ExistingSkill.SkillType == SkillBonus.SkillType)
            {
                float RandomBonus = FMath::RandRange(SkillBonus.Value * 0.5f, SkillBonus.Value);
                ExistingSkill.Value = FMath::Clamp(ExistingSkill.Value + RandomBonus, 1.0f, 100.0f);
                bSkillFound = true;
                break;
            }
        }
        
        // 新しいスキルとして追加
        if (!bSkillFound)
        {
            FSkillTalent NewSkill;
            NewSkill.SkillType = SkillBonus.SkillType;
            NewSkill.Value = FMath::RandRange(SkillBonus.Value * 0.5f, SkillBonus.Value);
            ResultTalent.Skills.Add(NewSkill);
        }
    }
    
    return ResultTalent;
}

FClubBonus UCharacterTalentGenerator::GetClubBonus(EClubType ClubType)
{
    FClubBonus Bonus;
    
    switch (ClubType)
    {
        case EClubType::Kendo:
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
            
        case EClubType::Baseball:
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
            break;
        }
            
        case EClubType::Chemistry:
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
            
        case EClubType::Archery:
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
            
        case EClubType::Karate:
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
            
        case EClubType::AmericanFootball:
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
            
        case EClubType::Golf:
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
            
        case EClubType::TrackAndField:
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
            
        case EClubType::Drama:
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
            
        case EClubType::TeaCeremony:
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
            
        case EClubType::Equestrian:
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
            
        case EClubType::Robotics:
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
            
        case EClubType::Gardening:
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
            
        case EClubType::Astronomy:
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
            
        case EClubType::TableTennis:
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
            
        case EClubType::Basketball:
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
            
        case EClubType::Badminton:
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
            
        case EClubType::Tennis:
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
            
        case EClubType::Volleyball:
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
            
        case EClubType::Soccer:
        {
            Bonus.Strength = 10.0f;
            Bonus.Toughness = 15.0f;
            Bonus.Intelligence = 5.0f;
            Bonus.Dexterity = 10.0f;
            Bonus.Agility = 20.0f;
            Bonus.Willpower = 15.0f;
            break;
        }
            
        case EClubType::Sumo:
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
            
        case EClubType::Cooking:
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
            
        case EClubType::Medical:
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
            
        case EClubType::Nursing:
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

