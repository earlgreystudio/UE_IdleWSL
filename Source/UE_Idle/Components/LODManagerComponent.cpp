#include "LODManagerComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"

ULODManagerComponent::ULODManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // デフォルト設定
    LODUpdateInterval = 0.5f;
    bMobileOptimizationMode = true;
    
    UE_LOG(LogTemp, Log, TEXT("LODManagerComponent: Created with mobile optimization mode %s"), 
        bMobileOptimizationMode ? TEXT("enabled") : TEXT("disabled"));
}

void ULODManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // LOD更新開始
    StartLODUpdates();
    
    UE_LOG(LogTemp, Log, TEXT("LODManagerComponent: Started LOD management system"));
}

void ULODManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // LOD更新停止
    StopLODUpdates();
    
    Super::EndPlay(EndPlayReason);
    
    UE_LOG(LogTemp, Log, TEXT("LODManagerComponent: Stopped LOD management system"));
}

void ULODManagerComponent::RegisterLODTarget(AActor* Actor)
{
    if (!Actor)
    {
        UE_LOG(LogTemp, Warning, TEXT("LODManagerComponent::RegisterLODTarget - Actor is null"));
        return;
    }

    // 既に登録済みかチェック
    TWeakObjectPtr<AActor> ActorPtr(Actor);
    if (LODTargets.Contains(ActorPtr))
    {
        return;
    }

    // 登録
    LODTargets.Add(ActorPtr);
    
    // 初期LOD設定
    UpdateActorLOD(Actor);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("LODManagerComponent: Registered LOD target %s"), *Actor->GetName());
}

void ULODManagerComponent::UnregisterLODTarget(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    TWeakObjectPtr<AActor> ActorPtr(Actor);
    int32 RemovedCount = LODTargets.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakPtr)
    {
        return WeakPtr.Get() == Actor;
    });

    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("LODManagerComponent: Unregistered LOD target %s"), *Actor->GetName());
    }
}

void ULODManagerComponent::UpdateAllLODLevels()
{
    // 無効なアクターをクリーンアップ
    CleanupInvalidTargets();
    
    // 全アクターのLOD更新
    for (const TWeakObjectPtr<AActor>& WeakActor : LODTargets)
    {
        if (AActor* Actor = WeakActor.Get())
        {
            UpdateActorLOD(Actor);
        }
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("LODManagerComponent: Updated LOD for %d actors"), LODTargets.Num());
}

void ULODManagerComponent::StartLODUpdates()
{
    if (!GetWorld())
    {
        return;
    }

    // 既存タイマーをクリア
    if (LODUpdateTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(LODUpdateTimerHandle);
    }

    // 新しいタイマー設定
    GetWorld()->GetTimerManager().SetTimer(
        LODUpdateTimerHandle,
        this,
        &ULODManagerComponent::ProcessLODUpdate,
        LODUpdateInterval,
        true // ループ
    );
    
    UE_LOG(LogTemp, Log, TEXT("LODManagerComponent: Started LOD updates with interval %.2f seconds"), LODUpdateInterval);
}

void ULODManagerComponent::StopLODUpdates()
{
    if (GetWorld() && LODUpdateTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(LODUpdateTimerHandle);
        LODUpdateTimerHandle.Invalidate();
        
        UE_LOG(LogTemp, Log, TEXT("LODManagerComponent: Stopped LOD updates"));
    }
}

FVector ULODManagerComponent::GetViewerLocation() const
{
    // プレイヤーカメラの位置を取得
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
        {
            if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
            {
                return CameraManager->GetCameraLocation();
            }
        }
    }
    
    // フォールバック：ワールド原点
    return FVector::ZeroVector;
}

ELODLevel ULODManagerComponent::CalculateLODLevel(AActor* Actor) const
{
    if (!Actor)
    {
        return ELODLevel::Hidden;
    }

    FVector ViewerLocation = GetViewerLocation();
    FVector ActorLocation = Actor->GetActorLocation();
    float Distance = FVector::Dist(ViewerLocation, ActorLocation);

    return CalculateLODLevelFromDistance(Distance);
}

int32 ULODManagerComponent::GetRegisteredActorCount() const
{
    return LODTargets.Num();
}

