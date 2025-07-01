#include "GridMapComponent_Optimized.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

UGridMapComponent_Optimized::UGridMapComponent_Optimized()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // デフォルト設定
    ChunkSize = 10;
    MaxActiveChunks = 9;
    ChunkDeactivationTime = 30.0f;
    ViewerChunkCoord = FIntPoint::ZeroValue;
    
    UE_LOG(LogTemp, Log, TEXT("GridMapComponent_Optimized: Created with chunk size %d"), ChunkSize);
}

void UGridMapComponent_Optimized::BeginPlay()
{
    Super::BeginPlay();
    
    // チャンク更新タイマー開始
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            ChunkUpdateTimerHandle,
            this,
            &UGridMapComponent_Optimized::ProcessChunkUpdate,
            1.0f, // 1秒間隔
            true
        );
        
        GetWorld()->GetTimerManager().SetTimer(
            ChunkCleanupTimerHandle,
            this,
            &UGridMapComponent_Optimized::ProcessChunkCleanup,
            10.0f, // 10秒間隔
            true
        );
    }
    
    UE_LOG(LogTemp, Log, TEXT("GridMapComponent_Optimized: Started chunk management system"));
}

void UGridMapComponent_Optimized::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // タイマークリア
    if (GetWorld())
    {
        if (ChunkUpdateTimerHandle.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(ChunkUpdateTimerHandle);
        }
        if (ChunkCleanupTimerHandle.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(ChunkCleanupTimerHandle);
        }
    }
    
    Super::EndPlay(EndPlayReason);
    
    UE_LOG(LogTemp, Log, TEXT("GridMapComponent_Optimized: Stopped chunk management system"));
}

void UGridMapComponent_Optimized::ActivateChunk(const FIntPoint& ChunkCoord)
{
    // チャンクが存在しない場合は初期化
    if (!DoesChunkExist(ChunkCoord))
    {
        InitializeChunk(ChunkCoord);
    }
    
    // アクティブリストに追加
    if (!ActiveChunks.Contains(ChunkCoord))
    {
        ActiveChunks.Add(ChunkCoord);
        
        // チャンクデータ更新
        if (FGridChunk* Chunk = Chunks.Find(ChunkCoord))
        {
            Chunk->bIsActive = true;
            Chunk->LastAccessTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("GridMapComponent_Optimized: Activated chunk (%d, %d)"), 
            ChunkCoord.X, ChunkCoord.Y);
    }
    
    // 最大アクティブチャンク数を超えた場合、古いチャンクを非アクティブ化
    if (ActiveChunks.Num() > MaxActiveChunks)
    {
        DeactivateOldChunks();
    }
}

void UGridMapComponent_Optimized::DeactivateChunk(const FIntPoint& ChunkCoord)
{
    if (ActiveChunks.Contains(ChunkCoord))
    {
        ActiveChunks.Remove(ChunkCoord);
        
        // チャンクデータ更新
        if (FGridChunk* Chunk = Chunks.Find(ChunkCoord))
        {
            Chunk->bIsActive = false;
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("GridMapComponent_Optimized: Deactivated chunk (%d, %d)"), 
            ChunkCoord.X, ChunkCoord.Y);
    }
}

void UGridMapComponent_Optimized::UpdateViewerPosition(const FVector& ViewerPosition)
{
    FIntPoint GridPos = WorldToGrid(ViewerPosition);
    FIntPoint NewViewerChunk = GetChunkCoord(GridPos);
    
    if (NewViewerChunk != ViewerChunkCoord)
    {
        ViewerChunkCoord = NewViewerChunk;
        UpdateVisibleChunks(ViewerPosition);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("GridMapComponent_Optimized: Viewer moved to chunk (%d, %d)"), 
            ViewerChunkCoord.X, ViewerChunkCoord.Y);
    }
}

void UGridMapComponent_Optimized::UpdateVisibleChunks(const FVector& ViewerPosition)
{
    UpdateViewerPosition(ViewerPosition);
    
    // 周辺チャンクをアクティブ化
    ActivateNeighboringChunks(ViewerChunkCoord, 1);
}

FIntPoint UGridMapComponent_Optimized::GetChunkCoord(const FIntPoint& GridPosition) const
{
    int32 ChunkX = FMath::FloorToInt(static_cast<float>(GridPosition.X) / ChunkSize);
    int32 ChunkY = FMath::FloorToInt(static_cast<float>(GridPosition.Y) / ChunkSize);
    return FIntPoint(ChunkX, ChunkY);
}

FVector UGridMapComponent_Optimized::GetChunkWorldCenter(const FIntPoint& ChunkCoord) const
{
    FIntPoint ChunkGridCenter = FIntPoint(
        ChunkCoord.X * ChunkSize + ChunkSize / 2,
        ChunkCoord.Y * ChunkSize + ChunkSize / 2
    );
    return GridToWorld(ChunkGridCenter);
}

