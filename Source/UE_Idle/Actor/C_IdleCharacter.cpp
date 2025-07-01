// Fill out your copyright notice in the Description page of Project Settings.


#include "C_IdleCharacter.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/InventoryComponent.h"
#include "../Components/CharacterBrain.h"
#include "../Components/LocationMovementComponent.h"
#include "../Components/TaskManagerComponent.h"
#include "../Components/TeamComponent.h"
#include "../C_PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "../AI/IdleAIController.h"

// Sets default values
AC_IdleCharacter::AC_IdleCharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// APawn基本設定
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	
	// === UE標準コンポーネントの作成 ===
	
	// 移動コンポーネント（2D用）- 基本クラスを使用
	FloatingMovement = CreateDefaultSubobject<UPawnMovementComponent>(TEXT("FloatingMovement"));
	if (FloatingMovement)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("PawnMovementComponent created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create PawnMovementComponent"));
	}
	
	// 2D表示メッシュ
	IconMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IconMesh"));
	if (IconMesh)
	{
		// ルートコンポーネントに設定
		RootComponent = IconMesh;
		
		// シンプルな球メッシュをデフォルトとして使用
		// メッシュは後でBlueprintで設定する
		IconMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		IconMesh->SetCollisionResponseToAllChannels(ECR_Block);
		IconMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("IconMesh created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create IconMesh"));
	}
	
	// グリッド位置初期化
	CurrentGridPosition = FIntPoint(10, 10); // デフォルト位置
	TargetGridPosition = CurrentGridPosition;
	
	// === AIコントローラー設定 ===
	AIControllerClass = AIdleAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	UE_LOG(LogTemp, Warning, TEXT("AC_IdleCharacter: APawn constructor completed"));

	// コンポーネント作成（防御的チェック付き）
	StatusComponent = CreateDefaultSubobject<UCharacterStatusComponent>(TEXT("StatusComponent"));
	if (!StatusComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create StatusComponent"));
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("StatusComponent created successfully"));
	}
	
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	if (InventoryComponent)
	{
		// Set owner ID for character inventory
		InventoryComponent->OwnerId = TEXT("Character");
		UE_LOG(LogTemp, VeryVerbose, TEXT("InventoryComponent created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create InventoryComponent"));
	}

	// ===========================================
	// 自律的キャラクターシステムの初期化
	// ===========================================
	
	// CharacterBrainを作成
	MyBrain = CreateDefaultSubobject<UCharacterBrain>(TEXT("CharacterBrain"));
	if (!MyBrain)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create CharacterBrain"));
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("CharacterBrain created successfully"));
	}

	// デフォルト設定
	MyPersonality = ECharacterPersonality::Loyal; // デフォルトは忠実
	bAutonomousSystemEnabled = true; // 自律システムを有効にする
	bShowDebugInfo = false; // デフォルトではデバッグ情報は非表示
	
	// 初期状況とアクションの設定
	CurrentSituation = FCharacterSituation();
	PlannedAction = FCharacterAction();
}

// Called when the game starts or when spawned
void AC_IdleCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// デバッグログ追加
	UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter::BeginPlay - %s, InventoryComponent: %s"), 
		*GetName(), InventoryComponent ? TEXT("Valid") : TEXT("NULL"));
	
	// インベントリの装備変更イベントをステータスコンポーネントの再計算に接続
	if (InventoryComponent && StatusComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter: Binding equipment change events"));
		
		// Dynamic multicast delegatesはAddDynamicを使用
		InventoryComponent->OnItemEquipped.AddDynamic(this, &AC_IdleCharacter::HandleItemEquipped);
		InventoryComponent->OnItemUnequipped.AddDynamic(this, &AC_IdleCharacter::HandleItemUnequipped);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("C_IdleCharacter: Failed to bind equipment events - missing components"));
	}

	// ===========================================
	// 自律的システムの初期化
	// ===========================================
	
	if (bAutonomousSystemEnabled && MyBrain)
	{
		// CharacterBrainに必要な参照を設定
		AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
			UGameplayStatics::GetPlayerController(GetWorld(), 0));
		
		if (PlayerController)
		{
			UTaskManagerComponent* TaskManager = PlayerController->FindComponentByClass<UTaskManagerComponent>();
			UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
			ULocationMovementComponent* MovementComp = PlayerController->FindComponentByClass<ULocationMovementComponent>();
			
			if (TaskManager && TeamComp && MovementComp)
			{
				MyBrain->SetCharacterReference(this);
				MyBrain->InitializeReferences(TaskManager, TeamComp, MovementComp);
				MyBrain->SetPersonality(MyPersonality);
				
				UE_LOG(LogTemp, Warning, TEXT("🧠 Autonomous system initialized for character %s"), *CharacterName);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ Failed to initialize autonomous system - missing components"));
			}
		}
	}
}

