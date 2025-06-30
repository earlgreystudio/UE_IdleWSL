#include "TaskTypes.h"

FString UTaskTypeUtils::GetTaskTypeDisplayName(ETaskType TaskType)
{
    switch (TaskType)
    {
        case ETaskType::Idle:
            return TEXT("待機");
        case ETaskType::All:
            return TEXT("全て");
        case ETaskType::Adventure:
            return TEXT("冒険");
        case ETaskType::Cooking:
            return TEXT("料理");
        case ETaskType::Construction:
            return TEXT("建築");
        case ETaskType::Gathering:
            return TEXT("採集");
        case ETaskType::Crafting:
            return TEXT("製作");
        default:
            return TEXT("不明");
    }
}

ETaskType UTaskTypeUtils::GetTaskTypeFromString(const FString& TaskName)
{
    if (TaskName == TEXT("待機"))
        return ETaskType::Idle;
    else if (TaskName == TEXT("全て"))
        return ETaskType::All;
    else if (TaskName == TEXT("冒険"))
        return ETaskType::Adventure;
    else if (TaskName == TEXT("料理"))
        return ETaskType::Cooking;
    else if (TaskName == TEXT("建築"))
        return ETaskType::Construction;
    else if (TaskName == TEXT("採集"))
        return ETaskType::Gathering;
    else if (TaskName == TEXT("製作"))
        return ETaskType::Crafting;
    
    return ETaskType::Idle; // デフォルト
}

TArray<ETaskType> UTaskTypeUtils::GetAllTaskTypes()
{
    return {
        ETaskType::Idle,
        ETaskType::All,
        ETaskType::Adventure,
        ETaskType::Cooking,
        ETaskType::Construction,
        ETaskType::Gathering,
        ETaskType::Crafting
    };
}

bool UTaskTypeUtils::IsValidTaskType(ETaskType TaskType)
{
    switch (TaskType)
    {
        case ETaskType::Idle:
        case ETaskType::All:
        case ETaskType::Adventure:
        case ETaskType::Cooking:
        case ETaskType::Construction:
        case ETaskType::Gathering:
        case ETaskType::Crafting:
            return true;
        default:
            return false;
    }
}

FString UTaskTypeUtils::GetActionStateDisplayName(ETeamActionState ActionState)
{
    switch (ActionState)
    {
        case ETeamActionState::Idle:
            return TEXT("待機");
        case ETeamActionState::Moving:
            return TEXT("移動中");
        case ETeamActionState::Returning:
            return TEXT("帰還中");
        case ETeamActionState::Working:
            return TEXT("作業中");
        case ETeamActionState::InCombat:
            return TEXT("戦闘中");
        case ETeamActionState::Locked:
            return TEXT("アクション中");
        default:
            return TEXT("不明");
    }
}

FString UTaskTypeUtils::GetCombatStateDisplayName(ETeamCombatState CombatState)
{
    switch (CombatState)
    {
        case ETeamCombatState::NotInCombat:
            return TEXT("非戦闘");
        case ETeamCombatState::Starting:
            return TEXT("戦闘開始中");
        case ETeamCombatState::InProgress:
            return TEXT("戦闘中");
        case ETeamCombatState::Ending:
            return TEXT("戦闘終了処理中");
        case ETeamCombatState::Finished:
            return TEXT("戦闘完了");
        default:
            return TEXT("不明");
    }
}

// ======== タスクとスキルの関連管理実装 ========

