#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_InventorySelectButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySelectButtonClicked, int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySelectButtonClickedWithData, UObject*, Data);

UCLASS(BlueprintType, Blueprintable)
class UE_IDLE_API UC_InventorySelectButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UC_InventorySelectButton(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
    class UButton* SelectButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI")
    class UTextBlock* ButtonText;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    int32 ButtonIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    FString DisplayText;
    
    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    UObject* AssociatedData = nullptr;

public:
    UPROPERTY(BlueprintAssignable, Category = "Button")
    FOnInventorySelectButtonClicked OnClicked;
    
    UPROPERTY(BlueprintAssignable, Category = "Button")
    FOnInventorySelectButtonClickedWithData OnClickedWithData;

    UFUNCTION(BlueprintCallable, Category = "Inventory Select Button")
    void InitializeButton(int32 InIndex, const FString& InDisplayText, UObject* InData = nullptr);

protected:
    UFUNCTION()
    void HandleButtonClicked();
};