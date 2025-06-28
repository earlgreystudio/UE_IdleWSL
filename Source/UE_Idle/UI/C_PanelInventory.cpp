#include "C_PanelInventory.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"
#include "../Components/TeamComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "C__InventoryList.h"
#include "C_InventorySelectButton.h"

UC_PanelInventory::UC_PanelInventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UC_PanelInventory::NativeConstruct()
{
    Super::NativeConstruct();

    // Set ScrollBox orientation to horizontal
    if (TeamSelectionScrollBox)
    {
        TeamSelectionScrollBox->SetOrientation(EOrientation::Orient_Horizontal);
    }
    
    if (MemberButtonsScrollBox)
    {
        MemberButtonsScrollBox->SetOrientation(EOrientation::Orient_Horizontal);
    }

    // Auto-initialize with PlayerController components
    AutoInitializeWithPlayerController();
}

void UC_PanelInventory::AutoInitializeWithPlayerController()
{
    // Get PlayerController
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory: No PlayerController found"));
        return;
    }

    // Get TeamComponent from PlayerController
    UTeamComponent* TeamComp = PC->FindComponentByClass<UTeamComponent>();
    if (!TeamComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory: No TeamComponent found on PlayerController"));
        return;
    }

    // Get BaseInventory from PlayerController
    UInventoryComponent* BaseInventory = PC->FindComponentByClass<UInventoryComponent>();
    if (!BaseInventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory: No InventoryComponent found on PlayerController"));
        return;
    }

    // Initialize panel
    InitializePanel(TeamComp, BaseInventory);
    
    // Auto-select first member after initialization
    AutoSelectFirstMember();
    RefreshMemberInventory();
    UpdateDisplayTexts();
    
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory: Auto-initialized successfully"));
}

void UC_PanelInventory::AutoSelectFirstMember()
{
    TArray<AC_IdleCharacter*> Members = GetCurrentMemberList();
    
    if (Members.Num() > 0)
    {
        CurrentSelectedMember = Members[0];
        UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::AutoSelectFirstMember - Auto-selected: %s"), 
               CurrentSelectedMember ? *CurrentSelectedMember->GetCharacterName_Implementation() : TEXT("NULL"));
    }
    else
    {
        CurrentSelectedMember = nullptr;
        UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::AutoSelectFirstMember - No members available, cleared selection"));
    }
}

void UC_PanelInventory::NativeDestruct()
{
    UnbindTeamEvents();
    Super::NativeDestruct();
}

void UC_PanelInventory::InitializePanel(UTeamComponent* InTeamComponent, UInventoryComponent* InBaseInventory)
{
    if (!InTeamComponent || !InBaseInventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory: InitializePanel called with null components"));
        return;
    }

    // Unbind from previous components
    if (CachedTeamComponent)
    {
        UnbindTeamEvents();
    }

    // Set new components
    CachedTeamComponent = InTeamComponent;
    BaseInventoryComponent = InBaseInventory;

    // Bind to new components
    BindTeamEvents();

    // Refresh entire panel
    RefreshPanel();
}

void UC_PanelInventory::RefreshPanel()
{
    RefreshTeamButtons();
    RefreshMemberButtons();
    RefreshTeamInventory();
    RefreshMemberInventory();
    UpdateDisplayTexts();
}

void UC_PanelInventory::OnTeamButtonClicked()
{
    // 送信者ボタンを特定する方法がないため、
    // 動的にバインド時にラムダ関数を使用する必要があります
    UE_LOG(LogTemp, Warning, TEXT("OnTeamButtonClicked: Cannot identify specific button - need individual callbacks"));
    
    // 一時的な解決策: 最初に見つかったボタンのチームを選択
    for (auto& Pair : ButtonToTeamIndexMap)
    {
        if (Pair.Key && Pair.Key->IsPressed())
        {
            SelectTeam(Pair.Value);
            return;
        }
    }
}

void UC_PanelInventory::OnTeamButtonClickedWithIndex(int32 TeamIndex)
{
    if (TeamIndex == -1)
    {
        SelectBase();
    }
    else
    {
        SelectTeam(TeamIndex);
    }
}

void UC_PanelInventory::SelectBase()
{
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::SelectBase - Switching to Base mode"));
    
    CurrentPanelMode = EInventoryPanelMode::Base;
    CurrentTeamIndex = -1;
    CurrentSelectedMember = nullptr;

    RefreshMemberButtons();
    RefreshTeamInventory();
    
    // Auto-select first member if available
    AutoSelectFirstMember();
    
    RefreshMemberInventory();
    UpdateDisplayTexts();
    
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::SelectBase - Base mode activated"));
}

