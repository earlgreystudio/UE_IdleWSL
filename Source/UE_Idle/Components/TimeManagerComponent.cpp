#include "TimeManagerComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "../C_PlayerController.h"
#include "TeamComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// ===========================================
// Phase 3: 簡素化されたTimeManagerComponent実装
// 唯一の責任：全キャラクターにターン開始通知を送ること
// ===========================================

UTimeManagerComponent::UTimeManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // 簡素化された初期化
    bTimeSystemActive = false;
    TimeUpdateInterval = 1.0f;  // 1秒 = 1ターン
    CurrentTurn = 0;
    bGamePaused = false;
    CurrentGameSpeed = "Normal";
    
    // ゲーム速度プリセット初期化
    GameSpeedPresets.Add("Slow", 2.0f);
    GameSpeedPresets.Add("Normal", 1.0f);
    GameSpeedPresets.Add("Fast", 0.5f);
    GameSpeedPresets.Add("Ultra", 0.1f);
    
    UE_LOG(LogTemp, Log, TEXT("🕐 Simplified TimeManagerComponent created with game speed control"));
}

void UTimeManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("🕐 Simplified TimeManagerComponent: BeginPlay - Ready for autonomous character system"));
}

void UTimeManagerComponent::BeginDestroy()
{
    // システム停止
    StopTimeSystem();
    
    UE_LOG(LogTemp, Log, TEXT("🕐 Simplified TimeManagerComponent: BeginDestroy - Clean shutdown"));
    
    Super::BeginDestroy();
}

// ===========================================
// Phase 3: 簡素化されたパブリックAPI実装
// ===========================================

void UTimeManagerComponent::StartTimeSystem()
{
    if (bTimeSystemActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("🕐 Time system is already active"));
        return;
    }

    bTimeSystemActive = true;
    SetupTimer();
    
    UE_LOG(LogTemp, Log, TEXT("🕐 Autonomous time system started - Turn interval: %.1f seconds"), TimeUpdateInterval);
}

void UTimeManagerComponent::StopTimeSystem()
{
    if (!bTimeSystemActive)
    {
        return;
    }

    bTimeSystemActive = false;
    ClearTimer();
    
    UE_LOG(LogTemp, Log, TEXT("🕐 Autonomous time system stopped at turn %d"), CurrentTurn);
}

void UTimeManagerComponent::ProcessTimeUpdate()
{
    // 実装計画書に記載された新しい設計に従う
    // 唯一の責任：ターン開始通知
    
    if (!bTimeSystemActive || bGamePaused)
    {
        return;
    }

    // ターン番号を進める
    CurrentTurn++;
    
    // ターン開始の目立つ区切り線を追加
    UE_LOG(LogTemp, Warning, TEXT("■■■■■■■■■■■■■■■■■■"));
    
    // UE_LOG(LogTemp, Verbose, TEXT("🕐⏰ Turn %d started - Notifying all autonomous characters"), CurrentTurn);
    
    // 🚨 CRITICAL FIX: チーム戦略の定期更新
    // 実装計画書：「既存機能の完全再現」を保証
    // UIからのタスク変更に加えて、状況変化に応じた戦略更新
    if (CurrentTurn % 10 == 0) // 10ターン毎に戦略を再評価
    {
        AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
            UGameplayStatics::GetPlayerController(GetWorld(), 0));
        if (PlayerController)
        {
            UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
            if (TeamComp)
            {
                TeamComp->ReevaluateAllTeamStrategies();
                UE_LOG(LogTemp, VeryVerbose, TEXT("🕐🎯 Turn %d: Team strategies reevaluated"), CurrentTurn);
            }
        }
    }
    
    // 🚨 UPDATED: グリッドベース移動システム統合
    // 自律的キャラクターが個別に移動を判断するため、TimeManagerは基本的な更新のみ
    AC_PlayerController* MovementPlayerController = Cast<AC_PlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0));
    if (MovementPlayerController)
    {
        UTeamComponent* TeamComp = MovementPlayerController->FindComponentByClass<UTeamComponent>();
        
        if (TeamComp)
        {
            // チーム状況の基本更新のみ実行
            // 実際の移動はBehavior Treeが各キャラクター個別に処理
            
            // UI更新は引き続き実行（チーム状況変化の反映）
            TeamComp->OnTeamsUpdated.Broadcast();
            UE_LOG(LogTemp, VeryVerbose, TEXT("🕐🎮 Turn %d: Team status updated for autonomous system"), 
                CurrentTurn);
        }
    }
    
    // 全キャラクターにターン開始を通知
    TArray<AActor*> AllCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_IdleCharacter::StaticClass(), AllCharacters);
    
    int32 NotifiedCharacters = 0;
    
    for (AActor* Actor : AllCharacters)
    {
        if (AC_IdleCharacter* Character = Cast<AC_IdleCharacter>(Actor))
        {
            // 各キャラクターの自律的処理を開始
            Character->OnTurnTick(CurrentTurn);
            NotifiedCharacters++;
        }
    }
    
    // UE_LOG(LogTemp, Verbose, TEXT("🕐✅ Turn %d completed - Notified %d characters"), 
    //     CurrentTurn, NotifiedCharacters);
    
    // それだけ！
    // 複雑なタスク処理、チーム管理、リソース監視などは
    // 自律的キャラクターシステムとサービス群が担当
}

