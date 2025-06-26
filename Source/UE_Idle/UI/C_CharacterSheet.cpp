#include "C_CharacterSheet.h"
#include "Components/TextBlock.h"
#include "../Types/CharacterTypes.h"
#include "../Interfaces/IdleCharacterInterface.h"

UC_CharacterSheet::UC_CharacterSheet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Character(nullptr)
    , bIsInitialized(false)
{
}

void UC_CharacterSheet::NativeConstruct()
{
    Super::NativeConstruct();
    bIsInitialized = true;
    
    // If character was set via ExposeOnSpawn, initialize automatically
    if (Character)
    {
        // Bind to character events
        UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
        if (StatusComponent)
        {
            // Check if already bound to avoid duplicate binding
            if (!StatusComponent->OnStatusChanged.IsAlreadyBound(this, &UC_CharacterSheet::OnCharacterStatusChanged))
            {
                StatusComponent->OnStatusChanged.AddDynamic(this, &UC_CharacterSheet::OnCharacterStatusChanged);
            }
        }
        
        UpdateAllStats();
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterSheet: Auto-initialized with character: %s"), 
               *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
}

void UC_CharacterSheet::InitializeWithCharacter(AC_IdleCharacter* InCharacter)
{
    if (!InCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_CharacterSheet::InitializeWithCharacter - Character is null!"));
        return;
    }

    if (Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterSheet::InitializeWithCharacter - Sheet already has character! This should only be called once."));
        return;
    }

    Character = InCharacter;
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterSheet::InitializeWithCharacter - Initialized sheet for character: %s"), 
           *IIdleCharacterInterface::Execute_GetCharacterName(Character));

    // Bind to character events
    if (Character)
    {
        UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
        if (StatusComponent)
        {
            // Check if already bound to avoid duplicate binding
            if (!StatusComponent->OnStatusChanged.IsAlreadyBound(this, &UC_CharacterSheet::OnCharacterStatusChanged))
            {
                StatusComponent->OnStatusChanged.AddDynamic(this, &UC_CharacterSheet::OnCharacterStatusChanged);
            }
        }
    }

    // Update display if widget is already constructed
    if (bIsInitialized)
    {
        UpdateAllStats();
    }
}

void UC_CharacterSheet::SetCharacter(AC_IdleCharacter* NewCharacter)
{
    // Unbind from previous character if exists
    if (Character)
    {
        UCharacterStatusComponent* PrevStatusComponent = Character->FindComponentByClass<UCharacterStatusComponent>();
        if (PrevStatusComponent)
        {
            PrevStatusComponent->OnStatusChanged.RemoveDynamic(this, &UC_CharacterSheet::OnCharacterStatusChanged);
        }
    }

    Character = NewCharacter;

    // Bind to new character events
    if (Character)
    {
        UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
        if (StatusComponent)
        {
            StatusComponent->OnStatusChanged.AddDynamic(this, &UC_CharacterSheet::OnCharacterStatusChanged);
        }
    }

    // Update display if widget is initialized
    if (bIsInitialized)
    {
        UpdateAllStats();
    }
}

void UC_CharacterSheet::UpdateAllStats()
{
    if (!Character)
    {
        return;
    }

    UpdateBasicInfo();
    UpdateAttributes();
    UpdateStatus();
    UpdateCombatStats();
    UpdateWorkStats();
    UpdateTraitsAndSkills();
}

void UC_CharacterSheet::UpdateBasicInfo()
{
    if (!Character)
    {
        return;
    }

    // Character Name
    if (CharacterNameText)
    {
        FString CharacterName = IIdleCharacterInterface::Execute_GetCharacterName(Character);
        CharacterNameText->SetText(FText::FromString(CharacterName));
    }

    // Specialty
    if (SpecialtyText)
    {
        UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
        if (StatusComponent)
        {
            ESpecialtyType Specialty = StatusComponent->GetSpecialtyType();
            FString SpecialtyDisplayName = GetSpecialtyDisplayName(Specialty);
            SpecialtyText->SetText(FText::FromString(SpecialtyDisplayName));
        }
    }
}