void UC_PanelInventory::SelectTeam(int32 TeamIndex)
{
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::SelectTeam - Switching to Team %d"), TeamIndex);
    
    if (!CachedTeamComponent || TeamIndex < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory::SelectTeam - Invalid parameters: TeamComponent=%s, TeamIndex=%d"), 
               CachedTeamComponent ? TEXT("Valid") : TEXT("NULL"), TeamIndex);
        return;
    }

    // チームが存在するかチェック
    TArray<FTeam> AllTeams = CachedTeamComponent->GetAllTeams();
    if (TeamIndex >= AllTeams.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory::SelectTeam - Team %d does not exist (max: %d)"), TeamIndex, AllTeams.Num() - 1);
        return;
    }

    CurrentPanelMode = EInventoryPanelMode::Team;
    CurrentTeamIndex = TeamIndex;
    CurrentSelectedMember = nullptr;

    RefreshMemberButtons();
    RefreshTeamInventory();
    
    // Auto-select first member if available
    AutoSelectFirstMember();
    
    RefreshMemberInventory();
    UpdateDisplayTexts();
    
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::SelectTeam - Team %d mode activated"), TeamIndex);
}

void UC_PanelInventory::OnMemberButtonClicked()
{
    // マッピングから最初のメンバーを選択（一時的な実装）
    UE_LOG(LogTemp, Log, TEXT("Member button clicked - selecting first available member"));
    
    TArray<AC_IdleCharacter*> Members = GetCurrentMemberList();
    if (Members.Num() > 0)
    {
        CurrentSelectedMember = Members[0];  // 最初のメンバーを選択
        RefreshMemberInventory();
        UpdateDisplayTexts();
        
        UE_LOG(LogTemp, Log, TEXT("Selected member: %s"), CurrentSelectedMember ? *CurrentSelectedMember->GetName() : TEXT("NULL"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No members available for selection"));
    }
}

void UC_PanelInventory::RefreshTeamButtons()
{
    if (!TeamSelectionScrollBox)
    {
        return;
    }

    // 既存のボタンをクリア
    ClearTeamButtons();

    // 拠点ボタンを作成
    CreateTeamButton(TEXT("拠点"), -1);

    // チームボタンを作成（制限なし）
    if (CachedTeamComponent)
    {
        TArray<FTeam> AllTeams = CachedTeamComponent->GetAllTeams();
        for (int32 i = 0; i < AllTeams.Num(); i++)
        {
            FString TeamDisplayName;
            if (!AllTeams[i].TeamName.IsEmpty())
            {
                TeamDisplayName = AllTeams[i].TeamName;
            }
            else
            {
                // デフォルト名生成（A, B, C, D, E...）
                TeamDisplayName = FString::Printf(TEXT("チーム%c"), 'A' + i);
            }
            
            CreateTeamButton(TeamDisplayName, i);
        }
    }
}

void UC_PanelInventory::RefreshMemberButtons()
{
    if (!MemberButtonsScrollBox)
    {
        return;
    }

    // 既存のボタンをクリア
    ClearMemberButtons();

    // 現在の選択に応じてメンバーリストを取得
    TArray<AC_IdleCharacter*> Members = GetCurrentMemberList();

    // 各メンバーのボタンを作成
    for (AC_IdleCharacter* Character : Members)
    {
        if (Character)
        {
            CreateMemberButton(Character);
        }
    }
}

void UC_PanelInventory::RefreshTeamInventory()
{
    if (!TeamInventoryList)
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory::RefreshTeamInventory - TeamInventoryList is NULL"));
        return;
    }

    UInventoryComponent* TargetInventory = GetCurrentTeamInventory();
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::RefreshTeamInventory - TargetInventory: %s"), 
           TargetInventory ? TEXT("Valid") : TEXT("NULL"));
    
    if (TargetInventory)
    {
        TeamInventoryList->InitializeWithInventory(TargetInventory);
        UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::RefreshTeamInventory - TeamInventoryList initialized"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory::RefreshTeamInventory - No target inventory found"));
    }
}

void UC_PanelInventory::RefreshMemberInventory()
{
    if (!MemberInventoryList)
    {
        return;
    }

    if (CurrentSelectedMember)
    {
        if (UInventoryComponent* MemberInventory = CurrentSelectedMember->FindComponentByClass<UInventoryComponent>())
        {
            MemberInventoryList->InitializeWithInventory(MemberInventory);
        }
    }
    else
    {
        // メンバーが選択されていない場合は空のリストを表示
        MemberInventoryList->InitializeWithInventory(nullptr);
    }
}