// Called every frame
void AC_IdleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Equipment change handlers
void AC_IdleCharacter::HandleItemEquipped(const FString& ItemId, EEquipmentSlot Slot)
{
	UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter: Item equipped - %s in slot %d"), *ItemId, (int32)Slot);
	if (StatusComponent)
	{
		StatusComponent->OnEquipmentChanged();
	}
}

void AC_IdleCharacter::HandleItemUnequipped(const FString& ItemId, EEquipmentSlot Slot)
{
	UE_LOG(LogTemp, Warning, TEXT("C_IdleCharacter: Item unequipped - %s from slot %d"), *ItemId, (int32)Slot);
	if (StatusComponent)
	{
		StatusComponent->OnEquipmentChanged();
	}
}

// IIdleCharacterInterface Implementation
FString AC_IdleCharacter::GetCharacterName_Implementation()
{
	return CharacterName;
}



bool AC_IdleCharacter::IsActive_Implementation()
{
	return bIsActive;
}

UCharacterStatusComponent* AC_IdleCharacter::GetStatusComponent() const
{
	if (!StatusComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("GetStatusComponent: StatusComponent is null for character %s"), 
			   CharacterName.IsEmpty() ? TEXT("Unknown") : *CharacterName);
		return nullptr;
	}
	else if (!IsValid(StatusComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("GetStatusComponent: StatusComponent is invalid for character %s"), 
			   CharacterName.IsEmpty() ? TEXT("Unknown") : *CharacterName);
		return nullptr;
	}
	return StatusComponent;
}

UCharacterStatusComponent* AC_IdleCharacter::GetCharacterStatusComponent_Implementation()
{
	return GetStatusComponent();  // 安全なGetStatusComponent()を使用
}

UInventoryComponent* AC_IdleCharacter::GetInventoryComponent() const
{
	// 詳細なデバッグログ
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("AC_IdleCharacter::GetInventoryComponent - InventoryComponent is NULL for %s"), *GetName());
		
		// const_castを使って、FindComponentByClassを試す（デバッグ用）
		AC_IdleCharacter* MutableThis = const_cast<AC_IdleCharacter*>(this);
		UInventoryComponent* FoundComp = MutableThis->FindComponentByClass<UInventoryComponent>();
		if (FoundComp)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("AC_IdleCharacter::GetInventoryComponent - Found via FindComponentByClass"));
			return FoundComp;
		}
	}
	
	return InventoryComponent;
}

UInventoryComponent* AC_IdleCharacter::GetCharacterInventoryComponent_Implementation()
{
	return InventoryComponent;
}

FCharacterTalent AC_IdleCharacter::GetCharacterTalent_Implementation()
{
	if (StatusComponent)
	{
		return StatusComponent->GetTalent();
	}
	return FCharacterTalent();
}

FDerivedStats AC_IdleCharacter::GetDerivedStats() const
{
	if (StatusComponent)
	{
		return StatusComponent->GetDerivedStats();
	}
	return FDerivedStats();
}

// ===========================================
// 自律的キャラクターシステムの実装
// ===========================================

void AC_IdleCharacter::OnTurnTick(int32 CurrentTurn)
{
	// 新しいBehavior Tree自律システムでの実装
	UE_LOG(LogTemp, Warning, TEXT("🧠 %s: OnTurnTick(Turn %d) - Behavior Tree autonomous processing"), 
		*CharacterName, CurrentTurn);

	// AIControllerからBehavior Treeを再開（毎ターン新しい判断）
	if (auto* AIController = GetController<AIdleAIController>())
	{
		AIController->RestartBehaviorTree();
		UE_LOG(LogTemp, Warning, TEXT("🧠🔄 %s: Behavior Tree restarted for fresh decision"), 
			*CharacterName);
	}
	else
	{
		// フォールバック：旧システムが残っている場合
		if (bAutonomousSystemEnabled && MyBrain)
		{
			UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ %s: Using fallback CharacterBrain system"), *CharacterName);
			
			// 自律的な判断プロセスを実行
			AnalyzeMySituation();
			ConsultMyTeam();
			DecideMyAction();
			ExecuteMyAction();

			// デバッグ情報表示
			if (bShowDebugInfo)
			{
				UE_LOG(LogTemp, Warning, TEXT("🧠📊 %s: Decided action %d (%s)"), 
					*CharacterName, 
					(int32)PlannedAction.ActionType, 
					*PlannedAction.ActionReason);
			}
		}
	}
}

