#include "C_GameInstance.h"
#include "Managers/ItemDataTableManager.h"
#include "Managers/CharacterPresetManager.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"

UC_GameInstance::UC_GameInstance()
{
    bIsInitialized = false;
    
    // デフォルトDataTableパス（プロジェクトに合わせて変更）
    static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataFinder(
        TEXT("/Game/Data/DT_ItemData"));
    if (ItemDataFinder.Succeeded())
    {
        ItemDataTable = ItemDataFinder.Object;
    }
    
    static ConstructorHelpers::FObjectFinder<UDataTable> CharacterPresetFinder(
        TEXT("/Game/Data/CharacterPresets"));
    if (CharacterPresetFinder.Succeeded())
    {
        CharacterPresetDataTable = CharacterPresetFinder.Object;
    }
    
    static ConstructorHelpers::FObjectFinder<UDataTable> LocationDataFinder(
        TEXT("/Game/Data/LocationData"));
    if (LocationDataFinder.Succeeded())
    {
        LocationDataTable = LocationDataFinder.Object;
    }
}

void UC_GameInstance::Init()
{
    Super::Init();
    
    UE_LOG(LogTemp, Log, TEXT("C_GameInstance::Init started"));
    
    // 0.2秒遅延後にDataTableを初期化（Blueprintと同じ挙動）
    FTimerHandle InitTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(InitTimerHandle, this, &UC_GameInstance::InitializeDataTables, 0.2f, false);
}

void UC_GameInstance::Shutdown()
{
    Super::Shutdown();
    
    UE_LOG(LogTemp, Log, TEXT("C_GameInstance::Shutdown"));
}

void UC_GameInstance::InitializeDataTables()
{
    if (bIsInitialized)
    {
        return;
    }
    
    // ItemDataTableManagerの初期化
    if (UItemDataTableManager* ItemManager = GetSubsystem<UItemDataTableManager>())
    {
        if (ItemDataTable)
        {
            ItemManager->SetItemDataTable(ItemDataTable);
            UE_LOG(LogTemp, Log, TEXT("ItemDataTable set: %s"), 
                *ItemDataTable->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ItemDataTable is not set!"));
        }
    }
    
    // CharacterPresetManagerの初期化
    if (UCharacterPresetManager* PresetManager = GetSubsystem<UCharacterPresetManager>())
    {
        if (CharacterPresetDataTable)
        {
            PresetManager->SetCharacterPresetDataTable(CharacterPresetDataTable);
            UE_LOG(LogTemp, Log, TEXT("CharacterPresetDataTable set: %s"), 
                *CharacterPresetDataTable->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterPresetDataTable is not set"));
        }
        
        if (LocationDataTable)
        {
            PresetManager->SetLocationDataTable(LocationDataTable);
            UE_LOG(LogTemp, Log, TEXT("LocationDataTable set: %s"), 
                *LocationDataTable->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("LocationDataTable is not set"));
        }
    }
    
    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("C_GameInstance: All DataTables initialized"));
}