#include "C_TeamList.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "../Components/TeamComponent.h"
#include "C_TeamCard.h"

UC_TeamList::UC_TeamList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsInitialized = false;
    TeamComponent = nullptr;
    
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamList C++ WIDGET CREATED ====="));
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamList C++ WIDGET CREATED ====="));
    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamList C++ WIDGET CREATED ====="));
}

void UC_TeamList::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Error, TEXT("========== UC_TeamList::NativeConstruct - WIDGET IS BEING CONSTRUCTED =========="));

    if (CreateTeamButton)
    {
        CreateTeamButton->OnClicked.AddDynamic(this, &UC_TeamList::OnCreateTeamClicked);
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::NativeConstruct - SUCCESS: CreateTeamButton found and bound"));
        
        // ボタンの有効性もチェック
        if (CreateTeamButton->GetIsEnabled())
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamList::NativeConstruct - CreateTeamButton is ENABLED"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamList::NativeConstruct - CreateTeamButton is DISABLED"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::NativeConstruct - FATAL: CreateTeamButton is NULL! Check Blueprint binding"));
    }

    if (TeamsWrapBox)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::NativeConstruct - SUCCESS: TeamsWrapBox found"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::NativeConstruct - FATAL: TeamsWrapBox is NULL! Check Blueprint binding"));
    }

    UE_LOG(LogTemp, Error, TEXT("========== UC_TeamList::NativeConstruct - WIDGET CONSTRUCTION COMPLETED =========="));

    // 自動でTeamComponentを取得して初期化を試行
    TryAutoInitialize();
}

void UC_TeamList::TryAutoInitialize()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::TryAutoInitialize - Attempting auto-initialization"));

    if (bIsInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamList::TryAutoInitialize - Already initialized, skipping"));
        return;
    }

    // PlayerControllerからTeamComponentを取得
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (UTeamComponent* FoundTeamComponent = PC->GetComponentByClass<UTeamComponent>())
            {
                UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::TryAutoInitialize - Found TeamComponent, initializing"));
                InitializeWithTeamComponent(FoundTeamComponent);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UC_TeamList::TryAutoInitialize - TeamComponent not found on PlayerController"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UC_TeamList::TryAutoInitialize - PlayerController not found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::TryAutoInitialize - World not found"));
    }
}

void UC_TeamList::NativeDestruct()
{
    UnbindTeamEvents();
    ClearAllTeamCards();

    if (CreateTeamButton)
    {
        CreateTeamButton->OnClicked.RemoveDynamic(this, &UC_TeamList::OnCreateTeamClicked);
    }

    UE_LOG(LogTemp, Error, TEXT("===== UC_TeamList C++ WIDGET DESTROYED ====="));

    Super::NativeDestruct();
}

void UC_TeamList::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // 最初の数秒だけログを出す（スパム防止）
    static float LogTimer = 0.0f;
    static bool bHasLogged = false;
    static bool bRetryInitialization = true;
    
    if (!bHasLogged && LogTimer < 3.0f)
    {
        LogTimer += InDeltaTime;
        if (LogTimer >= 1.0f && LogTimer < 1.1f) // 1秒後に1回だけログ
        {
            UE_LOG(LogTemp, Error, TEXT("===== UC_TeamList C++ WIDGET IS VISIBLE AND TICKING ====="));
            UE_LOG(LogTemp, Error, TEXT("Button Valid: %s, Initialized: %s"), 
                   CreateTeamButton ? TEXT("YES") : TEXT("NO"),
                   bIsInitialized ? TEXT("YES") : TEXT("NO"));
            bHasLogged = true;
            
            // 初期化されていない場合は再試行
            if (!bIsInitialized && bRetryInitialization)
            {
                UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::NativeTick - Not initialized, retrying auto-initialization"));
                TryAutoInitialize();
                bRetryInitialization = false;
            }
        }
    }
}

void UC_TeamList::InitializeWithTeamComponent(UTeamComponent* InTeamComponent)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::InitializeWithTeamComponent - Starting initialization"));

    if (!InTeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::InitializeWithTeamComponent - InTeamComponent is null"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::InitializeWithTeamComponent - Valid TeamComponent provided"));

    // 既存のイベントバインドを解除
    UnbindTeamEvents();

    TeamComponent = InTeamComponent;
    bIsInitialized = true;

    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::InitializeWithTeamComponent - TeamComponent set, bIsInitialized = true"));

    // 新しいTeamComponentのイベントにバインド
    BindTeamEvents();

    // 初回表示更新
    UpdateDisplay();

    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::InitializeWithTeamComponent - Initialization completed successfully"));
}

void UC_TeamList::UpdateDisplay()
{
    if (!bIsInitialized || !TeamComponent)
    {
        return;
    }

    RefreshAllTeamCards();
}