void AC_IdleCharacter::SetPersonality(ECharacterPersonality NewPersonality)
{
	MyPersonality = NewPersonality;
	
	if (MyBrain)
	{
		MyBrain->SetPersonality(NewPersonality);
	}
	
	UE_LOG(LogTemp, Log, TEXT("🧠 %s: Personality set to %d"), 
		*CharacterName, (int32)NewPersonality);
}

void AC_IdleCharacter::AnalyzeMySituation()
{
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🔍 %s: Starting situation analysis"), *CharacterName);
	
	// 現在地の取得（LocationMovementComponentから直接）
	// GetCurrentLocationDirect()が存在しないため一時的に"base"に設定
	CurrentSituation.CurrentLocation = TEXT("base");

	// 体力・スタミナの取得
	if (StatusComponent)
	{
		FCharacterStatus Status = StatusComponent->GetStatus();
		CurrentSituation.CurrentHealth = Status.CurrentHealth;
		CurrentSituation.CurrentStamina = Status.CurrentStamina;
	}

	// チーム情報の取得
	AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	
	if (PlayerController)
	{
		UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
		if (TeamComp)
		{
			// 所属チームを検索
			CurrentSituation.MyTeamIndex = -1;
			for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
			{
				FTeam Team = TeamComp->GetTeam(i);
				if (Team.Members.Contains(this))
				{
					CurrentSituation.MyTeamIndex = i;
					CurrentSituation.TeamAssignedTask = Team.AssignedTask;
					CurrentSituation.Teammates = Team.Members;
					
					UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 %s: Found in Team %d, assigned task: %d"), 
						*CharacterName, i, (int32)Team.AssignedTask);
					break;
				}
			}
			
			if (CurrentSituation.MyTeamIndex == -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ %s: Not assigned to any team!"), *CharacterName);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ %s: TeamComponent not found!"), *CharacterName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ %s: PlayerController not found!"), *CharacterName);
	}

	// 利用可能タスクの取得（TaskInformationService削除により一時的に無効化）
	// 必要に応じて別の方法で実装
	CurrentSituation.AvailableTasks.Empty();

	// 採集可能アイテムの取得（TaskManagerに移行）
	AC_PlayerController* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC)
	{
		if (UTaskManagerComponent* TaskManager = PC->FindComponentByClass<UTaskManagerComponent>())
		{
			CurrentSituation.GatherableItems = TaskManager->GetGatherableItemsAt(CurrentSituation.CurrentLocation);
		}
	}

	// 危険地域判定（簡易版）
	CurrentSituation.bDangerousArea = (CurrentSituation.CurrentLocation != TEXT("base"));

	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🔍 %s: Situation analyzed - Location: %s, Team: %d, Task: %d"), 
		*CharacterName, 
		*CurrentSituation.CurrentLocation, 
		CurrentSituation.MyTeamIndex, 
		(int32)CurrentSituation.TeamAssignedTask);
}