void ULODManagerComponent::GetLODStatistics(int32& OutHigh, int32& OutMedium, int32& OutLow, int32& OutHidden) const
{
    OutHigh = OutMedium = OutLow = OutHidden = 0;

    for (const TWeakObjectPtr<AActor>& WeakActor : LODTargets)
    {
        if (AActor* Actor = WeakActor.Get())
        {
            ELODLevel LODLevel = CalculateLODLevel(Actor);
            switch (LODLevel)
            {
            case ELODLevel::High:
                OutHigh++;
                break;
            case ELODLevel::Medium:
                OutMedium++;
                break;
            case ELODLevel::Low:
                OutLow++;
                break;
            case ELODLevel::Hidden:
                OutHidden++;
                break;
            }
        }
    }
}

// === Private Implementation ===

void ULODManagerComponent::ProcessLODUpdate()
{
    UpdateAllLODLevels();
}

void ULODManagerComponent::UpdateActorLOD(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    // LODレベル計算
    ELODLevel NewLODLevel = CalculateLODLevel(Actor);
    float DistanceToViewer = FVector::Dist(GetViewerLocation(), Actor->GetActorLocation());

    // ILODTargetインターフェースをチェック
    if (Actor->Implements<ULODTarget>())
    {
        ILODTarget::Execute_OnLODLevelChanged(Actor, NewLODLevel, DistanceToViewer);
    }
    else
    {
        // デフォルトLOD処理
        ApplyDefaultLOD(Actor, NewLODLevel);
    }

    // Tick間隔調整
    AdjustActorTickInterval(Actor, NewLODLevel);
}

void ULODManagerComponent::ApplyDefaultLOD(AActor* Actor, ELODLevel LODLevel)
{
    if (!Actor)
    {
        return;
    }

    switch (LODLevel)
    {
    case ELODLevel::High:
        // 高品質：全て表示
        Actor->SetActorHiddenInGame(false);
        Actor->SetActorTickEnabled(true);
        break;

    case ELODLevel::Medium:
        // 中品質：表示は維持、Tick頻度低下
        Actor->SetActorHiddenInGame(false);
        Actor->SetActorTickEnabled(true);
        break;

    case ELODLevel::Low:
        // 低品質：簡易表示、Tick大幅減少
        Actor->SetActorHiddenInGame(false);
        Actor->SetActorTickEnabled(true);
        
        // メッシュ品質低下（可能な場合）
        if (UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>())
        {
            // LOD強制設定（実装例）
            MeshComp->SetForcedLodModel(2); // 低品質LOD
        }
        break;

    case ELODLevel::Hidden:
        // 非表示：完全に無効化
        Actor->SetActorHiddenInGame(true);
        Actor->SetActorTickEnabled(false);
        break;
    }
}

ELODLevel ULODManagerComponent::CalculateLODLevelFromDistance(float Distance) const
{
    if (Distance <= LODSettings.HighQualityDistance)
    {
        return ELODLevel::High;
    }
    else if (Distance <= LODSettings.MediumQualityDistance)
    {
        return ELODLevel::Medium;
    }
    else if (Distance <= LODSettings.LowQualityDistance)
    {
        return ELODLevel::Low;
    }
    else
    {
        return ELODLevel::Hidden;
    }
}

void ULODManagerComponent::CleanupInvalidTargets()
{
    LODTargets.RemoveAll([](const TWeakObjectPtr<AActor>& WeakPtr)
    {
        return !WeakPtr.IsValid();
    });
}

void ULODManagerComponent::AdjustActorTickInterval(AActor* Actor, ELODLevel LODLevel)
{
    if (!Actor || !bMobileOptimizationMode)
    {
        return;
    }

    float TickInterval = 0.0f;
    
    switch (LODLevel)
    {
    case ELODLevel::High:
        TickInterval = LODSettings.HighQualityTickInterval;
        break;
    case ELODLevel::Medium:
        TickInterval = LODSettings.MediumQualityTickInterval;
        break;
    case ELODLevel::Low:
        TickInterval = LODSettings.LowQualityTickInterval;
        break;
    case ELODLevel::Hidden:
        TickInterval = 0.0f; // Tickは既に無効化
        break;
    }

    // Tick間隔設定
    Actor->SetActorTickInterval(TickInterval);
}