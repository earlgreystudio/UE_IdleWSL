#include "IdleAIController.h"
#include "../Components/GridMapComponent.h"
#include "../Actor/C_IdleCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Engine/Engine.h"

AIdleAIController::AIdleAIController()
{
	// Behavior TreeとBlackboardコンポーネントを作成
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	
	// AI知覚コンポーネント（将来の拡張用）
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	
	// デフォルト設定
	bSetControlRotationFromPawnOrientation = false;
}

void AIdleAIController::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("IdleAIController::BeginPlay"));
}

void AIdleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	UE_LOG(LogTemp, Warning, TEXT("IdleAIController::OnPossess - %s"), 
		InPawn ? *InPawn->GetName() : TEXT("NULL"));
	
	if (!InPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("IdleAIController: Cannot possess NULL pawn"));
		return;
	}
	
	// グリッドマップコンポーネント取得
	GridMapRef = GetGridMapComponent();
	if (!GridMapRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("IdleAIController: GridMapComponent not found, will retry later"));
	}
	
	// Behavior Tree開始（少し遅延させる）
	GetWorld()->GetTimerManager().SetTimer(
		StartupTimerHandle,
		this,
		&AIdleAIController::StartBehaviorTree,
		0.5f,
		false
	);
}

void AIdleAIController::OnUnPossess()
{
	// タイマークリア
	GetWorld()->GetTimerManager().ClearTimer(TurnTimer);
	
	// Behavior Tree停止
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree();
	}
	
	Super::OnUnPossess();
	
	UE_LOG(LogTemp, Warning, TEXT("IdleAIController::OnUnPossess"));
}

UGridMapComponent* AIdleAIController::GetGridMapComponent() const
{
	// PlayerControllerからグリッドマップを取得
	if (auto* PC = Cast<APlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		return PC->FindComponentByClass<UGridMapComponent>();
	}
	
	// または、ワールド内のアクターから検索
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
	
	for (AActor* Actor : FoundActors)
	{
		if (UGridMapComponent* GridComp = Actor->FindComponentByClass<UGridMapComponent>())
		{
			return GridComp;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("IdleAIController: GridMapComponent not found in world"));
	return nullptr;
}

void AIdleAIController::RestartBehaviorTree()
{
	if (BehaviorTreeComponent && IdleCharacterBT)
	{
		// デバッグログ
		UE_LOG(LogTemp, VeryVerbose, TEXT("IdleAIController: Restarting Behavior Tree for %s"), 
			GetPawn() ? *GetPawn()->GetName() : TEXT("Unknown"));
		
		// Behavior Tree再開始（毎ターン新しい判断）
		BehaviorTreeComponent->RestartTree();
	}
}

void AIdleAIController::OnTurnTick()
{
	// 毎ターンの処理
	RestartBehaviorTree();
	
	// デバッグ情報
	if (GetPawn())
	{
		if (auto* IdleChar = Cast<AC_IdleCharacter>(GetPawn()))
		{
			FIntPoint CurrentPos = IdleChar->GetCurrentGridPosition();
			UE_LOG(LogTemp, VeryVerbose, TEXT("Turn Tick - %s at Grid(%d,%d)"), 
				*IdleChar->GetName(), CurrentPos.X, CurrentPos.Y);
		}
	}
}

void AIdleAIController::StartBehaviorTree()
{
	if (bIsInitialized)
	{
		return;
	}
	
	// グリッドマップ再取得（まだない場合）
	if (!GridMapRef)
	{
		GridMapRef = GetGridMapComponent();
	}
	
	// Blackboard初期化
	if (IdleCharacterBB && BlackboardComponent)
	{
		BlackboardComponent->InitializeBlackboard(*IdleCharacterBB);
		UE_LOG(LogTemp, Warning, TEXT("IdleAIController: Blackboard initialized"));
	}
	
	// Behavior Tree開始
	if (IdleCharacterBT && BehaviorTreeComponent)
	{
		bool bSuccess = RunBehaviorTree(IdleCharacterBT);
		UE_LOG(LogTemp, Warning, TEXT("IdleAIController: Behavior Tree started - %s"), 
			bSuccess ? TEXT("Success") : TEXT("Failed"));
		
		if (bSuccess)
		{
			// TimeManagerComponentが全キャラクターのOnTurnTick()を管理するため、
			// AIController独自のタイマーは不要
			bIsInitialized = true;
			UE_LOG(LogTemp, Warning, TEXT("IdleAIController: Behavior Tree ready (TimeManager will handle turn ticks)"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("IdleAIController: Missing BehaviorTree or BehaviorTreeComponent"));
		
		// 再試行タイマー
		GetWorld()->GetTimerManager().SetTimer(
			RetryTimerHandle,
			this,
			&AIdleAIController::StartBehaviorTree,
			2.0f,
			false
		);
	}
}