FIntPoint UGridMapComponent_Optimized::GetLocalCoordInChunk(const FIntPoint& GridPosition) const
{
    int32 LocalX = GridPosition.X % ChunkSize;
    int32 LocalY = GridPosition.Y % ChunkSize;
    
    // 負の座標の処理
    if (LocalX < 0) LocalX += ChunkSize;
    if (LocalY < 0) LocalY += ChunkSize;
    
    return FIntPoint(LocalX, LocalY);
}

FGridCellData UGridMapComponent_Optimized::GetCellData(const FIntPoint& GridPos) const
{
    FIntPoint ChunkCoord = GetChunkCoord(GridPos);
    
    // チャンクが存在し、アクティブかチェック
    if (const FGridChunk* Chunk = Chunks.Find(ChunkCoord))
    {
        if (const FGridCellData* CellData = Chunk->CellData.Find(GridPos))
        {
            return *CellData;
        }
    }
    
    // デフォルトデータを返す
    return FGridCellData();
}

void UGridMapComponent_Optimized::SetCellDataOptimized(const FIntPoint& GridPos, const FGridCellData& CellData)
{
    FIntPoint ChunkCoord = GetChunkCoord(GridPos);
    
    // チャンクが存在しない場合は初期化
    if (!DoesChunkExist(ChunkCoord))
    {
        InitializeChunk(ChunkCoord);
    }
    
    // セルデータ設定
    if (FGridChunk* Chunk = Chunks.Find(ChunkCoord))
    {
        Chunk->CellData.FindOrAdd(GridPos) = CellData;
        Chunk->LastAccessTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    }
}

void UGridMapComponent_Optimized::RegisterActorToChunk(AActor* Actor, const FIntPoint& GridPosition)
{
    if (!Actor)
    {
        return;
    }
    
    FIntPoint ChunkCoord = GetChunkCoord(GridPosition);
    
    // チャンクが存在しない場合は初期化
    if (!DoesChunkExist(ChunkCoord))
    {
        InitializeChunk(ChunkCoord);
    }
    
    // アクター登録
    if (FGridChunk* Chunk = Chunks.Find(ChunkCoord))
    {
        TWeakObjectPtr<AActor> ActorPtr(Actor);
        if (!Chunk->ContainedActors.Contains(ActorPtr))
        {
            Chunk->ContainedActors.Add(ActorPtr);
            Chunk->LastAccessTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        }
    }
}

void UGridMapComponent_Optimized::UnregisterActorFromChunk(AActor* Actor, const FIntPoint& GridPosition)
{
    if (!Actor)
    {
        return;
    }
    
    FIntPoint ChunkCoord = GetChunkCoord(GridPosition);
    
    if (FGridChunk* Chunk = Chunks.Find(ChunkCoord))
    {
        TWeakObjectPtr<AActor> ActorPtr(Actor);
        Chunk->ContainedActors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakPtr)
        {
            return WeakPtr.Get() == Actor;
        });
    }
}

TArray<AActor*> UGridMapComponent_Optimized::GetActorsInChunk(const FIntPoint& ChunkCoord) const
{
    TArray<AActor*> Result;
    
    if (const FGridChunk* Chunk = Chunks.Find(ChunkCoord))
    {
        for (const TWeakObjectPtr<AActor>& WeakActor : Chunk->ContainedActors)
        {
            if (AActor* Actor = WeakActor.Get())
            {
                Result.Add(Actor);
            }
        }
    }
    
    return Result;
}

float UGridMapComponent_Optimized::GetEstimatedMemoryUsage() const
{
    float TotalMemory = 0.0f;
    
    // チャンクデータのメモリ使用量（概算）
    for (const auto& ChunkPair : Chunks)
    {
        const FGridChunk& Chunk = ChunkPair.Value;
        
        // 基本チャンクサイズ
        TotalMemory += sizeof(FGridChunk);
        
        // セルデータサイズ
        TotalMemory += Chunk.CellData.Num() * sizeof(FGridCellData);
        
        // アクター参照サイズ
        TotalMemory += Chunk.ContainedActors.Num() * sizeof(TWeakObjectPtr<AActor>);
    }
    
    // KB単位で返す
    return TotalMemory / 1024.0f;
}

void UGridMapComponent_Optimized::SetChunkBoundsVisible(bool bVisible)
{
    bShowChunkBounds = bVisible;
    DrawChunkBounds(bVisible);
}

// === Private Implementation ===

void UGridMapComponent_Optimized::ProcessChunkUpdate()
{
    // アクティブチャンクのアクセス時間更新
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    for (const FIntPoint& ChunkCoord : ActiveChunks)
    {
        if (FGridChunk* Chunk = Chunks.Find(ChunkCoord))
        {
            Chunk->LastAccessTime = CurrentTime;
        }
    }
}