void UC_CharacterSheet::UpdateAttributes()
{
    if (!Character)
    {
        return;
    }

    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (!StatusComponent)
    {
        return;
    }

    FCharacterTalent Talent = StatusComponent->GetTalent();

    // Update attribute text blocks
    if (StrengthText)
    {
        StrengthText->SetText(FText::FromString(FString::Printf(TEXT("筋力: %.1f"), Talent.Strength)));
    }
    if (ToughnessText)
    {
        ToughnessText->SetText(FText::FromString(FString::Printf(TEXT("耐久: %.1f"), Talent.Toughness)));
    }
    if (IntelligenceText)
    {
        IntelligenceText->SetText(FText::FromString(FString::Printf(TEXT("知力: %.1f"), Talent.Intelligence)));
    }
    if (DexterityText)
    {
        DexterityText->SetText(FText::FromString(FString::Printf(TEXT("器用: %.1f"), Talent.Dexterity)));
    }
    if (AgilityText)
    {
        AgilityText->SetText(FText::FromString(FString::Printf(TEXT("敏捷: %.1f"), Talent.Agility)));
    }
    if (WillpowerText)
    {
        WillpowerText->SetText(FText::FromString(FString::Printf(TEXT("意志: %.1f"), Talent.Willpower)));
    }
}

void UC_CharacterSheet::UpdateStatus()
{
    if (!Character)
    {
        return;
    }

    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (!StatusComponent)
    {
        return;
    }

    FCharacterStatus Status = StatusComponent->GetStatus();

    // Update status text blocks
    if (HealthText)
    {
        FString HealthString = FString::Printf(TEXT("体力: %.0f / %.0f"), Status.CurrentHealth, Status.MaxHealth);
        HealthText->SetText(FText::FromString(HealthString));
    }
    if (StaminaText)
    {
        FString StaminaString = FString::Printf(TEXT("スタミナ: %.0f / %.0f"), Status.CurrentStamina, Status.MaxStamina);
        StaminaText->SetText(FText::FromString(StaminaString));
    }
    if (MentalText)
    {
        FString MentalString = FString::Printf(TEXT("メンタル: %.0f / %.0f"), Status.CurrentMental, Status.MaxMental);
        MentalText->SetText(FText::FromString(MentalString));
    }
    if (CarryingCapacityText)
    {
        FString CapacityString = FString::Printf(TEXT("積載量: %.1f kg"), Status.CarryingCapacity);
        CarryingCapacityText->SetText(FText::FromString(CapacityString));
    }
}

void UC_CharacterSheet::UpdateCombatStats()
{
    if (!Character)
    {
        return;
    }

    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (!StatusComponent)
    {
        return;
    }

    FDerivedStats DerivedStats = StatusComponent->GetDerivedStats();

    // Update combat stats
    if (AttackSpeedText)
    {
        AttackSpeedText->SetText(FText::FromString(FString::Printf(TEXT("攻撃速度: %.2f"), DerivedStats.AttackSpeed)));
    }
    if (HitChanceText)
    {
        HitChanceText->SetText(FText::FromString(FString::Printf(TEXT("命中: %.1f%%"), DerivedStats.HitChance)));
    }
    if (CriticalChanceText)
    {
        CriticalChanceText->SetText(FText::FromString(FString::Printf(TEXT("クリティカル率: %.1f%%"), DerivedStats.CriticalChance)));
    }
    if (BaseDamageText)
    {
        BaseDamageText->SetText(FText::FromString(FString::Printf(TEXT("基本ダメージ: %d"), DerivedStats.BaseDamage)));
    }
    if (DodgeChanceText)
    {
        DodgeChanceText->SetText(FText::FromString(FString::Printf(TEXT("回避: %.1f%%"), DerivedStats.DodgeChance)));
    }
    if (ParryChanceText)
    {
        ParryChanceText->SetText(FText::FromString(FString::Printf(TEXT("受け流し率: %.1f%%"), DerivedStats.ParryChance)));
    }
    if (ShieldChanceText)
    {
        ShieldChanceText->SetText(FText::FromString(FString::Printf(TEXT("盾防御率: %.1f%%"), DerivedStats.ShieldChance)));
    }
    if (DefenseValueText)
    {
        DefenseValueText->SetText(FText::FromString(FString::Printf(TEXT("防御値: %d"), DerivedStats.DefenseValue)));
    }
    if (DPSText)
    {
        DPSText->SetText(FText::FromString(FString::Printf(TEXT("DPS: %.1f"), DerivedStats.DPS)));
    }
    if (TotalDefensePowerText)
    {
        TotalDefensePowerText->SetText(FText::FromString(FString::Printf(TEXT("総合防御力: %.1f"), DerivedStats.TotalDefensePower)));
    }
    if (CombatPowerText)
    {
        CombatPowerText->SetText(FText::FromString(FString::Printf(TEXT("戦闘力: %.1f"), DerivedStats.CombatPower)));
    }
}