// ===========================================
// Phase 3: 簡素化された内部実装
// ===========================================

void UTimeManagerComponent::SetupTimer()
{
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("🕐❌ Cannot setup timer - World is null"));
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        TimeUpdateTimerHandle,
        this,
        &UTimeManagerComponent::ProcessTimeUpdate,
        TimeUpdateInterval,
        true  // Loop
    );
    
    UE_LOG(LogTemp, Verbose, TEXT("🕐⏲️ Timer setup complete - Interval: %.1f seconds"), TimeUpdateInterval);
}

void UTimeManagerComponent::ClearTimer()
{
    if (GetWorld() && TimeUpdateTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(TimeUpdateTimerHandle);
        TimeUpdateTimerHandle.Invalidate();
        
        UE_LOG(LogTemp, Verbose, TEXT("🕐⏹️ Timer cleared"));
    }
}

// ===============================================
// ゲーム速度制御実装
// ===============================================

void UTimeManagerComponent::SetGameSpeed(const FString& SpeedName)
{
    if (const float* Speed = GameSpeedPresets.Find(SpeedName))
    {
        CurrentGameSpeed = SpeedName;
        SetCustomInterval(*Speed);
        
        UE_LOG(LogTemp, Log, TEXT("🕐⚡ Game speed set to %s (%.2f seconds per turn)"), 
            *SpeedName, *Speed);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🕐❌ Unknown game speed: %s"), *SpeedName);
    }
}

void UTimeManagerComponent::SetCustomInterval(float NewInterval)
{
    if (NewInterval < 0.1f || NewInterval > 10.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🕐❌ Invalid interval: %.2f (must be 0.1-10.0)"), NewInterval);
        return;
    }

    TimeUpdateInterval = NewInterval;
    
    // タイマーが動いている場合は再設定
    if (bTimeSystemActive)
    {
        StopTimeSystem();
        StartTimeSystem();
    }
    
    UE_LOG(LogTemp, Log, TEXT("🕐⚙️ Custom interval set to %.2f seconds"), NewInterval);
}

void UTimeManagerComponent::PauseGame()
{
    if (!bGamePaused)
    {
        bGamePaused = true;
        UE_LOG(LogTemp, Log, TEXT("🕐⏸️ Game paused at turn %d"), CurrentTurn);
    }
}

void UTimeManagerComponent::ResumeGame()
{
    if (bGamePaused)
    {
        bGamePaused = false;
        UE_LOG(LogTemp, Log, TEXT("🕐▶️ Game resumed at turn %d"), CurrentTurn);
    }
}