void AC_IdleCharacter::ConsultMyTeam()
{
	// Phase 2.3: チーム連携機能の強化実装
	// TeamComponentの新しい連携機能を活用
	
	if (CurrentSituation.MyTeamIndex == -1)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥 %s: Not in any team, skipping team consultation"), *CharacterName);
		return;
	}

	// PlayerControllerからTeamComponentを取得
	AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
		
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("🧠👥⚠️ %s: PlayerController not found"), *CharacterName);
		return;
	}

	UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
	if (!TeamComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("🧠👥⚠️ %s: TeamComponent not found"), *CharacterName);
		return;
	}

	// ========================================
	// 1. チーム戦略の取得と分析
	// ========================================
	
	FTeamStrategy TeamStrategy = TeamComp->GetTeamStrategy(CurrentSituation.MyTeamIndex);
	
	// 取得した戦略を状況に反映
	CurrentSituation.RecommendedTaskType = TeamStrategy.RecommendedTaskType;
	CurrentSituation.TeamRecommendedLocation = TeamStrategy.RecommendedLocation;
	CurrentSituation.TeamRecommendedItem = TeamStrategy.RecommendedTargetItem;
	CurrentSituation.TeamStrategyReason = TeamStrategy.StrategyReason;
	
	UE_LOG(LogTemp, Verbose, TEXT("🧠👥📋 %s: Team strategy - Task: %d, Location: %s, Item: %s"), 
		*CharacterName, 
		(int32)TeamStrategy.RecommendedTaskType,
		*TeamStrategy.RecommendedLocation,
		*TeamStrategy.RecommendedTargetItem);

	// ========================================
	// 2. 詳細なチーム情報の取得
	// ========================================
	
	FTeamInfo DetailedTeamInfo = TeamComp->GetTeamInfoForCharacter(this);
	
	// チーム情報を状況に反映
	CurrentSituation.TeamActionState = DetailedTeamInfo.ActionState;
	CurrentSituation.ActiveTeammates = DetailedTeamInfo.ActiveMembers;
	CurrentSituation.TotalTeammates = DetailedTeamInfo.TotalMembers;
	CurrentSituation.CurrentTeamTarget = DetailedTeamInfo.CurrentTargetLocation;
	CurrentSituation.bTeamNeedsCoordination = DetailedTeamInfo.bNeedsCoordination;
	CurrentSituation.TeamCoordinationMessage = DetailedTeamInfo.CoordinationMessage;
	
	UE_LOG(LogTemp, Verbose, TEXT("🧠👥📊 %s: Team info - Members: %d/%d, State: %d, Target: %s"), 
		*CharacterName, 
		DetailedTeamInfo.ActiveMembers,
		DetailedTeamInfo.TotalMembers,
		(int32)DetailedTeamInfo.ActionState,
		*DetailedTeamInfo.CurrentTargetLocation);

	// ========================================
	// 3. チーム連携が必要な場合の処理
	// ========================================
	
	if (CurrentSituation.bTeamNeedsCoordination)
	{
		UE_LOG(LogTemp, Log, TEXT("🧠👥🤝 %s: Team coordination needed - %s"), 
			*CharacterName, 
			*CurrentSituation.TeamCoordinationMessage);
			
		// 連携が必要な場合は、後でDecideMyAction()での行動決定時に
		// CoordinateWithTeammates()を呼び出すフラグを設定
		CurrentSituation.bShouldCoordinateAction = true;
	}
	else
	{
		CurrentSituation.bShouldCoordinateAction = false;
	}

	// ========================================
	// 4. チーム状況の総合判断
	// ========================================
	
	// チームの効率性を判断
	float TeamEfficiency = (float)DetailedTeamInfo.ActiveMembers / (float)FMath::Max(1, DetailedTeamInfo.TotalMembers);
	CurrentSituation.TeamEfficiency = TeamEfficiency;
	
	// チーム戦略の優先度を考慮
	CurrentSituation.bShouldFollowTeamStrategy = (TeamStrategy.StrategyPriority >= 3); // 優先度3以上なら従う
	
	// 性格に基づく協調性の判定
	bool bPersonalitySupportsTeamwork = (MyPersonality == ECharacterPersonality::Loyal || 
	                                   MyPersonality == ECharacterPersonality::Defensive);
	CurrentSituation.bPersonallyCooperative = bPersonalitySupportsTeamwork;

	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥✅ %s: Team consultation completed - Strategy priority: %d, Should coordinate: %s, Team efficiency: %.2f"), 
		*CharacterName, 
		TeamStrategy.StrategyPriority,
		CurrentSituation.bShouldCoordinateAction ? TEXT("Yes") : TEXT("No"),
		TeamEfficiency);
}

