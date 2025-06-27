#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Types/TeamTypes.h"
#include "C_PanelInventory.generated.h"

// Forward declarations
class UComboBoxString;
class UButton;
class UHorizontalBox;
class UVerticalBox;
class UTextBlock;
class UC__InventoryList;
class UInventoryComponent;
class UTeamComponent;
class AC_IdleCharacter;

// 選択状態の種類
UENUM(BlueprintType)
enum class EInventoryPanelMode : uint8
{
    Base        UMETA(DisplayName = "拠点"),
    Team        UMETA(DisplayName = "チーム")
};

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_PanelInventory : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_PanelInventory(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // UI References
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    class UScrollBox* TeamSelectionScrollBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    UC__InventoryList* TeamInventoryList;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    class UScrollBox* MemberButtonsScrollBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    UC__InventoryList* MemberInventoryList;

    // Optional UI References  
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
    UTextBlock* UpperInventoryNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
    UTextBlock* LowerInventoryNameText;

    // Component References
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    UTeamComponent* CachedTeamComponent;

    UPROPERTY(BlueprintReadOnly, Category = "Components")
    UInventoryComponent* BaseInventoryComponent;

    // Current Selection State
    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    EInventoryPanelMode CurrentPanelMode = EInventoryPanelMode::Base;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    int32 CurrentTeamIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    AC_IdleCharacter* CurrentSelectedMember = nullptr;

    // Team Selection Buttons
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    TArray<UButton*> TeamButtons;

    // Member Buttons
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    TArray<UButton*> MemberButtons;

    // Button to Character mapping
    UPROPERTY()
    TMap<UButton*, AC_IdleCharacter*> ButtonToCharacterMap;

    // Button to Team mapping
    UPROPERTY()
    TMap<UButton*, int32> ButtonToTeamIndexMap;

public:
    // Main Functions
    UFUNCTION(BlueprintCallable, Category = "Panel Inventory")
    void InitializePanel(UTeamComponent* InTeamComponent, UInventoryComponent* InBaseInventory);

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory")
    void RefreshPanel();

    // Team Selection
    UFUNCTION()
    void OnTeamButtonClicked();
    
    UFUNCTION()
    void OnTeamButtonClickedWithIndex(int32 TeamIndex);

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory|Team")
    void SelectBase();

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory|Team")
    void SelectTeam(int32 TeamIndex);

    // Individual team selection functions (for button binding)
    UFUNCTION()
    void SelectTeam0() { SelectTeam(0); }

    UFUNCTION()
    void SelectTeam1() { SelectTeam(1); }

    UFUNCTION()
    void SelectTeam2() { SelectTeam(2); }

    // Member Selection
    UFUNCTION()
    void OnMemberButtonClicked();
    
    UFUNCTION()
    void OnMemberButtonClickedWithData(UObject* Data);

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory|Team")
    void RefreshTeamButtons();

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory|Member")
    void RefreshMemberButtons();

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory|Member")
    void RefreshTeamInventory();

    UFUNCTION(BlueprintCallable, Category = "Panel Inventory|Member")
    void RefreshMemberInventory();

protected:
    // Auto-initialization
    UFUNCTION()
    void AutoInitializeWithPlayerController();
    
    // Auto-select first member
    UFUNCTION()
    void AutoSelectFirstMember();

    // UI Creation Functions
    UFUNCTION()
    void CreateTeamButton(const FString& ButtonText, int32 TeamIndex);

    UFUNCTION()
    void CreateMemberButton(AC_IdleCharacter* Character);

    UFUNCTION()
    void ClearTeamButtons();

    UFUNCTION()
    void ClearMemberButtons();

    UFUNCTION()
    void UpdateDisplayTexts();

    // Event Handlers
    UFUNCTION()
    void OnTeamsUpdated();

    UFUNCTION()
    void OnCharacterListChanged();

    // Utility Functions
    TArray<AC_IdleCharacter*> GetCurrentMemberList() const;
    UInventoryComponent* GetCurrentTeamInventory() const;
    FString GetCurrentDisplayName() const;

    // Event binding
    void BindTeamEvents();
    void UnbindTeamEvents();

private:
    // Button Widget Classes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget Classes", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<class UC_InventorySelectButton> InventorySelectButtonClass;

    // Button styling
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button Style", meta = (AllowPrivateAccess = "true"))
    FName NormalButtonStyle = TEXT("Button");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button Style", meta = (AllowPrivateAccess = "true"))
    FName SelectedButtonStyle = TEXT("Button.Selected");
};
