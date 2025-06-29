#include "C_GameInstance.h"
#include "Managers/ItemDataTableManager.h"
#include "Managers/LocationDataTableManager.h"
#include "Managers/CharacterPresetManager.h"
#include "Managers/FacilityManager.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"

UC_GameInstance::UC_GameInstance()
{
    bIsInitialized = false;
    
    // デフォルトDataTableパス（DT_プレフィックスで統一）
    static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataFinder(
        TEXT("/Game/Data/DT_ItemData"));
    if (ItemDataFinder.Succeeded())
    {
        ItemDataTable = ItemDataFinder.Object;
        UE_LOG(LogTemp, Warning, TEXT("C_GameInstance Constructor: Found ItemDataTable at /Game/Data/DT_ItemData"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("C_GameInstance Constructor: ItemDataTable not found at /Game/Data/DT_ItemData"));
    }
    
    static ConstructorHelpers::FObjectFinder<UDataTable> CharacterPresetFinder(
        TEXT("/Game/Data/DT_CharacterPresets"));
    if (CharacterPresetFinder.Succeeded())
    {
        CharacterPresetDataTable = CharacterPresetFinder.Object;
        UE_LOG(LogTemp, Warning, TEXT("C_GameInstance Constructor: Found CharacterPresetDataTable at /Game/Data/DT_CharacterPresets"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("C_GameInstance Constructor: CharacterPresetDataTable not found at /Game/Data/DT_CharacterPresets"));
    }
    
    static ConstructorHelpers::FObjectFinder<UDataTable> LocationDataFinder(
        TEXT("/Game/Data/DT_LocationData"));
    if (LocationDataFinder.Succeeded())
    {
        LocationDataTable = LocationDataFinder.Object;
        UE_LOG(LogTemp, Warning, TEXT("C_GameInstance Constructor: Found LocationDataTable at /Game/Data/DT_LocationData"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("C_GameInstance Constructor: LocationDataTable not found at /Game/Data/DT_LocationData"));
    }
    
    static ConstructorHelpers::FObjectFinder<UDataTable> FacilityDataFinder(
        TEXT("/Game/Data/DT_FacilityData"));
    if (FacilityDataFinder.Succeeded())
    {
        FacilityDataTable = FacilityDataFinder.Object;
        UE_LOG(LogTemp, Warning, TEXT("C_GameInstance Constructor: Found FacilityDataTable at /Game/Data/DT_FacilityData"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("C_GameInstance Constructor: FacilityDataTable not found at /Game/Data/DT_FacilityData"));
    }
}

void UC_GameInstance::Init()
{
    Super::Init();
    
    UE_LOG(LogTemp, Warning, TEXT("C_GameInstance::Init started"));
    
    // DataTable設定状況をログ出力
    UE_LOG(LogTemp, Warning, TEXT("C_GameInstance::Init - FacilityDataTable status: %s"), 
        FacilityDataTable ? *FacilityDataTable->GetName() : TEXT("NULL"));
    
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
        UE_LOG(LogTemp, Warning, TEXT("InitializeDataTables: ItemDataTableManager found"));
        if (ItemDataTable)
        {
            ItemManager->SetItemDataTable(ItemDataTable);
            UE_LOG(LogTemp, Warning, TEXT("ItemDataTable set successfully: %s with %d rows"), 
                *ItemDataTable->GetName(), ItemDataTable->GetRowMap().Num());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ItemDataTable is null! CSV file needs to be imported as DataTable asset in UE Editor."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ItemDataTableManager subsystem not found!"));
    }
    
    // LocationDataTableManagerの初期化
    if (ULocationDataTableManager* LocationManager = GetSubsystem<ULocationDataTableManager>())
    {
        if (LocationDataTable)
        {
            LocationManager->SetLocationDataTable(LocationDataTable);
            UE_LOG(LogTemp, Log, TEXT("LocationDataTable set: %s"), 
                *LocationDataTable->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("LocationDataTable is not set!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LocationDataTableManager not found!"));
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
    
    // FacilityManagerの初期化
    if (UFacilityManager* FacilityManager = GetSubsystem<UFacilityManager>())
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeDataTables - FacilityManager found"));
        if (FacilityDataTable)
        {
            FacilityManager->SetFacilityDataTable(FacilityDataTable);
            UE_LOG(LogTemp, Warning, TEXT("FacilityDataTable set: %s"), 
                *FacilityDataTable->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FacilityDataTable is not set - CSV file needs to be imported as DataTable asset"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeDataTables - FacilityManager not found!"));
    }
    
    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("C_GameInstance: All DataTables initialized"));
}