void AC_IdleCharacter::DecideMyAction()
{
	if (!MyBrain)
	{
		UE_LOG(LogTemp, Error, TEXT("🧠❌ %s: MyBrain is null - using default wait action"), *CharacterName);
		PlannedAction = FCharacterAction(); // デフォルトアクション（待機）
		PlannedAction.ActionReason = TEXT("No brain available");
		return;
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🎯 %s: Brain available, analyzing situation for decision"), *CharacterName);

	// ========================================
	// Phase 2.3: チーム連携を考慮した行動決定
	// ========================================

	// 1. CharacterBrainに基本的な判断を委譲
	FCharacterAction InitialAction = MyBrain->DecideOptimalAction(CurrentSituation);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🎯 %s: Initial action decided - %d (%s)"), 
		*CharacterName, 
		(int32)InitialAction.ActionType, 
		*InitialAction.ActionReason);

	// 2. チーム連携が必要な場合の調整処理
	if (CurrentSituation.bShouldCoordinateAction && CurrentSituation.MyTeamIndex != -1)
	{
		// PlayerControllerからTeamComponentを取得
		AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
			UGameplayStatics::GetPlayerController(GetWorld(), 0));
			
		if (PlayerController)
		{
			UTeamComponent* TeamComp = PlayerController->FindComponentByClass<UTeamComponent>();
			if (TeamComp)
			{
				// チームメンバーとの行動調整を実行
				bool bActionApproved = TeamComp->CoordinateWithTeammates(this, InitialAction);
				
				if (bActionApproved)
				{
					// 行動が承認された場合はそのまま採用
					PlannedAction = InitialAction;
					PlannedAction.ActionReason += TEXT(" (Team coordinated)");
					
					UE_LOG(LogTemp, Verbose, TEXT("🧠👥✅ %s: Action approved by team coordination"), 
						*CharacterName);
				}
				else
				{
					// 行動が調整された場合は代替案を検討
					UE_LOG(LogTemp, Verbose, TEXT("🧠👥🔄 %s: Action adjustment needed by team"), 
						*CharacterName);
					
					// 代替行動を決定（チーム戦略に基づく）
					FCharacterAction AdjustedAction = DetermineAdjustedAction();
					PlannedAction = AdjustedAction;
				}
			}
			else
			{
				// TeamComponentが見つからない場合は初期行動をそのまま使用
				PlannedAction = InitialAction;
				PlannedAction.ActionReason += TEXT(" (No team coordination available)");
			}
		}
		else
		{
			// PlayerControllerが見つからない場合
			PlannedAction = InitialAction;
			PlannedAction.ActionReason += TEXT(" (No player controller)");
		}
	}
	else
	{
		// チーム連携が不要な場合は初期行動をそのまま使用
		PlannedAction = InitialAction;
		
		// チーム戦略を尊重すべき場合の追加判定
		if (CurrentSituation.bShouldFollowTeamStrategy && 
		    CurrentSituation.bPersonallyCooperative)
		{
			// チーム戦略に沿った行動への調整を検討
			FCharacterAction StrategyAlignedAction = AlignActionWithTeamStrategy(InitialAction);
			if (StrategyAlignedAction.ActionType != ECharacterActionType::Wait)
			{
				PlannedAction = StrategyAlignedAction;
			}
		}
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🎯✅ %s: Final action decided - %d (%s)"), 
		*CharacterName, 
		(int32)PlannedAction.ActionType, 
		*PlannedAction.ActionReason);
}

// ========================================
// Phase 2.3: チーム連携用ヘルパーメソッド
// ========================================

FCharacterAction AC_IdleCharacter::DetermineAdjustedAction()
{
	// チーム調整が必要な場合の代替行動決定
	FCharacterAction AdjustedAction;
	
	// チーム戦略に基づく行動を優先
	if (CurrentSituation.RecommendedTaskType != ETaskType::Idle)
	{
		switch (CurrentSituation.RecommendedTaskType)
		{
			case ETaskType::Gathering:
				AdjustedAction.ActionType = ECharacterActionType::GatherResources;
				AdjustedAction.TargetItem = CurrentSituation.TeamRecommendedItem;
				AdjustedAction.TargetLocation = CurrentSituation.TeamRecommendedLocation;
				AdjustedAction.ActionReason = TEXT("Team strategy: Gathering");
				break;
				
			case ETaskType::Adventure:
				AdjustedAction.ActionType = ECharacterActionType::AttackEnemy;
				AdjustedAction.TargetLocation = CurrentSituation.TeamRecommendedLocation;
				AdjustedAction.ActionReason = TEXT("Team strategy: Adventure");
				break;
				
			default:
				// その他のタスクタイプの場合は待機
				AdjustedAction.ActionType = ECharacterActionType::Wait;
				AdjustedAction.ActionReason = TEXT("Team coordination: Waiting for assignment");
				break;
		}
	}
	else
	{
		// 推奨タスクがない場合は待機
		AdjustedAction.ActionType = ECharacterActionType::Wait;
		AdjustedAction.ActionReason = TEXT("Team coordination: No specific task assigned");
	}
	
	UE_LOG(LogTemp, Verbose, TEXT("🧠👥🔄 %s: Adjusted action - %d (%s)"), 
		*CharacterName, 
		(int32)AdjustedAction.ActionType, 
		*AdjustedAction.ActionReason);
	
	return AdjustedAction;
}

