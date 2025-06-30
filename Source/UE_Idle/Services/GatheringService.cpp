#include "GatheringService.h"
#include "../Actor/C_IdleCharacter.h"
#include "../Components/GatheringComponent.h"
#include "../Components/TeamComponent.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Managers/LocationDataTableManager.h"
#include "../C_PlayerController.h"
#include "MovementService.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UGatheringService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 統計情報初期化
    ResetStatistics();
    
    // 効率キャッシュ初期化
    EfficiencyCache.Empty();
    LastEfficiencyCacheUpdate = 0.0f;
    
    // アイテム・場所データ初期化
    InitializeItemLocationData();
    
    UE_LOG(LogTemp, Log, TEXT("⛏️ GatheringService initialized successfully"));
}

void UGatheringService::Deinitialize()
{
    // クリーンアップ
    EfficiencyCache.Empty();
    ItemRarityMap.Empty();
    LocationDifficultyMap.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("⛏️ GatheringService deinitialized"));
    
    Super::Deinitialize();
}

bool UGatheringService::ExecuteGathering(AC_IdleCharacter* Character, const FString& TargetItem)
{
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Error, TEXT("⛏️❌ GatheringService: Invalid character provided"));
        return false;
    }

    if (TargetItem.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("⛏️❌ GatheringService: Empty target item"));
        return false;
    }

    // 統計更新
    TotalGatheringRequests++;
    
    // 既存の採集ロジックを実行
    bool bSuccess = ExecuteExistingGatheringLogic(Character, TargetItem);
    
    if (bSuccess)
    {
        SuccessfulGatherings++;
        
        // 効率と時間を統計に追加
        float Efficiency = CalculateGatheringEfficiency(Character, TargetItem);
        float GatheringTime = EstimateGatheringTime(Character, TargetItem);
        
        TotalEfficiency += Efficiency;
        TotalGatheringTime += GatheringTime;
        
        UE_LOG(LogTemp, VeryVerbose, 
            TEXT("⛏️✅ GatheringService: Character %s gathering %s (efficiency: %.1f, time: %.1fs)"),
            *IIdleCharacterInterface::Execute_GetCharacterName(Character), *TargetItem, Efficiency, GatheringTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("⛏️⚠️ GatheringService: Failed gathering %s with character %s"),
            *TargetItem, *IIdleCharacterInterface::Execute_GetCharacterName(Character));
    }
    
    return bSuccess;
}

TArray<FString> UGatheringService::GetGatherableItemsAt(const FString& Location)
{
    if (Location.IsEmpty())
    {
        return TArray<FString>();
    }
    
    // 既存のLocationDataTableManagerを使用
    return GetExistingGatherableItems(Location);
}

bool UGatheringService::IsCharacterGathering(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    UTeamComponent* TeamComp = GetTeamComponent();
    if (!TeamComp)
    {
        return false;
    }
    
    // チームの現在の状態をチェック
    ETeamActionState ActionState = TeamComp->GetTeamActionState(TeamIndex);
    return ActionState == ETeamActionState::Working;
}

bool UGatheringService::ProcessGatheringProgress(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    
    // 採集完了の判定（簡易版）
    // 実際の実装では GatheringComponent の進行状況をチェック
    return !IsCharacterGathering(Character);
}

float UGatheringService::CalculateGatheringEfficiency(AC_IdleCharacter* Character, const FString& ItemType)
{
    if (!IsValid(Character))
    {
        return 1.0f; // デフォルト効率
    }
    
    // キャッシュチェック
    if (IsEfficiencyCacheValid() && EfficiencyCache.Contains(Character))
    {
        return EfficiencyCache[Character];
    }
    
    // 基本採集効率を計算
    float BaseEfficiency = CalculateBaseGatheringPower(Character);
    
    // アイテム別修正を適用
    float ItemModifiedEfficiency = ApplyItemTypeModifier(BaseEfficiency, ItemType);
    
    // 現在地による修正（簡易版 - 実際の場所取得は MovementService から）
    float FinalEfficiency = ApplyLocationModifier(ItemModifiedEfficiency, TEXT("plains"));
    
    // キャッシュに保存
    EfficiencyCache.Add(Character, FinalEfficiency);
    
    return FinalEfficiency;
}

float UGatheringService::EstimateGatheringTime(AC_IdleCharacter* Character, const FString& ItemType, int32 Quantity)
{
    float Efficiency = CalculateGatheringEfficiency(Character, ItemType);
    
    // 基本時間は1個につき1秒、効率で割る
    float BaseTime = 1.0f * Quantity;
    float EstimatedTime = BaseTime / FMath::Max(Efficiency, 0.1f);
    
    return FMath::Max(EstimatedTime, 0.5f); // 最低0.5秒
}

