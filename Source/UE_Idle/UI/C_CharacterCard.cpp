#include "C_CharacterCard.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "../Types/CharacterTypes.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "../Components/TeamComponent.h"

UC_CharacterCard::UC_CharacterCard(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Character(nullptr)
    , bIsInitialized(false)
{
}

void UC_CharacterCard::NativeConstruct()
{
    Super::NativeConstruct();
    bIsInitialized = true;
    
    // If character was set via ExposeOnSpawn, initialize automatically
    if (Character)
    {
        BindCharacterEvents();
        UpdateDisplay();
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard: Auto-initialized with character: %s"), 
               *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
}

void UC_CharacterCard::NativeDestruct()
{
    // Unbind character events before destruction
    UnbindCharacterEvents();
    Super::NativeDestruct();
}

void UC_CharacterCard::InitializeWithCharacter(AC_IdleCharacter* InCharacter)
{
    if (!InCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_CharacterCard::InitializeWithCharacter - Character is null!"));
        return;
    }

    if (Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::InitializeWithCharacter - Card already has character! This should only be called once."));
        return;
    }

    Character = InCharacter;
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::InitializeWithCharacter - Initialized card for character: %s"), 
           *IIdleCharacterInterface::Execute_GetCharacterName(Character));

    // Bind to character events
    BindCharacterEvents();

    // Update display if widget is already constructed
    if (bIsInitialized)
    {
        UpdateDisplay();
    }
}

void UC_CharacterCard::UpdateDisplay()
{
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_CharacterCard::UpdateDisplay - Character is null!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::UpdateDisplay - Updating display for character: %s"), 
           *IIdleCharacterInterface::Execute_GetCharacterName(Character));

    UpdateCharacterName();
    UpdateSpecialty();
    UpdateCombatStats();
    UpdateHealthDisplay();
    UpdateProgressBars();
}

void UC_CharacterCard::UpdateCharacterName()
{
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateCharacterName: Character is null"));
        return;
    }
    
    FString CharacterName = IIdleCharacterInterface::Execute_GetCharacterName(Character);
    UE_LOG(LogTemp, Warning, TEXT("UpdateCharacterName: Character name is '%s'"), *CharacterName);
    
    if (CharacterNameText)
    {
        CharacterNameText->SetText(FText::FromString(CharacterName));
        UE_LOG(LogTemp, Warning, TEXT("UpdateCharacterName: Successfully set text to CharacterNameText"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateCharacterName: CharacterNameText widget is NULL! Check Blueprint binding."));
    }
}

void UC_CharacterCard::UpdateSpecialty()
{
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateSpecialty: Character is null"));
        return;
    }
    
    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (!StatusComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateSpecialty: StatusComponent is null"));
        return;
    }
    
    ESpecialtyType Specialty = StatusComponent->GetSpecialtyType();
    FString SpecialtyDisplayName = GetSpecialtyDisplayName(Specialty);
    UE_LOG(LogTemp, Warning, TEXT("UpdateSpecialty: Specialty is '%s'"), *SpecialtyDisplayName);
    
    if (SpecialtyText)
    {
        SpecialtyText->SetText(FText::FromString(SpecialtyDisplayName));
        UE_LOG(LogTemp, Warning, TEXT("UpdateSpecialty: Successfully set text to SpecialtyText"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateSpecialty: SpecialtyText widget is NULL! Check Blueprint binding."));
    }
}

void UC_CharacterCard::UpdateCombatStats()
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

    // Update DPS
    if (DPSText)
    {
        FString DPSString = FString::Printf(TEXT("%.0f"), DerivedStats.DPS);
        DPSText->SetText(FText::FromString(DPSString));
    }

    // Update Total Defense Power
    if (TotalDefensePowerText)
    {
        FString DefenseString = FString::Printf(TEXT("%.0f"), DerivedStats.TotalDefensePower);
        TotalDefensePowerText->SetText(FText::FromString(DefenseString));
    }
}

void UC_CharacterCard::UpdateHealthDisplay()
{
    if (!Character || !HealthText)
    {
        return;
    }

    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (StatusComponent)
    {
        FCharacterStatus Status = StatusComponent->GetStatus();
        FString HealthString = FString::Printf(TEXT("%.0f"), Status.CurrentHealth);
        HealthText->SetText(FText::FromString(HealthString));
    }
}

void UC_CharacterCard::UpdateProgressBars()
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

    // Update Health Progress Bar
    if (HealthProgressBar)
    {
        float HealthPercent = (Status.MaxHealth > 0) ? (Status.CurrentHealth / Status.MaxHealth) : 0.0f;
        HealthProgressBar->SetPercent(HealthPercent);
    }

    // Update Stamina Progress Bar
    if (StaminaProgressBar)
    {
        float StaminaPercent = (Status.MaxStamina > 0) ? (Status.CurrentStamina / Status.MaxStamina) : 0.0f;
        StaminaProgressBar->SetPercent(StaminaPercent);
    }

    // Update Mental Progress Bar
    if (MentalProgressBar)
    {
        float MentalPercent = (Status.MaxMental > 0) ? (Status.CurrentMental / Status.MaxMental) : 0.0f;
        MentalProgressBar->SetPercent(MentalPercent);
    }
}