FCharacterAction AC_IdleCharacter::AlignActionWithTeamStrategy(const FCharacterAction& OriginalAction)
{
	// 元の行動をチーム戦略に合わせて調整
	FCharacterAction AlignedAction = OriginalAction;
	
	// チーム推奨場所がある場合の調整
	if (!CurrentSituation.TeamRecommendedLocation.IsEmpty() && 
	    CurrentSituation.TeamRecommendedLocation != CurrentSituation.CurrentLocation)
	{
		// 推奨場所への移動を優先
		if (OriginalAction.ActionType == ECharacterActionType::GatherResources ||
		    OriginalAction.ActionType == ECharacterActionType::AttackEnemy)
		{
			AlignedAction.TargetLocation = CurrentSituation.TeamRecommendedLocation;
			AlignedAction.ActionReason += TEXT(" (Aligned with team location)");
		}
	}
	
	// チーム推奨アイテムがある場合の調整
	if (!CurrentSituation.TeamRecommendedItem.IsEmpty() && 
	    OriginalAction.ActionType == ECharacterActionType::GatherResources)
	{
		AlignedAction.TargetItem = CurrentSituation.TeamRecommendedItem;
		AlignedAction.ActionReason += TEXT(" (Aligned with team target item)");
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠👥📋 %s: Action aligned with team strategy"), *CharacterName);
	
	return AlignedAction;
}

void AC_IdleCharacter::ExecuteMyAction()
{
	if (PlannedAction.ActionType == ECharacterActionType::Wait)
	{
		// 待機の場合は何もしない
		UE_LOG(LogTemp, VeryVerbose, TEXT("🧠💤 %s: Executing wait action"), *CharacterName);
		return;
	}

	// 🔄 継続的な行動管理：移動や作業の進行状況をチェック
	CheckAndUpdateActionProgress();

	// 各種サービスを通じて行動を実行
	switch (PlannedAction.ActionType)
	{
		case ECharacterActionType::MoveToLocation:
			ExecuteMovementAction();
			break;
			
		case ECharacterActionType::GatherResources:
			ExecuteGatheringAction();
			break;
			
		case ECharacterActionType::AttackEnemy:
			ExecuteCombatAction();
			break;
			
		case ECharacterActionType::ReturnToBase:
			ExecuteReturnAction();
			break;
			
		case ECharacterActionType::UnloadItems:
			ExecuteUnloadAction();
			break;
			
		default:
			UE_LOG(LogTemp, Warning, TEXT("🧠⚠️ %s: Unhandled action type %d"), 
				*CharacterName, (int32)PlannedAction.ActionType);
			break;
	}
}

void AC_IdleCharacter::CheckAndUpdateActionProgress()
{
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🔄 %s: Checking action progress for %d"), 
		*CharacterName, (int32)PlannedAction.ActionType);
	
	// 移動アクションの場合：移動完了をチェック
	if (PlannedAction.ActionType == ECharacterActionType::MoveToLocation)
	{
		if (CurrentSituation.MyTeamIndex != -1)
		{
			bool bMovementCompleted = CheckMovementProgressDirect();
			if (bMovementCompleted)
			{
				UE_LOG(LogTemp, Warning, TEXT("🧠✅ %s: Movement completed! Analyzing new situation..."), *CharacterName);
				
				// 移動完了：状況を再分析して次の行動を決定
				AnalyzeMySituation();
				ConsultMyTeam(); 
				DecideMyAction();
				
				UE_LOG(LogTemp, Warning, TEXT("🧠🔄 %s: New action after movement: %d (%s)"), 
					*CharacterName, (int32)PlannedAction.ActionType, *PlannedAction.ActionReason);
			}
			else
			{
				UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🚶 %s: Still moving... waiting for completion"), *CharacterName);
			}
		}
	}
	
	// 荷下ろしアクションの場合：ExecuteMyAction()で実際に実行されるため、ここでは何もしない
	// (即座に完了判定すると、実際の荷下ろし前に次のアクションを決定してしまい無限ループになる)
	
	// 採集アクションの場合：採集完了をチェック（将来実装）
	// 戦闘アクションの場合：戦闘完了をチェック（将来実装）
}

void AC_IdleCharacter::ExecuteMovementAction()
{
	bool bSuccess = MoveToLocationDirect(PlannedAction.TargetLocation);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠🚶 %s: Movement to %s %s"), 
		*CharacterName, 
		*PlannedAction.TargetLocation, 
		bSuccess ? TEXT("started") : TEXT("failed"));
}