void UGridMapComponent_Optimized::ProcessChunkCleanup()
{
    if (!GetWorld())
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    TArray<FIntPoint> ChunksToRemove;
    
    // 古い非アクティブチャンクを削除候補に追加
    for (auto& ChunkPair : Chunks)
    {
        const FIntPoint& ChunkCoord = ChunkPair.Key;
        FGridChunk& Chunk = ChunkPair.Value;
        
        // アクティブでなく、しばらくアクセスされていないチャンク
        if (!Chunk.bIsActive && (CurrentTime - Chunk.LastAccessTime) > ChunkDeactivationTime)
        {
            // アクターが残っていない場合のみ削除
            if (Chunk.ContainedActors.Num() == 0)
            {
                ChunksToRemove.Add(ChunkCoord);
            }
        }
    }
    
    // チャンク削除実行
    for (const FIntPoint& ChunkCoord : ChunksToRemove)
    {
        Chunks.Remove(ChunkCoord);
        UE_LOG(LogTemp, VeryVerbose, TEXT("GridMapComponent_Optimized: Cleaned up chunk (%d, %d)"), 
            ChunkCoord.X, ChunkCoord.Y);
    }
    
    if (ChunksToRemove.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("GridMapComponent_Optimized: Cleaned up %d old chunks"), ChunksToRemove.Num());
    }
}

void UGridMapComponent_Optimized::InitializeChunk(const FIntPoint& ChunkCoord)
{
    if (!DoesChunkExist(ChunkCoord))
    {
        FGridChunk NewChunk(ChunkCoord);
        Chunks.Add(ChunkCoord, NewChunk);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("GridMapComponent_Optimized: Initialized chunk (%d, %d)"), 
            ChunkCoord.X, ChunkCoord.Y);
    }
}

bool UGridMapComponent_Optimized::DoesChunkExist(const FIntPoint& ChunkCoord) const
{
    return Chunks.Contains(ChunkCoord);
}

void UGridMapComponent_Optimized::ActivateNeighboringChunks(const FIntPoint& CenterChunkCoord, int32 Radius)
{
    for (int32 dx = -Radius; dx <= Radius; dx++)
    {
        for (int32 dy = -Radius; dy <= Radius; dy++)
        {
            FIntPoint NeighborChunk = CenterChunkCoord + FIntPoint(dx, dy);
            ActivateChunk(NeighborChunk);
        }
    }
}

void UGridMapComponent_Optimized::DeactivateOldChunks()
{
    if (!GetWorld())
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    TArray<FIntPoint> ChunksToDeactivate;
    
    // アクセス時間が古いチャンクを探す
    for (const FIntPoint& ChunkCoord : ActiveChunks)
    {
        if (const FGridChunk* Chunk = Chunks.Find(ChunkCoord))
        {
            if ((CurrentTime - Chunk->LastAccessTime) > ChunkDeactivationTime / 2.0f)
            {
                ChunksToDeactivate.Add(ChunkCoord);
            }
        }
    }
    
    // 古いチャンクから順に非アクティブ化
    ChunksToDeactivate.Sort([this](const FIntPoint& A, const FIntPoint& B)
    {
        const FGridChunk* ChunkA = Chunks.Find(A);
        const FGridChunk* ChunkB = Chunks.Find(B);
        if (ChunkA && ChunkB)
        {
            return ChunkA->LastAccessTime < ChunkB->LastAccessTime;
        }
        return false;
    });
    
    // 必要数だけ非アクティブ化
    int32 DeactivateCount = FMath::Max(0, ActiveChunks.Num() - MaxActiveChunks);
    for (int32 i = 0; i < DeactivateCount && i < ChunksToDeactivate.Num(); i++)
    {
        DeactivateChunk(ChunksToDeactivate[i]);
    }
}

void UGridMapComponent_Optimized::DrawChunkBounds(bool bVisible)
{
    if (!GetWorld() || !bVisible)
    {
        return;
    }
    
    // デバッグ描画でチャンク境界を表示
    for (const auto& ChunkPair : Chunks)
    {
        const FIntPoint& ChunkCoord = ChunkPair.Key;
        const FGridChunk& Chunk = ChunkPair.Value;
        
        FVector ChunkCenter = GetChunkWorldCenter(ChunkCoord);
        FVector ChunkExtent = FVector(ChunkSize * CellSize / 2.0f, ChunkSize * CellSize / 2.0f, 100.0f);
        
        FColor Color = Chunk.bIsActive ? FColor::Green : FColor::Red;
        
        DrawDebugBox(GetWorld(), ChunkCenter, ChunkExtent, Color, false, 1.0f, 0, 5.0f);
    }
}