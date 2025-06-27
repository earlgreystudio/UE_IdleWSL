#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Types/CharacterTypes.h"
#include "C_CharacterSheet.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_CharacterSheet : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_CharacterSheet(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;

    // Basic Info
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Basic")
    UTextBlock* CharacterNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Basic")
    UTextBlock* SpecialtyText;

    // Base Attributes
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Attributes")
    UTextBlock* StrengthText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Attributes")
    UTextBlock* ToughnessText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Attributes")
    UTextBlock* IntelligenceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Attributes")
    UTextBlock* DexterityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Attributes")
    UTextBlock* AgilityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Attributes")
    UTextBlock* WillpowerText;

    // Status
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Status")
    UTextBlock* HealthText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Status")
    UTextBlock* StaminaText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Status")
    UTextBlock* MentalText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Status")
    UTextBlock* CarryingCapacityText;

    // Combat Stats
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* AttackSpeedText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* HitChanceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* CriticalChanceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* BaseDamageText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* DodgeChanceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* ParryChanceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* ShieldChanceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* DefenseValueText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* DPSText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* TotalDefensePowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Combat")
    UTextBlock* CombatPowerText;

    // Work Stats
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Work")
    UTextBlock* ConstructionPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Work")
    UTextBlock* ProductionPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Work")
    UTextBlock* GatheringPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Work")
    UTextBlock* CookingPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Work")
    UTextBlock* CraftingPowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Work")
    UTextBlock* WorkPowerText;

    // Traits and Skills
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Traits")
    UTextBlock* CharacterTraitsText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Skills")
    UTextBlock* SkillsText;

    // Character Reference
    UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (ExposeOnSpawn = "true"))
    AC_IdleCharacter* Character;

public:
    // Initialization - called once when sheet is created
    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void InitializeWithCharacter(AC_IdleCharacter* InCharacter);

    // Legacy function for compatibility
    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void SetCharacter(AC_IdleCharacter* NewCharacter);

    // Update display with current character data
    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateAllStats();

protected:
    // Update Functions
    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateBasicInfo();

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateAttributes();

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateStatus();

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateCombatStats();

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateWorkStats();

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    void UpdateTraitsAndSkills();

    // Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    FString GetSpecialtyDisplayName(ESpecialtyType SpecialtyType);

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    FString GetTraitsDisplayString(const TArray<ECharacterTrait>& Traits);

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    FString GetSkillsDisplayString(const TArray<FSkillTalent>& Skills);

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    FString GetTraitDisplayName(ECharacterTrait Trait);

    UFUNCTION(BlueprintCallable, Category = "Character Sheet")
    FString GetSkillTypeDisplayName(ESkillType SkillType);

private:
    // Internal state
    bool bIsInitialized;

    // Event binding
    UFUNCTION()
    void OnCharacterStatusChanged(const FCharacterStatus& NewStatus);
};