void AC_IdleCharacter::ExecuteGatheringAction()
{
	// TaskManagerに移行済みの採集実行
	AC_PlayerController* PC = Cast<AC_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("🧠❌ %s: PlayerController not available"), *CharacterName);
		return;
	}
	
	UTaskManagerComponent* TaskManager = PC->FindComponentByClass<UTaskManagerComponent>();
	if (!TaskManager)
	{
		UE_LOG(LogTemp, Error, TEXT("🧠❌ %s: TaskManager not available"), *CharacterName);
		return;
	}
	
	// チームインデックスを取得
	UTeamComponent* TeamComp = PC->FindComponentByClass<UTeamComponent>();
	int32 MyTeamIndex = -1;
	if (TeamComp)
	{
		for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
		{
			FTeam Team = TeamComp->GetTeam(i);
			if (Team.Members.Contains(this))
			{
				MyTeamIndex = i;
				break;
			}
		}
	}
	
	if (MyTeamIndex == -1)
	{
		UE_LOG(LogTemp, Error, TEXT("🧠❌ %s: Could not find team index"), *CharacterName);
		return;
	}
	
	// 現在地を取得
	FString CurrentLocation = GetCurrentLocationDirect();
	
	bool bSuccess = TaskManager->ExecuteGathering(MyTeamIndex, PlannedAction.TargetItem, CurrentLocation);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠⛏️ %s: Gathering %s %s"), 
		*CharacterName, 
		*PlannedAction.TargetItem, 
		bSuccess ? TEXT("started") : TEXT("failed"));
}

void AC_IdleCharacter::ExecuteCombatAction()
{
	// 戦闘機能は未実装
	UE_LOG(LogTemp, Warning, TEXT("🧠⚔️ %s: Combat action not yet implemented"), *CharacterName);
	bool bSuccess = false;
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("🧠⚔️ %s: Combat at %s %s"), 
		*CharacterName, 
		*PlannedAction.TargetLocation, 
		bSuccess ? TEXT("started") : TEXT("failed"));
}

void AC_IdleCharacter::ExecuteReturnAction()
{
	// 拠点帰還は移動アクションと同じ
	FCharacterAction ReturnAction = PlannedAction;
	ReturnAction.ActionType = ECharacterActionType::MoveToLocation;
	ReturnAction.TargetLocation = TEXT("base");
	
	PlannedAction = ReturnAction;
	ExecuteMovementAction();
}

void AC_IdleCharacter::ExecuteUnloadAction()
{
	FString CharName = GetName();
	UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: Executing unload action"), *CharName);
	
	// PlayerControllerを取得して荷下ろし
	AC_PlayerController* PlayerController = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("🧠❌ %s: PlayerController not found for unload"), *CharName);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: PlayerController found, checking inventory"), *CharName);
	
	// InventoryComponentを取得（既存のメンバ変数とFindComponentの両方を試す）
	UInventoryComponent* MyInventory = InventoryComponent;
	if (!MyInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: InventoryComponent member is null, trying FindComponentByClass"), *CharName);
		MyInventory = FindComponentByClass<UInventoryComponent>();
	}
	
	if (!MyInventory)
	{
		UE_LOG(LogTemp, Error, TEXT("🧠❌ %s: InventoryComponent not found"), *CharName);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: InventoryComponent found, getting items"), *CharName);
	
	TMap<FString, int32> AllItems = MyInventory->GetAllItems();
	int32 TransferredCount = 0;
	
	UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: Found %d different item types in inventory"), 
		*CharName, AllItems.Num());
	
	// 全アイテムを転送
	for (const auto& ItemPair : AllItems)
	{
		const FString& ItemId = ItemPair.Key;
		int32 Quantity = ItemPair.Value;
		
		UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: Processing item %s x %d"), 
			*CharName, *ItemId, Quantity);
		
		// PlayerControllerのAddItemToStorageを直接呼び出し
		PlayerController->AddItemToStorage_Implementation(ItemId, Quantity);
		TransferredCount += Quantity;
		UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: Transferred %d x %s to storage"), 
			*CharName, Quantity, *ItemId);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: Total items to transfer: %d"), 
		*CharName, TransferredCount);
	
	// インベントリから転送したアイテムを削除
	if (TransferredCount > 0)
	{
		// 転送済みアイテムを削除
		for (const auto& ItemPair : AllItems)
		{
			MyInventory->RemoveItem(ItemPair.Key, ItemPair.Value);
		}
		UE_LOG(LogTemp, Warning, TEXT("🧠✅ %s: Unloaded %d items to storage"), 
			*CharName, TransferredCount);
		
		// 荷下ろし完了後：状況を再分析して次の行動を決定
		UE_LOG(LogTemp, Warning, TEXT("🧠✅ %s: Unload completed! Analyzing new situation..."), *CharName);
		AnalyzeMySituation();
		ConsultMyTeam(); 
		DecideMyAction();
		UE_LOG(LogTemp, Warning, TEXT("🧠🔄 %s: New action after unload: %d (%s)"), 
			*CharName, (int32)PlannedAction.ActionType, *PlannedAction.ActionReason);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🧠📦 %s: No items to unload"), *CharName);
	}
}