bool UGatheringService::CanCharacterGatherItem(AC_IdleCharacter* Character, const FString& ItemType, const FString& Location)
{
    if (!IsValid(Character) || ItemType.IsEmpty() || Location.IsEmpty())
    {
        return false;
    }
    
    // その場所で採集可能なアイテムかチェック
    TArray<FString> GatherableItems = GetGatherableItemsAt(Location);
    if (!GatherableItems.Contains(ItemType))
    {
        return false;
    }
    
    // キャラクターの能力チェック（簡易版）
    float Efficiency = CalculateGatheringEfficiency(Character, ItemType);
    return Efficiency > 0.0f;
}

TArray<FString> UGatheringService::FindLocationsForItem(const FString& ItemType)
{
    TArray<FString> Locations;
    
    if (ItemType.IsEmpty())
    {
        return Locations;
    }
    
    // 各場所で採集可能かチェック
    TArray<FString> AllLocations = {TEXT("base"), TEXT("plains")};
    
    for (const FString& Location : AllLocations)
    {
        TArray<FString> GatherableItems = GetGatherableItemsAt(Location);
        if (GatherableItems.Contains(ItemType))
        {
            Locations.Add(Location);
        }
    }
    
    return Locations;
}

int32 UGatheringService::GetLocationDifficultyLevel(const FString& Location)
{
    if (LocationDifficultyMap.Contains(Location))
    {
        return LocationDifficultyMap[Location];
    }
    
    return 1; // デフォルト難易度
}

int32 UGatheringService::GetItemRarityLevel(const FString& ItemType)
{
    if (ItemRarityMap.Contains(ItemType))
    {
        return ItemRarityMap[ItemType];
    }
    
    return 1; // デフォルト希少度
}

FString UGatheringService::GetGatheringStatistics()
{
    float SuccessRate = (TotalGatheringRequests > 0) ? 
        (float)SuccessfulGatherings / TotalGatheringRequests * 100.0f : 0.0f;
    
    float AverageGatheringTime = (SuccessfulGatherings > 0) ? 
        TotalGatheringTime / SuccessfulGatherings : 0.0f;
    
    float AverageEfficiency = (SuccessfulGatherings > 0) ? 
        TotalEfficiency / SuccessfulGatherings : 0.0f;
    
    return FString::Printf(
        TEXT("Gathering Stats: %d requests, %d successful (%.1f%%), avg time: %.1fs, avg efficiency: %.1f"),
        TotalGatheringRequests, SuccessfulGatherings, SuccessRate, AverageGatheringTime, AverageEfficiency);
}

TArray<FString> UGatheringService::GetAllGatherableItems()
{
    TArray<FString> AllItems;
    
    // 全場所から採集可能アイテムを集計
    TArray<FString> AllLocations = {TEXT("base"), TEXT("plains")};
    
    for (const FString& Location : AllLocations)
    {
        TArray<FString> LocationItems = GetGatherableItemsAt(Location);
        for (const FString& Item : LocationItems)
        {
            AllItems.AddUnique(Item);
        }
    }
    
    return AllItems;
}

bool UGatheringService::ExecuteExistingGatheringLogic(AC_IdleCharacter* Character, const FString& ItemType)
{
    int32 TeamIndex = GetCharacterTeamIndex(Character);
    if (TeamIndex == -1)
    {
        return false;
    }
    
    UGatheringComponent* GatheringComp = GetGatheringComponent();
    if (!GatheringComp)
    {
        return false;
    }
    
    // 現在地を取得してGatheringComponentに設定
    UMovementService* MovementService = GetWorld()->GetGameInstance()->GetSubsystem<UMovementService>();
    if (MovementService)
    {
        FString CurrentLocation = MovementService->GetCharacterCurrentLocation(Character);
        UE_LOG(LogTemp, Warning, TEXT("⛏️ GatheringService: Setting team %d location to %s for gathering"), 
            TeamIndex, *CurrentLocation);
        
        // TeamTargetLocationsを設定
        GatheringComp->SetTeamTargetLocation(TeamIndex, CurrentLocation);
    }
    
    // 既存のProcessTeamGatheringWithTargetロジックを使用
    GatheringComp->ProcessTeamGatheringWithTarget(TeamIndex, ItemType);
    return true;
}

