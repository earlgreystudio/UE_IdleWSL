#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CharacterTypes.h"
#include "CharacterPresetTypes.generated.h"

// キャラクタープリセット構造体
USTRUCT(BlueprintType)
struct FCharacterPresetDataRow : public FTableRowBase
{
    GENERATED_BODY()

    // 表示名
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FString Name;

    // 説明
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FString Description;

    // 敵かどうか
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    bool bIsEnemy;

    // 基本能力値 (1-30)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1", ClampMax = "30"))
    float Strength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1", ClampMax = "30"))
    float Toughness;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1", ClampMax = "30"))
    float Intelligence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1", ClampMax = "30"))
    float Dexterity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1", ClampMax = "30"))
    float Agility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1", ClampMax = "30"))
    float Willpower;

    // スキル文字列 (CSV用: "Combat:3|Evasion:3")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    FString SkillsString;

    // 初期装備文字列 (CSV用: "short_sword:1|iron_helmet:1")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FString InitialItemsString;

    // 自然武器ID (素手時に使用される種族固有の武器)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NaturalWeapon")
    FString NaturalWeaponId;

    // 自然武器攻撃力
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NaturalWeapon", meta = (ClampMin = "0", ClampMax = "50"))
    int32 NaturalWeaponPower;

    // 自然武器表示名
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NaturalWeapon")
    FString NaturalWeaponName;

    FCharacterPresetDataRow()
    {
        Name = TEXT("");
        Description = TEXT("");
        bIsEnemy = false;
        Strength = 10.0f;
        Toughness = 10.0f;
        Intelligence = 10.0f;
        Dexterity = 10.0f;
        Agility = 10.0f;
        Willpower = 10.0f;
        SkillsString = TEXT("");
        InitialItemsString = TEXT("");
        NaturalWeaponId = TEXT("");
        NaturalWeaponPower = 2;  // 人間の素手攻撃力
        NaturalWeaponName = TEXT("素手");
    }

    // CSV文字列からFCharacterTalentを生成するヘルパー関数
    FCharacterTalent CreateCharacterTalent() const
    {
        FCharacterTalent Talent;
        
        // 基本能力値設定
        Talent.Strength = Strength;
        Talent.Toughness = Toughness;
        Talent.Intelligence = Intelligence;
        Talent.Dexterity = Dexterity;
        Talent.Agility = Agility;
        Talent.Willpower = Willpower;

        // スキル解析 (例: "Combat:3|Evasion:3")
        if (!SkillsString.IsEmpty())
        {
            TArray<FString> SkillPairs;
            SkillsString.ParseIntoArray(SkillPairs, TEXT("|"), true);

            for (const FString& SkillPair : SkillPairs)
            {
                FString SkillName;
                FString SkillValue;
                if (SkillPair.Split(TEXT(":"), &SkillName, &SkillValue))
                {
                    FSkillTalent Skill;
                    
                    // スキル名からESkillTypeに変換
                    if (SkillName == TEXT("Combat"))
                        Skill.SkillType = ESkillType::Combat;
                    else if (SkillName == TEXT("Evasion"))
                        Skill.SkillType = ESkillType::Evasion;
                    else if (SkillName == TEXT("Swimming"))
                        Skill.SkillType = ESkillType::Swimming;
                    else if (SkillName == TEXT("OneHandedWeapons"))
                        Skill.SkillType = ESkillType::OneHandedWeapons;
                    // 必要に応じて他のスキルタイプも追加
                    
                    Skill.Value = FCString::Atof(*SkillValue);
                    Talent.Skills.Add(Skill);
                }
            }
        }

        return Talent;
    }

    // 初期装備を解析するヘルパー関数
    TMap<FString, int32> ParseInitialItems() const
    {
        TMap<FString, int32> Items;
        
        if (!InitialItemsString.IsEmpty())
        {
            TArray<FString> ItemPairs;
            InitialItemsString.ParseIntoArray(ItemPairs, TEXT("|"), true);

            for (const FString& ItemPair : ItemPairs)
            {
                FString ItemId;
                FString QuantityStr;
                if (ItemPair.Split(TEXT(":"), &ItemId, &QuantityStr))
                {
                    int32 Quantity = FCString::Atoi(*QuantityStr);
                    Items.Add(ItemId, Quantity);
                }
            }
        }

        return Items;
    }
};