FString UC_CharacterCard::GetSpecialtyDisplayName(ESpecialtyType SpecialtyType)
{
    // Convert enum to display string using UEnum
    const UEnum* SpecialtyEnum = StaticEnum<ESpecialtyType>();
    if (SpecialtyEnum)
    {
        FString EnumString = SpecialtyEnum->GetDisplayNameTextByValue((int64)SpecialtyType).ToString();
        return EnumString;
    }
    
    return TEXT("Unknown");
}

void UC_CharacterCard::OnCharacterStatusChanged(const FCharacterStatus& NewStatus)
{
    // Update display when character status changes
    if (bIsInitialized)
    {
        UpdateDisplay();
    }
}

void UC_CharacterCard::OnCharacterDataChanged(AC_IdleCharacter* ChangedCharacter)
{
    // Update display only if this is our character
    if (bIsInitialized && ChangedCharacter == Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::OnCharacterDataChanged - Updating card for: %s"), 
               *IIdleCharacterInterface::Execute_GetCharacterName(Character));
        UpdateDisplay();
    }
}

void UC_CharacterCard::BindCharacterEvents()
{
    if (!Character)
    {
        return;
    }

    // Bind to character status changes
    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (StatusComponent)
    {
        // Check if already bound to avoid duplicate binding
        if (!StatusComponent->OnStatusChanged.IsAlreadyBound(this, &UC_CharacterCard::OnCharacterStatusChanged))
        {
            StatusComponent->OnStatusChanged.AddDynamic(this, &UC_CharacterCard::OnCharacterStatusChanged);
            UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::BindCharacterEvents - Bound to character status events"));
        }
    }

    // Bind to team data changes
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (UTeamComponent* TeamComp = PC->FindComponentByClass<UTeamComponent>())
            {
                // Check if already bound to avoid duplicate binding
                if (!TeamComp->OnCharacterDataChanged.IsAlreadyBound(this, &UC_CharacterCard::OnCharacterDataChanged))
                {
                    TeamComp->OnCharacterDataChanged.AddDynamic(this, &UC_CharacterCard::OnCharacterDataChanged);
                    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::BindCharacterEvents - Bound to team data events"));
                }
            }
        }
    }
}

void UC_CharacterCard::UnbindCharacterEvents()
{
    if (!Character)
    {
        return;
    }

    // Unbind character status events
    UCharacterStatusComponent* StatusComponent = Character->GetStatusComponent();
    if (StatusComponent)
    {
        StatusComponent->OnStatusChanged.RemoveDynamic(this, &UC_CharacterCard::OnCharacterStatusChanged);
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::UnbindCharacterEvents - Unbound from character status events"));
    }

    // Unbind team data events
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (UTeamComponent* TeamComp = PC->FindComponentByClass<UTeamComponent>())
            {
                TeamComp->OnCharacterDataChanged.RemoveDynamic(this, &UC_CharacterCard::OnCharacterDataChanged);
                UE_LOG(LogTemp, Warning, TEXT("UC_CharacterCard::UnbindCharacterEvents - Unbound from team data events"));
            }
        }
    }
}