TArray<FString> UGatheringService::GetExistingGatherableItems(const FString& Location)
{
    TArray<FString> Items;
    
    // LocationDataTableManagerから採集可能アイテムを取得
    ULocationDataTableManager* LocationManager = GetWorld() ? 
        GetWorld()->GetGameInstance()->GetSubsystem<ULocationDataTableManager>() : nullptr;
    
    if (!LocationManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("⛏️ GatheringService: LocationDataTableManager not found"));
        return Items;
    }
    
    FLocationDataRow LocationData;
    if (LocationManager->GetLocationData(Location, LocationData))
    {
        LocationData.GetGatherableItemIds(Items);
    }
    
    return Items;
}

int32 UGatheringService::GetCharacterTeamIndex(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return -1;
    }
    
    UTeamComponent* TeamComp = GetTeamComponent();
    if (!TeamComp)
    {
        return -1;
    }
    
    // キャラクターが所属するチームを検索
    for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
    {
        FTeam Team = TeamComp->GetTeam(i);
        if (Team.Members.Contains(Character))
        {
            return i;
        }
    }
    
    return -1;
}

float UGatheringService::CalculateBaseGatheringPower(AC_IdleCharacter* Character)
{
    if (!IsValid(Character))
    {
        return 1.0f;
    }
    
    // CharacterStatusComponentから採集能力を取得
    UCharacterStatusComponent* StatusComp = Character->GetStatusComponent();
    if (!StatusComp)
    {
        return 1.0f;
    }
    
    // 派生ステータスから採集能力を取得
    FDerivedStats DerivedStats = StatusComp->GetDerivedStats();
    return DerivedStats.GatheringPower;
}

float UGatheringService::ApplyItemTypeModifier(float BaseEfficiency, const FString& ItemType)
{
    // アイテムタイプ別の効率修正
    if (ItemType == TEXT("wood"))
    {
        return BaseEfficiency * 1.0f; // 標準
    }
    else if (ItemType == TEXT("stone"))
    {
        return BaseEfficiency * 0.8f; // やや困難
    }
    else if (ItemType == TEXT("iron"))
    {
        return BaseEfficiency * 0.6f; // 困難
    }
    
    return BaseEfficiency; // デフォルト
}

float UGatheringService::ApplyLocationModifier(float BaseEfficiency, const FString& Location)
{
    // 場所別の効率修正
    if (Location == TEXT("base"))
    {
        return BaseEfficiency * 1.2f; // 拠点は効率が良い
    }
    else if (Location == TEXT("plains"))
    {
        return BaseEfficiency * 1.0f; // 標準
    }
    
    return BaseEfficiency * 0.9f; // 未知の場所は効率が悪い
}

UGatheringComponent* UGatheringService::GetGatheringComponent()
{
    // PlayerControllerからGatheringComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return nullptr;
    }
    
    return PlayerController->FindComponentByClass<UGatheringComponent>();
}

UTeamComponent* UGatheringService::GetTeamComponent()
{
    // PlayerControllerからTeamComponentを取得
    AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    
    if (!PlayerController)
    {
        return nullptr;
    }
    
    return PlayerController->FindComponentByClass<UTeamComponent>();
}

bool UGatheringService::AreReferencesValid()
{
    return IsValid(GetGatheringComponent()) && IsValid(GetTeamComponent());
}

void UGatheringService::ResetStatistics()
{
    TotalGatheringRequests = 0;
    SuccessfulGatherings = 0;
    TotalGatheringTime = 0.0f;
    TotalEfficiency = 0.0f;
}

void UGatheringService::UpdateEfficiencyCache()
{
    if (!IsEfficiencyCacheValid())
    {
        EfficiencyCache.Empty();
        LastEfficiencyCacheUpdate = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    }
}

bool UGatheringService::IsEfficiencyCacheValid() const
{
    if (!GetWorld())
    {
        return false;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    return (CurrentTime - LastEfficiencyCacheUpdate) < EfficiencyCacheValidDuration;
}

void UGatheringService::InitializeItemLocationData()
{
    // アイテム希少度の初期化
    ItemRarityMap.Empty();
    ItemRarityMap.Add(TEXT("wood"), 1);      // 一般的
    ItemRarityMap.Add(TEXT("stone"), 2);     // やや希少
    ItemRarityMap.Add(TEXT("iron"), 4);      // 希少
    ItemRarityMap.Add(TEXT("food"), 1);      // 一般的
    ItemRarityMap.Add(TEXT("ingredient"), 2); // やや希少
    
    // 場所難易度の初期化
    LocationDifficultyMap.Empty();
    LocationDifficultyMap.Add(TEXT("base"), 1);   // 簡単
    LocationDifficultyMap.Add(TEXT("plains"), 2); // 普通
    
    UE_LOG(LogTemp, Log, TEXT("⛏️ GatheringService: Item and location data initialized"));
}