FTaskRelatedSkills UTaskTypeUtils::GetTaskRelatedSkills(ETaskType TaskType)
{
    FTaskRelatedSkills Skills;
    
    switch (TaskType)
    {
        case ETaskType::Construction:
            Skills.PrimarySkill = TEXT("ConstructionPower");
            Skills.SecondarySkill = TEXT("CraftingPower");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 0.7f;
            Skills.SecondaryWeight = 0.3f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Cooking:
            Skills.PrimarySkill = TEXT("CookingPower");
            Skills.SecondarySkill = TEXT("");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 1.0f;
            Skills.SecondaryWeight = 0.0f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Gathering:
            Skills.PrimarySkill = TEXT("GatheringPower");
            Skills.SecondarySkill = TEXT("");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 1.0f;
            Skills.SecondaryWeight = 0.0f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Crafting:
            Skills.PrimarySkill = TEXT("CraftingPower");
            Skills.SecondarySkill = TEXT("ProductionPower");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 0.6f;
            Skills.SecondaryWeight = 0.4f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Adventure:
            Skills.PrimarySkill = TEXT("CombatPower");
            Skills.SecondarySkill = TEXT("DefenseValue");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 0.7f;
            Skills.SecondaryWeight = 0.3f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Idle:
        case ETaskType::All:
        default:
            // スキル不要
            break;
            
        // 将来実装用タスクタイプのスキル定義
        case ETaskType::Farming:
            Skills.PrimarySkill = TEXT("GatheringPower");  // 農業は採集スキルを暫定使用
            Skills.SecondarySkill = TEXT("");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 1.0f;
            Skills.SecondaryWeight = 0.0f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Mining:
            Skills.PrimarySkill = TEXT("GatheringPower");  // 採掘も採集スキルを暫定使用
            Skills.SecondarySkill = TEXT("ConstructionPower");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 0.7f;
            Skills.SecondaryWeight = 0.3f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Medical:
            Skills.PrimarySkill = TEXT("CraftingPower");  // 治療は工作スキルを暫定使用
            Skills.SecondarySkill = TEXT("");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 1.0f;
            Skills.SecondaryWeight = 0.0f;
            Skills.TertiaryWeight = 0.0f;
            break;
            
        case ETaskType::Taming:
            Skills.PrimarySkill = TEXT("WorkPower");  // 調教は作業力総合値を暫定使用
            Skills.SecondarySkill = TEXT("");
            Skills.TertiarySkill = TEXT("");
            Skills.PrimaryWeight = 1.0f;
            Skills.SecondaryWeight = 0.0f;
            Skills.TertiaryWeight = 0.0f;
            break;
    }
    
    return Skills;
}

TArray<FString> UTaskTypeUtils::GetTaskSkillDisplayNames(ETaskType TaskType)
{
    TArray<FString> DisplayNames;
    FTaskRelatedSkills Skills = GetTaskRelatedSkills(TaskType);
    
    if (!Skills.PrimarySkill.IsEmpty())
    {
        DisplayNames.Add(ConvertSkillNameToPropertyName(Skills.PrimarySkill));
    }
    
    if (!Skills.SecondarySkill.IsEmpty())
    {
        DisplayNames.Add(ConvertSkillNameToPropertyName(Skills.SecondarySkill));
    }
    
    if (!Skills.TertiarySkill.IsEmpty())
    {
        DisplayNames.Add(ConvertSkillNameToPropertyName(Skills.TertiarySkill));
    }
    
    return DisplayNames;
}

FString UTaskTypeUtils::ConvertSkillNameToPropertyName(const FString& SkillName)
{
    // FDerivedStatsのプロパティ名から表示名へのマッピング
    static TMap<FString, FString> SkillDisplayNames = {
        {TEXT("ConstructionPower"), TEXT("建築")},
        {TEXT("ProductionPower"), TEXT("生産")},
        {TEXT("GatheringPower"), TEXT("採集")},
        {TEXT("CookingPower"), TEXT("料理")},
        {TEXT("CraftingPower"), TEXT("工作")},
        {TEXT("CombatPower"), TEXT("戦闘力")},
        {TEXT("AttackSpeed"), TEXT("攻撃速度")},
        {TEXT("DefenseValue"), TEXT("防御値")},
        {TEXT("HitChance"), TEXT("命中率")},
        {TEXT("CriticalChance"), TEXT("クリティカル率")},
        {TEXT("DodgeChance"), TEXT("回避率")}
    };
    
    if (const FString* DisplayName = SkillDisplayNames.Find(SkillName))
    {
        return *DisplayName;
    }
    
    return SkillName; // マッピングがない場合はそのまま返す
}