void UC_PanelInventory::CreateTeamButton(const FString& ButtonText, int32 TeamIndex)
{
    if (!TeamSelectionScrollBox)
    {
        return;
    }

    // InventorySelectButtonClassが設定されている場合は専用ウィジェットを使用
    if (InventorySelectButtonClass)
    {
        UC_InventorySelectButton* SelectButton = CreateWidget<UC_InventorySelectButton>(this, InventorySelectButtonClass);
        if (SelectButton)
        {
            SelectButton->InitializeButton(TeamIndex, ButtonText);
            SelectButton->OnClicked.AddDynamic(this, &UC_PanelInventory::OnTeamButtonClickedWithIndex);
            TeamSelectionScrollBox->AddChild(SelectButton);
            return;
        }
    }

    // フォールバック: 通常のボタンを作成
    UButton* NewButton = WidgetTree->ConstructWidget<UButton>();
    if (!NewButton)
    {
        return;
    }

    // ボタンにテキストブロックを追加
    UTextBlock* ButtonTextWidget = WidgetTree->ConstructWidget<UTextBlock>();
    if (ButtonTextWidget)
    {
        ButtonTextWidget->SetText(FText::FromString(ButtonText));
        NewButton->AddChild(ButtonTextWidget);
    }

    // チーム情報をマッピングに保存
    ButtonToTeamIndexMap.Add(NewButton, TeamIndex);

    // 個別のクリックイベントをバインド
    if (TeamIndex == -1)
    {
        // 拠点ボタン
        NewButton->OnClicked.AddDynamic(this, &UC_PanelInventory::SelectBase);
    }
    else
    {
        // チームボタン（AddDynamicでキャプチャ変数を渡せないため、一時的な解決策）
        // TODO: より良い解決策を実装する
        if (TeamIndex == 0)
        {
            NewButton->OnClicked.AddDynamic(this, &UC_PanelInventory::SelectTeam0);
        }
        else if (TeamIndex == 1)
        {
            NewButton->OnClicked.AddDynamic(this, &UC_PanelInventory::SelectTeam1);
        }
        else if (TeamIndex == 2)
        {
            NewButton->OnClicked.AddDynamic(this, &UC_PanelInventory::SelectTeam2);
        }
        else
        {
            // 3以上のチームの場合は汎用ハンドラー
            NewButton->OnClicked.AddDynamic(this, &UC_PanelInventory::OnTeamButtonClicked);
        }
    }

    // ボタンをScrollBoxに追加
    TeamSelectionScrollBox->AddChild(NewButton);
    TeamButtons.Add(NewButton);
}

void UC_PanelInventory::CreateMemberButton(AC_IdleCharacter* Character)
{
    if (!Character || !MemberButtonsScrollBox)
    {
        return;
    }

    // InventorySelectButtonClassが設定されている場合は専用ウィジェットを使用
    if (InventorySelectButtonClass)
    {
        UC_InventorySelectButton* SelectButton = CreateWidget<UC_InventorySelectButton>(this, InventorySelectButtonClass);
        if (SelectButton)
        {
            FString CharacterName = Character->GetCharacterName_Implementation();
            UE_LOG(LogTemp, Log, TEXT("CreateMemberButton: Character=%s, Name=%s"), *Character->GetName(), *CharacterName);
            SelectButton->InitializeButton(0, CharacterName, Character);
            SelectButton->OnClickedWithData.AddDynamic(this, &UC_PanelInventory::OnMemberButtonClickedWithData);
            MemberButtonsScrollBox->AddChild(SelectButton);
            return;
        }
    }

    // フォールバック: 通常のボタンを作成
    UButton* NewButton = WidgetTree->ConstructWidget<UButton>();
    if (!NewButton)
    {
        return;
    }

    // ボタンにテキストブロックを追加
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
    if (ButtonText)
    {
        FString CharacterName = Character->GetCharacterName_Implementation();
        UE_LOG(LogTemp, Log, TEXT("CreateMemberButton (fallback): Character=%s, Name=%s"), *Character->GetName(), *CharacterName);
        ButtonText->SetText(FText::FromString(CharacterName));
        NewButton->AddChild(ButtonText);
    }

    // キャラクター情報をマッピングに保存
    ButtonToCharacterMap.Add(NewButton, Character);

    // キャラクターボタンのクリックイベント（特定の問題があるため、一時的に汎用ハンドラーを使用）
    NewButton->OnClicked.AddDynamic(this, &UC_PanelInventory::OnMemberButtonClicked);

    // ボタンをScrollBoxに追加
    MemberButtonsScrollBox->AddChild(NewButton);
    MemberButtons.Add(NewButton);
}

void UC_PanelInventory::OnMemberButtonClickedWithData(UObject* Data)
{
    if (AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(Data))
    {
        CurrentSelectedMember = Character;
        RefreshMemberInventory();
        UpdateDisplayTexts();
        
        UE_LOG(LogTemp, Log, TEXT("Selected member: %s"), *Character->GetName());
    }
}

void UC_PanelInventory::ClearTeamButtons()
{
    if (TeamSelectionScrollBox)
    {
        TeamSelectionScrollBox->ClearChildren();
    }
    TeamButtons.Empty();
    ButtonToTeamIndexMap.Empty();
}