void UC_TeamList::RefreshAllTeamCards()
{
    if (!TeamComponent || !TeamsWrapBox)
    {
        return;
    }

    // 既存のチームカードをクリア
    ClearAllTeamCards();

    // 全チームのカードを作成
    const TArray<FTeam>& Teams = TeamComponent->GetAllTeams();
    for (int32 i = 0; i < Teams.Num(); ++i)
    {
        if (Teams[i].IsValidTeam())
        {
            UC_TeamCard* TeamCard = CreateTeamCard(i);
            if (TeamCard)
            {
                TeamCards.Add(TeamCard);
                TeamsWrapBox->AddChildToWrapBox(TeamCard);
            }
        }
    }
}

void UC_TeamList::UpdateTeamCard(int32 TeamIndex)
{
    UC_TeamCard* ExistingCard = FindTeamCard(TeamIndex);
    if (ExistingCard)
    {
        ExistingCard->UpdateDisplay();
    }
    else
    {
        // カードが存在しない場合は新規作成
        if (TeamComponent && TeamComponent->IsValidTeamIndex(TeamIndex))
        {
            UC_TeamCard* NewCard = CreateTeamCard(TeamIndex);
            if (NewCard && TeamsWrapBox)
            {
                TeamCards.Add(NewCard);
                TeamsWrapBox->AddChildToWrapBox(NewCard);
            }
        }
    }
}

void UC_TeamList::OnCreateTeamClicked()
{
    UE_LOG(LogTemp, Error, TEXT("***** UC_TeamList::OnCreateTeamClicked - BUTTON WAS CLICKED! *****"));
    UE_LOG(LogTemp, Error, TEXT("***** UC_TeamList::OnCreateTeamClicked - BUTTON WAS CLICKED! *****"));
    UE_LOG(LogTemp, Error, TEXT("***** UC_TeamList::OnCreateTeamClicked - BUTTON WAS CLICKED! *****"));

    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::OnCreateTeamClicked - TeamComponent is NULL! Cannot create team"));
        return;
    }

    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::OnCreateTeamClicked - Widget is not initialized! Call InitializeWithTeamComponent first"));
        return;
    }

    UE_LOG(LogTemp, Error, TEXT("UC_TeamList::OnCreateTeamClicked - Calling ShowTeamNameInputDialog"));
    
    // Blueprint側のダイアログ表示を呼び出し
    ShowTeamNameInputDialog();
    
    UE_LOG(LogTemp, Error, TEXT("UC_TeamList::OnCreateTeamClicked - ShowTeamNameInputDialog called"));
    
    // Blueprint実装がない場合のフォールバック - デフォルト名でチーム作成
    UE_LOG(LogTemp, Error, TEXT("UC_TeamList::OnCreateTeamClicked - Creating default team as fallback"));
    CreateNewTeam();
}

void UC_TeamList::CreateNewTeam()
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::CreateNewTeam - Starting team creation"));

    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::CreateNewTeam - TeamComponent is null"));
        return;
    }

    // デフォルト名でチーム作成
    FString DefaultTeamName = FString::Printf(TEXT("チーム%d"), TeamComponent->GetAllTeams().Num() + 1);
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::CreateNewTeam - Creating team with name: %s"), *DefaultTeamName);
    
    int32 NewTeamIndex = TeamComponent->CreateTeam(DefaultTeamName);

    if (NewTeamIndex >= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamList::CreateNewTeam - Team created successfully with index: %d"), NewTeamIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::CreateNewTeam - Failed to create team (returned index: %d)"), NewTeamIndex);
    }
}

void UC_TeamList::ConfirmCreateTeam(const FString& TeamName)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::ConfirmCreateTeam - Called with TeamName: %s"), *TeamName);

    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::ConfirmCreateTeam - TeamComponent is null"));
        return;
    }

    FString ActualTeamName = TeamName.IsEmpty() ? TEXT("新しいチーム") : TeamName;
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::ConfirmCreateTeam - Creating team with name: %s"), *ActualTeamName);
    
    int32 NewTeamIndex = TeamComponent->CreateTeam(ActualTeamName);

    if (NewTeamIndex >= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("UC_TeamList::ConfirmCreateTeam - Team '%s' created with index: %d"), 
               *ActualTeamName, NewTeamIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::ConfirmCreateTeam - Failed to create team '%s' (returned index: %d)"), 
               *ActualTeamName, NewTeamIndex);
    }
}

void UC_TeamList::TestCreateTeam()
{
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::TestCreateTeam - Manual test function called"));
    
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::TestCreateTeam - Widget not initialized"));
        return;
    }
    
    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::TestCreateTeam - TeamComponent is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::TestCreateTeam - Creating test team"));
    CreateNewTeam();
}

void UC_TeamList::ForceButtonClick()
{
    UE_LOG(LogTemp, Error, TEXT("UC_TeamList::ForceButtonClick - Manually triggering button click event"));
    OnCreateTeamClicked();
}

bool UC_TeamList::IsCreateButtonValid() const
{
    if (CreateTeamButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::IsCreateButtonValid - Button exists, enabled: %s"), 
               CreateTeamButton->GetIsEnabled() ? TEXT("true") : TEXT("false"));
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UC_TeamList::IsCreateButtonValid - Button is NULL"));
        return false;
    }
}

