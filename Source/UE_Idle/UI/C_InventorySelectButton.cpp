#include "C_InventorySelectButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UC_InventorySelectButton::UC_InventorySelectButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UC_InventorySelectButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (SelectButton)
    {
        SelectButton->OnClicked.AddDynamic(this, &UC_InventorySelectButton::HandleButtonClicked);
    }
}

void UC_InventorySelectButton::InitializeButton(int32 InIndex, const FString& InDisplayText, UObject* InData)
{
    ButtonIndex = InIndex;
    DisplayText = InDisplayText;
    AssociatedData = InData;

    if (ButtonText)
    {
        ButtonText->SetText(FText::FromString(DisplayText));
    }
}

void UC_InventorySelectButton::HandleButtonClicked()
{
    OnClicked.Broadcast(ButtonIndex);
    
    if (AssociatedData)
    {
        OnClickedWithData.Broadcast(AssociatedData);
    }
}
