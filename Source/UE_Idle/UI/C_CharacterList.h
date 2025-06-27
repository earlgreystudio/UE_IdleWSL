#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "../Actor/C_IdleCharacter.h"
#include "C_CharacterCard.h"
#include "C_CharacterList.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_CharacterList : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_CharacterList(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // UI References
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    UPanelWidget* CharacterCardPanel;

    // Character Management
    UPROPERTY(BlueprintReadOnly, Category = "Characters")
    TArray<AC_IdleCharacter*> CachedCharacterList;

    UPROPERTY(BlueprintReadOnly, Category = "Characters")
    TArray<UC_CharacterCard*> CharacterCardWidgets;

public:
    // Main Functions
    UFUNCTION(BlueprintCallable, Category = "Character List")
    void RefreshCharacterList();

    UFUNCTION(BlueprintCallable, Category = "Character List")
    void UpdateCharacterCards();

    // Manual refresh for Blueprint use
    UFUNCTION(BlueprintCallable, Category = "Character List")
    void ForceRefresh();

    // Event-based refresh
    UFUNCTION(BlueprintCallable, Category = "Character List")
    void InitializeWithDelay();

    // Start periodic checking
    UFUNCTION(BlueprintCallable, Category = "Character List")
    void StartPeriodicRefresh();

    // Stop periodic checking
    UFUNCTION(BlueprintCallable, Category = "Character List")
    void StopPeriodicRefresh();

protected:
    // Internal Functions
    UFUNCTION()
    void CreateCharacterCard(AC_IdleCharacter* Character);

    UFUNCTION()
    void ClearCharacterCards();

    UFUNCTION()
    void OnCharacterListChanged();

    // TeamComponent event handler
    UFUNCTION()
    void OnTeamCharacterListChanged();

private:
    // Character Card Widget Class
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget Classes", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UC_CharacterCard> CharacterCardWidgetClass;

    // Timer handle for delayed refresh
    FTimerHandle RefreshTimerHandle;
};