// ===========================================
// MovementService削除に伴うヘルパー関数
// ===========================================

FString AC_IdleCharacter::GetCurrentLocationDirect()
{
	// PlayerControllerからLocationMovementComponentを取得
	AC_PlayerController* PC = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		return TEXT("base"); // デフォルト
	}
	
	ULocationMovementComponent* MovementComp = PC->FindComponentByClass<ULocationMovementComponent>();
	if (!MovementComp)
	{
		return TEXT("base"); // デフォルト
	}
	
	// チームインデックスを取得
	int32 MyTeamIndex = GetTeamIndex();
	if (MyTeamIndex == -1)
	{
		return TEXT("base"); // デフォルト
	}
	
	// LocationMovementComponentから移動情報を取得して現在地を判定
	FMovementInfo MovementInfo = MovementComp->GetMovementInfo(MyTeamIndex);
	
	// 移動中でない場合は、移動状態に基づいて現在地を判定
	if (MovementInfo.State == EMovementState::Stationary)
	{
		// 現在距離が0なら拠点
		float CurrentDistance = MovementComp->GetCurrentDistanceFromBase(MyTeamIndex);
		if (CurrentDistance <= 0.1f)
		{
			return TEXT("base");
		}
		else
		{
			// 距離に基づいて場所を推定（簡易実装）
			return TEXT("unknown_location");
		}
	}
	else if (MovementInfo.State == EMovementState::MovingToBase)
	{
		return MovementInfo.FromLocation; // 移動元
	}
	else if (MovementInfo.State == EMovementState::MovingToDestination)
	{
		return MovementInfo.FromLocation; // 移動元
	}
	else if (MovementInfo.State == EMovementState::Arrived)
	{
		return MovementInfo.ToLocation; // 到着地
	}
	
	return TEXT("base"); // デフォルト
}

bool AC_IdleCharacter::CheckMovementProgressDirect()
{
	AC_PlayerController* PC = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		return false;
	}
	
	ULocationMovementComponent* MovementComp = PC->FindComponentByClass<ULocationMovementComponent>();
	if (!MovementComp)
	{
		return false;
	}
	
	int32 MyTeamIndex = GetTeamIndex();
	if (MyTeamIndex == -1)
	{
		return false;
	}
	
	// 移動状態をチェック
	EMovementState State = MovementComp->GetMovementState(MyTeamIndex);
	return (State == EMovementState::Stationary || State == EMovementState::Arrived);
}

bool AC_IdleCharacter::MoveToLocationDirect(const FString& TargetLocation)
{
	AC_PlayerController* PC = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		return false;
	}
	
	ULocationMovementComponent* MovementComp = PC->FindComponentByClass<ULocationMovementComponent>();
	if (!MovementComp)
	{
		return false;
	}
	
	int32 MyTeamIndex = GetTeamIndex();
	if (MyTeamIndex == -1)
	{
		return false;
	}
	
	// 現在地を取得
	FString CurrentLocation = GetCurrentLocationDirect();
	
	// 既に目的地にいる場合はスキップ
	if (CurrentLocation == TargetLocation)
	{
		return true;
	}
	
	// LocationMovementComponentで移動開始
	return MovementComp->StartMovement(MyTeamIndex, CurrentLocation, TargetLocation);
}

int32 AC_IdleCharacter::GetTeamIndex()
{
	AC_PlayerController* PC = Cast<AC_PlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		return -1;
	}
	
	UTeamComponent* TeamComp = PC->FindComponentByClass<UTeamComponent>();
	if (!TeamComp)
	{
		return -1;
	}
	
	// チームメンバーから自分のチームインデックスを検索
	for (int32 i = 0; i < TeamComp->GetTeamCount(); i++)
	{
		FTeam Team = TeamComp->GetTeam(i);
		if (Team.Members.Contains(this))
		{
			return i;
		}
	}
	
	return -1;
}