void UC_TeamList::BindTeamEvents()
{
    if (!TeamComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::BindTeamEvents - TeamComponent is null"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::BindTeamEvents - Binding events to TeamComponent"));

    TeamComponent->OnTeamCreated.AddDynamic(this, &UC_TeamList::OnTeamCreated);
    TeamComponent->OnTeamDeleted.AddDynamic(this, &UC_TeamList::OnTeamDeleted);
    TeamComponent->OnTeamsUpdated.AddDynamic(this, &UC_TeamList::OnTeamsUpdated);
    TeamComponent->OnMemberAssigned.AddDynamic(this, &UC_TeamList::OnMemberAssigned);
    TeamComponent->OnMemberRemoved.AddDynamic(this, &UC_TeamList::OnMemberRemoved);
    TeamComponent->OnTaskChanged.AddDynamic(this, &UC_TeamList::OnTaskChanged);
    TeamComponent->OnTeamNameChanged.AddDynamic(this, &UC_TeamList::OnTeamNameChanged);

    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::BindTeamEvents - All events bound successfully"));
}

void UC_TeamList::UnbindTeamEvents()
{
    if (!TeamComponent)
    {
        return;
    }

    TeamComponent->OnTeamCreated.RemoveDynamic(this, &UC_TeamList::OnTeamCreated);
    TeamComponent->OnTeamDeleted.RemoveDynamic(this, &UC_TeamList::OnTeamDeleted);
    TeamComponent->OnTeamsUpdated.RemoveDynamic(this, &UC_TeamList::OnTeamsUpdated);
    TeamComponent->OnMemberAssigned.RemoveDynamic(this, &UC_TeamList::OnMemberAssigned);
    TeamComponent->OnMemberRemoved.RemoveDynamic(this, &UC_TeamList::OnMemberRemoved);
    TeamComponent->OnTaskChanged.RemoveDynamic(this, &UC_TeamList::OnTaskChanged);
    TeamComponent->OnTeamNameChanged.RemoveDynamic(this, &UC_TeamList::OnTeamNameChanged);
}

void UC_TeamList::OnTeamCreated(int32 TeamIndex, const FString& TeamName)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnTeamCreated - Team %d: %s"), TeamIndex, *TeamName);
    UpdateTeamCard(TeamIndex);
}

void UC_TeamList::OnTeamDeleted(int32 TeamIndex)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnTeamDeleted - Team %d"), TeamIndex);
    RemoveTeamCard(TeamIndex);
}

void UC_TeamList::OnTeamsUpdated()
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnTeamsUpdated"));
    RefreshAllTeamCards();
}

void UC_TeamList::OnMemberAssigned(int32 TeamIndex, AC_IdleCharacter* Character, const FString& TeamName)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnMemberAssigned - Team %d: %s"), TeamIndex, *TeamName);
    UpdateTeamCard(TeamIndex);
}

void UC_TeamList::OnMemberRemoved(int32 TeamIndex, AC_IdleCharacter* Character)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnMemberRemoved - Team %d"), TeamIndex);
    UpdateTeamCard(TeamIndex);
}

void UC_TeamList::OnTaskChanged(int32 TeamIndex, ETaskType NewTask)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnTaskChanged - Team %d"), TeamIndex);
    UpdateTeamCard(TeamIndex);
}

void UC_TeamList::OnTeamNameChanged(int32 TeamIndex, const FString& NewName)
{
    UE_LOG(LogTemp, Log, TEXT("UC_TeamList::OnTeamNameChanged - Team %d: %s"), TeamIndex, *NewName);
    UpdateTeamCard(TeamIndex);
}

void UC_TeamList::ClearAllTeamCards()
{
    if (TeamsWrapBox)
    {
        TeamsWrapBox->ClearChildren();
    }

    for (UC_TeamCard* Card : TeamCards)
    {
        if (Card)
        {
            Card->RemoveFromParent();
        }
    }

    TeamCards.Empty();
}

UC_TeamCard* UC_TeamList::CreateTeamCard(int32 TeamIndex)
{
    if (!TeamCardClass || !TeamComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::CreateTeamCard - TeamCardClass or TeamComponent is null"));
        return nullptr;
    }

    if (!TeamComponent->IsValidTeamIndex(TeamIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_TeamList::CreateTeamCard - Invalid team index: %d"), TeamIndex);
        return nullptr;
    }

    UC_TeamCard* NewCard = CreateWidget<UC_TeamCard>(this, TeamCardClass);
    if (NewCard)
    {
        NewCard->InitializeWithTeam(TeamIndex, TeamComponent);
    }

    return NewCard;
}

UC_TeamCard* UC_TeamList::FindTeamCard(int32 TeamIndex) const
{
    for (UC_TeamCard* Card : TeamCards)
    {
        if (Card && Card->GetTeamIndex() == TeamIndex)
        {
            return Card;
        }
    }
    return nullptr;
}

void UC_TeamList::RemoveTeamCard(int32 TeamIndex)
{
    UC_TeamCard* CardToRemove = FindTeamCard(TeamIndex);
    if (CardToRemove)
    {
        TeamCards.Remove(CardToRemove);
        CardToRemove->RemoveFromParent();
    }
}