void UC_CharacterSheet::UpdateWorkStats()
{
    if (!Character)
    {
        return;
    }

    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (!StatusComponent)
    {
        return;
    }

    FDerivedStats DerivedStats = StatusComponent->GetDerivedStats();

    // Update work stats
    if (ConstructionPowerText)
    {
        ConstructionPowerText->SetText(FText::FromString(FString::Printf(TEXT("建築: %.1f"), DerivedStats.ConstructionPower)));
    }
    if (ProductionPowerText)
    {
        ProductionPowerText->SetText(FText::FromString(FString::Printf(TEXT("生産: %.1f"), DerivedStats.ProductionPower)));
    }
    if (GatheringPowerText)
    {
        GatheringPowerText->SetText(FText::FromString(FString::Printf(TEXT("採集: %.1f"), DerivedStats.GatheringPower)));
    }
    if (CookingPowerText)
    {
        CookingPowerText->SetText(FText::FromString(FString::Printf(TEXT("料理: %.1f"), DerivedStats.CookingPower)));
    }
    if (CraftingPowerText)
    {
        CraftingPowerText->SetText(FText::FromString(FString::Printf(TEXT("工作: %.1f"), DerivedStats.CraftingPower)));
    }
    if (WorkPowerText)
    {
        WorkPowerText->SetText(FText::FromString(FString::Printf(TEXT("総合作業力: %.1f"), DerivedStats.WorkPower)));
    }
}

void UC_CharacterSheet::UpdateTraitsAndSkills()
{
    if (!Character)
    {
        return;
    }

    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (!StatusComponent)
    {
        return;
    }

    FCharacterTalent Talent = StatusComponent->GetTalent();

    // Update traits
    if (CharacterTraitsText)
    {
        FString TraitsString = GetTraitsDisplayString(Talent.CharacterTraits);
        CharacterTraitsText->SetText(FText::FromString(TraitsString));
    }

    // Update skills
    if (SkillsText)
    {
        FString SkillsString = GetSkillsDisplayString(Talent.Skills);
        SkillsText->SetText(FText::FromString(SkillsString));
    }
}

FString UC_CharacterSheet::GetSpecialtyDisplayName(ESpecialtyType SpecialtyType)
{
    const UEnum* SpecialtyEnum = StaticEnum<ESpecialtyType>();
    if (SpecialtyEnum)
    {
        FString EnumString = SpecialtyEnum->GetDisplayNameTextByValue((int64)SpecialtyType).ToString();
        return EnumString;
    }
    return TEXT("Unknown");
}

FString UC_CharacterSheet::GetTraitsDisplayString(const TArray<ECharacterTrait>& Traits)
{
    if (Traits.Num() == 0)
    {
        return TEXT("なし");
    }

    TArray<FString> TraitNames;
    for (const ECharacterTrait& Trait : Traits)
    {
        if (Trait != ECharacterTrait::None)
        {
            TraitNames.Add(GetTraitDisplayName(Trait));
        }
    }

    if (TraitNames.Num() == 0)
    {
        return TEXT("なし");
    }

    return FString::Join(TraitNames, TEXT(", "));
}

FString UC_CharacterSheet::GetSkillsDisplayString(const TArray<FSkillTalent>& Skills)
{
    if (Skills.Num() == 0)
    {
        return TEXT("なし");
    }

    TArray<FString> SkillStrings;
    for (const FSkillTalent& Skill : Skills)
    {
        FString SkillDisplayName = GetSkillTypeDisplayName(Skill.SkillType);
        FString SkillString = FString::Printf(TEXT("%s (%.1f)"), *SkillDisplayName, Skill.Value);
        SkillStrings.Add(SkillString);
    }

    return FString::Join(SkillStrings, TEXT(", "));
}

FString UC_CharacterSheet::GetTraitDisplayName(ECharacterTrait Trait)
{
    const UEnum* TraitEnum = StaticEnum<ECharacterTrait>();
    if (TraitEnum)
    {
        FString EnumString = TraitEnum->GetDisplayNameTextByValue((int64)Trait).ToString();
        return EnumString;
    }
    return TEXT("Unknown");
}

FString UC_CharacterSheet::GetSkillTypeDisplayName(ESkillType SkillType)
{
    const UEnum* SkillEnum = StaticEnum<ESkillType>();
    if (SkillEnum)
    {
        FString EnumString = SkillEnum->GetDisplayNameTextByValue((int64)SkillType).ToString();
        return EnumString;
    }
    return TEXT("Unknown");
}

void UC_CharacterSheet::OnCharacterStatusChanged(const FCharacterStatus& NewStatus)
{
    // Update display when character status changes
    if (bIsInitialized)
    {
        UpdateAllStats();
    }
}