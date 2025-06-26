#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "C_CharacterCard.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_CharacterCard : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_CharacterCard(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Text Display References
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* CharacterNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* SpecialtyText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* DPSText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* TotalDefensePowerText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|Text")
    UTextBlock* HealthText;

    // Progress Bar References
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|ProgressBar")
    UProgressBar* HealthProgressBar;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|ProgressBar")
    UProgressBar* StaminaProgressBar;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI|ProgressBar")
    UProgressBar* MentalProgressBar;

    // Character Reference
    UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (ExposeOnSpawn = "true"))
    AC_IdleCharacter* Character;

public:
    // Initialization - called once when card is created
    UFUNCTION(BlueprintCallable, Category = "Character Card")
    void InitializeWithCharacter(AC_IdleCharacter* InCharacter);

    // Update display with current character data
    UFUNCTION(BlueprintCallable, Category = "Character Card")
    void UpdateDisplay();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Card")
    AC_IdleCharacter* GetCharacter() const { return Character; }

    // Check if this card represents the given character
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Card")
    bool IsCharacterCard(AC_IdleCharacter* TestCharacter) const { return Character == TestCharacter; }

protected:
    // Internal Update Functions
    void UpdateCharacterName();
    void UpdateSpecialty();
    void UpdateCombatStats();
    void UpdateHealthDisplay();
    void UpdateProgressBars();

    // Helper Functions
    UFUNCTION(BlueprintCallable, Category = "Character Card")
    FString GetSpecialtyDisplayName(ESpecialtyType SpecialtyType);

private:
    // Internal state tracking
    bool bIsInitialized;

    // Event binding
    UFUNCTION()
    void OnCharacterStatusChanged(const FCharacterStatus& NewStatus);

    UFUNCTION()
    void OnCharacterDataChanged(AC_IdleCharacter* ChangedCharacter);

    // Bind/unbind character events
    void BindCharacterEvents();
    void UnbindCharacterEvents();
};