void UC_PanelInventory::ClearMemberButtons()
{
    if (MemberButtonsScrollBox)
    {
        MemberButtonsScrollBox->ClearChildren();
    }
    MemberButtons.Empty();
    ButtonToCharacterMap.Empty();
}


void UC_PanelInventory::UpdateDisplayTexts()
{
    // 上段インベントリ名の表示
    if (UpperInventoryNameText)
    {
        UpperInventoryNameText->SetText(FText::FromString(GetCurrentDisplayName()));
    }

    // 下段インベントリ名の表示
    if (LowerInventoryNameText)
    {
        if (CurrentSelectedMember)
        {
            // キャラクター名を表示
            LowerInventoryNameText->SetText(FText::FromString(CurrentSelectedMember->GetName()));
        }
        else
        {
            // メンバー未選択時の表示
            LowerInventoryNameText->SetText(FText::FromString(TEXT("メンバーを選択してください")));
        }
    }
}

void UC_PanelInventory::OnTeamsUpdated()
{
    RefreshPanel();
}

void UC_PanelInventory::OnCharacterListChanged()
{
    RefreshMemberButtons();
}

TArray<AC_IdleCharacter*> UC_PanelInventory::GetCurrentMemberList() const
{
    if (!CachedTeamComponent)
    {
        return TArray<AC_IdleCharacter*>();
    }

    switch (CurrentPanelMode)
    {
        case EInventoryPanelMode::Base:
            // 拠点の場合は未所属のキャラクター
            return CachedTeamComponent->GetUnassignedCharacters();

        case EInventoryPanelMode::Team:
            // チームの場合はそのチームのメンバー
            if (CurrentTeamIndex >= 0)
            {
                FTeam Team = CachedTeamComponent->GetTeam(CurrentTeamIndex);
                return Team.Members;
            }
            break;
    }

    return TArray<AC_IdleCharacter*>();
}

UInventoryComponent* UC_PanelInventory::GetCurrentTeamInventory() const
{
    UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::GetCurrentTeamInventory - Mode: %s, TeamIndex: %d"), 
           CurrentPanelMode == EInventoryPanelMode::Base ? TEXT("Base") : TEXT("Team"), CurrentTeamIndex);

    switch (CurrentPanelMode)
    {
        case EInventoryPanelMode::Base:
            // 拠点の場合はBaseInventoryComponent
            UE_LOG(LogTemp, Log, TEXT("UC_PanelInventory::GetCurrentTeamInventory - Returning BaseInventory: %s"), 
                   BaseInventoryComponent ? TEXT("Valid") : TEXT("NULL"));
            return BaseInventoryComponent;

        case EInventoryPanelMode::Team:
            // 新採集システムではチームインベントリは削除されました
            // 個人キャラクターのインベントリを使用してください
            UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory::GetCurrentTeamInventory - Team inventory removed in new gathering system"));
            return nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("UC_PanelInventory::GetCurrentTeamInventory - Returning NULL"));
    return nullptr;
}

FString UC_PanelInventory::GetCurrentDisplayName() const
{
    switch (CurrentPanelMode)
    {
        case EInventoryPanelMode::Base:
            return TEXT("拠点の倉庫");

        case EInventoryPanelMode::Team:
            if (CachedTeamComponent && CurrentTeamIndex >= 0)
            {
                FTeam Team = CachedTeamComponent->GetTeam(CurrentTeamIndex);
                if (!Team.TeamName.IsEmpty())
                {
                    return FString::Printf(TEXT("%s のインベントリ"), *Team.TeamName);
                }
                else
                {
                    // A, B, C, D, E... 形式でデフォルト名を生成
                    FString TeamLetter = FString::Printf(TEXT("%c"), 'A' + CurrentTeamIndex);
                    return FString::Printf(TEXT("チーム%s のインベントリ"), *TeamLetter);
                }
            }
            break;
    }

    return TEXT("不明");
}

void UC_PanelInventory::BindTeamEvents()
{
    if (CachedTeamComponent)
    {
        CachedTeamComponent->OnTeamsUpdated.AddDynamic(this, &UC_PanelInventory::OnTeamsUpdated);
        CachedTeamComponent->OnCharacterListChanged.AddDynamic(this, &UC_PanelInventory::OnCharacterListChanged);
    }
}

void UC_PanelInventory::UnbindTeamEvents()
{
    if (CachedTeamComponent)
    {
        CachedTeamComponent->OnTeamsUpdated.RemoveDynamic(this, &UC_PanelInventory::OnTeamsUpdated);
        CachedTeamComponent->OnCharacterListChanged.RemoveDynamic(this, &UC_PanelInventory::OnCharacterListChanged);
    }
}
