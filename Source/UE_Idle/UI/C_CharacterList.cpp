#include "C_CharacterList.h"
#include "../C_PlayerController.h"
#include "../Interfaces/PlayerControllerInterface.h"
#include "../Interfaces/IdleCharacterInterface.h"
#include "../Components/TeamComponent.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"

UC_CharacterList::UC_CharacterList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set default character card widget class (to be set in Blueprint)
    CharacterCardWidgetClass = UC_CharacterCard::StaticClass();
}

void UC_CharacterList::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind to TeamComponent events instead of polling
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(GetOwningPlayer());
    if (PlayerController)
    {
        UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
        if (TeamComp)
        {
            TeamComp->OnCharacterListChanged.AddDynamic(this, &UC_CharacterList::OnTeamCharacterListChanged);
            UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList: Bound to TeamComponent events"));
        }
    }
    
    // Initial refresh
    RefreshCharacterList();
}

void UC_CharacterList::NativeDestruct()
{
    // Clear timer
    if (GetWorld() && RefreshTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }
    
    ClearCharacterCards();
    Super::NativeDestruct();
}

void UC_CharacterList::RefreshCharacterList()
{
    // Get character list from PlayerController
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(GetOwningPlayer());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList: PlayerController not found"));
        return;
    }

    // Get updated character list using Interface Execute function
    TArray<AActor*> ActorList = IPlayerControllerInterface::Execute_GetCharacterList(PlayerController);
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::RefreshCharacterList - Found %d actors"), ActorList.Num());
    
    TArray<AC_IdleCharacter*> NewCharacterList;
    for (AActor* Actor : ActorList)
    {
        if (AC_IdleCharacter* IdleCharacter = Cast<AC_IdleCharacter>(Actor))
        {
            NewCharacterList.Add(IdleCharacter);
            UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::RefreshCharacterList - Added character: %s"), 
                   *IIdleCharacterInterface::Execute_GetCharacterName(IdleCharacter));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::RefreshCharacterList - Total characters: %d"), NewCharacterList.Num());
    
    // Check if the list has changed
    bool bListChanged = (NewCharacterList.Num() != CachedCharacterList.Num());
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::RefreshCharacterList - List size changed: %s (Old: %d, New: %d)"), 
           bListChanged ? TEXT("YES") : TEXT("NO"), CachedCharacterList.Num(), NewCharacterList.Num());
    
    if (!bListChanged)
    {
        for (int32 i = 0; i < NewCharacterList.Num(); i++)
        {
            if (NewCharacterList[i] != CachedCharacterList[i])
            {
                bListChanged = true;
                UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::RefreshCharacterList - Character at index %d changed"), i);
                break;
            }
        }
    }

    // Update cached list and refresh UI if changed
    if (bListChanged)
    {
        CachedCharacterList = NewCharacterList;
        OnCharacterListChanged();
        
        // Keep timer running to detect new characters
    }
}

void UC_CharacterList::UpdateCharacterCards()
{
    // Update existing character cards with current data
    for (UC_CharacterCard* CharacterCard : CharacterCardWidgets)
    {
        if (CharacterCard)
        {
            CharacterCard->UpdateDisplay();
        }
    }
}

void UC_CharacterList::CreateCharacterCard(AC_IdleCharacter* Character)
{
    if (!Character || !CharacterCardWidgetClass || !CharacterCardPanel)
    {
        return;
    }

    // Create new character card widget
    UC_CharacterCard* NewCharacterCard = CreateWidget<UC_CharacterCard>(this, CharacterCardWidgetClass);
    if (NewCharacterCard)
    {
        // Initialize card with character (once, permanently)
        NewCharacterCard->InitializeWithCharacter(Character);
        
        // Add to panel
        CharacterCardPanel->AddChild(NewCharacterCard);
        
        // Cache the widget
        CharacterCardWidgets.Add(NewCharacterCard);
        
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::CreateCharacterCard - Created card for: %s"), 
               *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
}

void UC_CharacterList::ClearCharacterCards()
{
    UE_LOG(LogTemp, Error, TEXT("UC_CharacterList::ClearCharacterCards - WARNING: Clearing all cards! This should not happen with differential updates!"));
    
    // Remove all character card widgets
    for (UC_CharacterCard* CharacterCard : CharacterCardWidgets)
    {
        if (CharacterCard)
        {
            CharacterCard->RemoveFromParent();
        }
    }
    
    CharacterCardWidgets.Empty();
    
    // Clear panel
    if (CharacterCardPanel)
    {
        CharacterCardPanel->ClearChildren();
    }
}

void UC_CharacterList::OnCharacterListChanged()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::OnCharacterListChanged - Updating cards (Existing: %d, New: %d)"), 
           CharacterCardWidgets.Num(), CachedCharacterList.Num());
    
    // Remove cards for characters that no longer exist
    for (int32 i = CharacterCardWidgets.Num() - 1; i >= 0; i--)
    {
        UC_CharacterCard* CardWidget = CharacterCardWidgets[i];
        if (!CardWidget || !CachedCharacterList.Contains(CardWidget->GetCharacter()))
        {
            if (CardWidget)
            {
                UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::OnCharacterListChanged - Removing card"));
                CardWidget->RemoveFromParent();
            }
            CharacterCardWidgets.RemoveAt(i);
        }
    }
    
    // Add cards for new characters
    for (AC_IdleCharacter* Character : CachedCharacterList)
    {
        bool bCardExists = false;
        for (UC_CharacterCard* ExistingCard : CharacterCardWidgets)
        {
            if (ExistingCard && ExistingCard->GetCharacter() == Character)
            {
                bCardExists = true;
                break;
            }
        }
        
        if (!bCardExists)
        {
            UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::OnCharacterListChanged - Adding new card for: %s"), 
                   *IIdleCharacterInterface::Execute_GetCharacterName(Character));
            CreateCharacterCard(Character);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::OnCharacterListChanged - Final card count: %d"), CharacterCardWidgets.Num());
}

void UC_CharacterList::ForceRefresh()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::ForceRefresh - Manual refresh called"));
    RefreshCharacterList();
}

void UC_CharacterList::InitializeWithDelay()
{
    // Alternative method for Blueprint use
    if (CachedCharacterList.Num() == 0)
    {
        GetWorld()->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UC_CharacterList::RefreshCharacterList, 0.1f, true);
    }
}

void UC_CharacterList::StartPeriodicRefresh()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UC_CharacterList::RefreshCharacterList, 0.5f, true);
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList: Started periodic refresh (0.5s interval)"));
    }
}

void UC_CharacterList::StopPeriodicRefresh()
{
    if (GetWorld() && RefreshTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList: Stopped periodic refresh"));
    }
}

void UC_CharacterList::OnTeamCharacterListChanged()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_CharacterList::OnTeamCharacterListChanged - Character list event received!"));
    RefreshCharacterList();
}
