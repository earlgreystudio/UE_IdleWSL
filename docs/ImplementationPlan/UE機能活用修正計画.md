# UE機能活用修正計画（改訂版2）

## 概要

UE_IdleWSLプロジェクトをUnrealEngine標準機能を最大活用する設計に全面改修。グリッドベースの場所管理と2D表示を前提とし、アイドルゲームに最適化されたアーキテクチャへ移行。

## 🎯 修正の核心理念

### 設計方針
- **グリッドマップシステム**: 明確な場所管理と効率的な空間検索
- **属性システム維持**: 現在のCharacterStatusComponentを基本とし、必要に応じて拡張
- **Behavior Tree完全移行**: CharacterBrainをBTで置換
- **2D表示最適化**: 真上視点のシンプルな表現
- **ハイブリッドアプローチ**: UE標準機能と独自実装の最適なバランス

### 完全UE標準化のメリット
- **開発効率**: 実装済み機能の活用で開発高速化
- **保守性**: UE更新で自動的に機能改善
- **拡張性**: エディタツールによる非プログラマー対応
- **デバッグ**: 標準デバッグツールの活用
- **モバイル最適化**: 軽量な2D表示とグリッドベース処理

## 🏗️ アーキテクチャ全面改修

### 1. キャラクタークラス：APawn継承

```cpp
UCLASS()
class UE_IDLE_API AC_IdleCharacter : public APawn, public IIdleCharacterInterface
{
    GENERATED_BODY()
    
public:
    // === UE標準コンポーネント ===
    
    // 移動コンポーネント（2D移動用）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    class UFloatingPawnMovementComponent* FloatingMovement;
    
    // 2D表示コンポーネント（いずれかを選択）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
    class UPaperSpriteComponent* SpriteComponent; // Paper2D使用時
    
    // または
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
    class UStaticMeshComponent* IconMesh; // シンプルな3Dメッシュ使用時
    
    // === 既存コンポーネント（維持） ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
    class UCharacterStatusComponent* StatusComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
    class UInventoryComponent* InventoryComponent;
    
    // === グリッド位置管理 ===
    UPROPERTY(BlueprintReadOnly, Category = "Grid")
    FIntPoint CurrentGridPosition;
    
    UPROPERTY(BlueprintReadOnly, Category = "Grid")
    FIntPoint TargetGridPosition;
    
    // === AIコントローラークラス指定 ===
    AC_IdleCharacter()
    {
        AIControllerClass = AC_IdleAIController::StaticClass();
        AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
        
        // 2D移動制約
        bConstrainToPlane = true;
        SetPlaneConstraintNormal(FVector::UpVector);
    }
};
```

### 2. グリッドマップシステム

#### グリッドマップコンポーネント

```cpp
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_IDLE_API UGridMapComponent : public UActorComponent
{
    GENERATED_BODY()
    
public:
    // グリッド設定
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    int32 GridWidth = 20;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    int32 GridHeight = 20;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    float CellSize = 1000.0f; // 10m per cell
    
    // グリッドデータ
    UPROPERTY()
    TMap<FIntPoint, FGridCellData> GridData;
    
    // === 座標変換 ===
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GridToWorld(const FIntPoint& GridPos) const;
    
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FIntPoint WorldToGrid(const FVector& WorldPos) const;
    
    // === セル情報 ===
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FGridCellData GetCellData(const FIntPoint& GridPos) const;
    
    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<FIntPoint> GetAdjacentCells(const FIntPoint& GridPos) const;
    
    // === パス探索（A*） ===
    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<FIntPoint> FindPath(const FIntPoint& Start, const FIntPoint& Goal);
    
    // === 高速検索 ===
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FIntPoint FindNearestCellWithTag(const FIntPoint& Origin, FGameplayTag RequiredTag);
    
    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<FIntPoint> GetCellsWithTag(FGameplayTag LocationTag);
};

// グリッドセルデータ
USTRUCT(BlueprintType)
struct FGridCellData
{
    GENERATED_BODY()
    
    // 場所タイプ
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag LocationType; // Location.Base, Location.Forest, Location.Plains
    
    // 移動可能性
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsWalkable = true;
    
    // 移動コスト
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementCost = 1.0f;
    
    // 採集可能リソース
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGameplayTag> AvailableResources; // Resource.Wood, Resource.Stone等
    
    // 視覚表現アクター
    UPROPERTY(BlueprintReadWrite)
    class AGridCellActor* CellActor = nullptr;
};
```

### 3. AI実装：Behavior Tree完全移行（グリッド対応）

```cpp
// グリッド対応AIコントローラー
UCLASS()
class UE_IDLE_API AC_IdleAIController : public AAIController
{
    GENERATED_BODY()
    
protected:
    // Behavior Tree資産
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* IdleCharacterBT;
    
    // Blackboardデータ
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBlackboardData* IdleCharacterBB;
    
    // グリッドマップ参照
    UPROPERTY()
    UGridMapComponent* GridMapRef;
    
    virtual void OnPossess(APawn* InPawn) override
    {
        Super::OnPossess(InPawn);
        
        // グリッドマップ取得
        GridMapRef = GetGridMapComponent();
        
        // Behavior Tree開始
        if (IdleCharacterBT)
        {
            RunBehaviorTree(IdleCharacterBT);
        }
        
        // 毎ターン再評価（「毎ターン新しい判断」実装）
        GetWorld()->GetTimerManager().SetTimer(
            TurnTimer, this, &AC_IdleAIController::RestartBehaviorTree, 
            1.0f, true
        );
    }
};
```

#### Behavior Tree構造（グリッド版）

```
BT_IdleCharacter_Grid
│
├─ Root Selector（毎ターン評価）
│
├─ Sequence: 緊急帰還
│  ├─ Decorator: BTD_CheckInventoryFull
│  ├─ Task: BTT_FindBaseGrid (グリッド検索)
│  └─ Task: BTT_MoveToGridCell
│
├─ Sequence: タスク実行
│  ├─ Service: BTS_UpdateCurrentTask
│  ├─ Selector: タスク別処理
│  │  ├─ Sequence: 採集
│  │  │  ├─ Decorator: BTD_HasGatheringTask
│  │  │  ├─ Task: BTT_FindGatheringGrid (最適化グリッド検索)
│  │  │  ├─ Task: BTT_MoveToGridCell
│  │  │  └─ Task: BTT_ExecuteGathering
│  │  │
│  │  ├─ Sequence: 戦闘
│  │  │  ├─ Decorator: BTD_HasCombatTask
│  │  │  └─ Task: BTT_ExecuteCombat
│  │  │
│  │  └─ Sequence: 製作
│  │     ├─ Decorator: BTD_HasCraftingTask
│  │     └─ Task: BTT_ExecuteCrafting
│  │
│  └─ Task: BTT_CompleteTask
│
└─ Task: BTT_Wait（デフォルト）
```

### 4. 属性システム（ハイブリッドアプローチ）

```cpp
// 現在の設計を維持しつつ拡張性を確保
UCLASS()
class UE_IDLE_API UCharacterStatusComponent : public UActorComponent
{
    // === 既存の固定属性（高速アクセス） ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
    FCharacterStatus Status;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Status")
    FCharacterTalent Talent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Derived Stats")
    FDerivedStats DerivedStats;
    
    // === RPG Systemから取り入れる機能 ===
    
    // Modifierシステム
    UPROPERTY()
    TArray<FAttributeModifier> ActiveModifiers;
    
    // 拡張属性（将来用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extended Attributes")
    TMap<FGameplayTag, float> ExtendedAttributes;
    
    // === Modifier関連関数 ===
    UFUNCTION(BlueprintCallable, Category = "Modifiers")
    void AddModifier(const FAttributeModifier& Modifier);
    
    UFUNCTION(BlueprintCallable, Category = "Modifiers")
    void RemoveModifier(const FString& ModifierId);
    
    UFUNCTION(BlueprintCallable, Category = "Modifiers")
    void AddTimedModifier(const FAttributeModifier& Modifier, float Duration);
};

// Modifier構造体
USTRUCT(BlueprintType)
struct FAttributeModifier
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    FString ModifierId;
    
    UPROPERTY(BlueprintReadWrite)
    EModifierType Type; // Additive, Multiplicative
    
    // 固定属性への修正
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, float> StatModifiers;
    
    // GameplayTag属性への修正
    UPROPERTY(BlueprintReadWrite)
    TMap<FGameplayTag, float> TagModifiers;
};
```

### 5. EQS活用（グリッド最適化版）

```cpp
// グリッド用軽量EQS
UCLASS()
class UEnvQueryContext_GridCells : public UEnvQueryContext
{
    virtual void ProvideContext(FEnvQueryInstance& QueryInstance, 
                               FEnvQueryContextData& ContextData) const override
    {
        // グリッドベースの高速検索
        auto* GridMap = GetGridMapComponent();
        
        // 現在位置から一定範囲のセルのみ検索
        FIntPoint CurrentGrid = GetQuerierGridPosition(QueryInstance);
        int32 SearchRadius = 5; // 5セル範囲
        
        TArray<FVector> CellLocations;
        for (int32 dx = -SearchRadius; dx <= SearchRadius; dx++)
        {
            for (int32 dy = -SearchRadius; dy <= SearchRadius; dy++)
            {
                FIntPoint CellPos = CurrentGrid + FIntPoint(dx, dy);
                if (GridMap->IsValidCell(CellPos))
                {
                    CellLocations.Add(GridMap->GridToWorld(CellPos));
                }
            }
        }
        
        UEnvQueryItemType_Point::SetContextHelper(ContextData, CellLocations);
    }
};
```

### 6. GameplayTags定義（グリッド対応）

```cpp
// DefaultGameplayTags.ini
[/Script/GameplayTags.GameplayTagsSettings]
// タスクタイプ
+GameplayTagList=(Tag="Task.Gathering", DevComment="採集タスク")
+GameplayTagList=(Tag="Task.Combat", DevComment="戦闘タスク")
+GameplayTagList=(Tag="Task.Crafting", DevComment="製作タスク")

// 状態
+GameplayTagList=(Tag="State.Moving", DevComment="移動中")
+GameplayTagList=(Tag="State.Working", DevComment="作業中")
+GameplayTagList=(Tag="State.InCombat", DevComment="戦闘中")

// グリッド場所タイプ
+GameplayTagList=(Tag="Location.Base", DevComment="拠点")
+GameplayTagList=(Tag="Location.Forest", DevComment="森")
+GameplayTagList=(Tag="Location.Plains", DevComment="平原")
+GameplayTagList=(Tag="Location.Mountain", DevComment="山")
+GameplayTagList=(Tag="Location.River", DevComment="川")

// リソース
+GameplayTagList=(Tag="Resource.Wood", DevComment="木材")
+GameplayTagList=(Tag="Resource.Stone", DevComment="石材")
+GameplayTagList=(Tag="Resource.Food", DevComment="食料")
```

### 7. 2D表示システム

```cpp
// グリッドセル表示アクター
UCLASS()
class AGridCellActor : public AActor
{
    GENERATED_BODY()
    
public:
    // 2D表示コンポーネント
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* CellMesh;
    
    UPROPERTY(VisibleAnywhere)
    UTextRenderComponent* CellLabel;
    
    // セルタイプ別マテリアル
    UPROPERTY(EditDefaultsOnly)
    TMap<FGameplayTag, UMaterialInterface*> CellMaterials;
    
    // セルタイプ設定
    void SetCellType(FGameplayTag LocationType)
    {
        if (auto* Material = CellMaterials.Find(LocationType))
        {
            CellMesh->SetMaterial(0, *Material);
        }
        
        // ラベル更新
        FString TypeName = LocationType.GetTagName().ToString();
        CellLabel->SetText(FText::FromString(TypeName));
    }
};

// 2Dカメラ設定
UCLASS()
class ATopDownCamera : public AActor
{
    UPROPERTY(VisibleAnywhere)
    UCameraComponent* Camera;
    
    ATopDownCamera()
    {
        // 真上から見下ろすカメラ
        Camera->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
        Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
        Camera->OrthoWidth = 5000.0f; // 視野幅
    }
};
```

## 📊 実装スケジュール（改訂版）

### Phase 1: 基盤整備（1週間）
1. **Day 1-2**: AC_IdleCharacterをAPawnに変更、2D表示設定
2. **Day 3-4**: グリッドマップシステム基本実装
3. **Day 5-7**: AIControllerとBehavior Tree基本構造作成

### Phase 2: グリッドシステム完成（1週間）
1. **Day 1-2**: グリッドマップコンポーネント詳細実装
2. **Day 3-4**: A*パス探索とグリッド移動実装
3. **Day 5-7**: グリッドセル表示システム

### Phase 3: AI移行（2週間）
1. **Week 1**: CharacterBrainロジックをBTタスクに移植
2. **Week 2**: グリッドベースEQS実装とBT統合

### Phase 4: システム統合（1週間）
1. **Day 1-2**: 属性システムのModifier機能追加
2. **Day 3-4**: GameplayTags完全統合
3. **Day 5-7**: 統合テストとデバッグ

### Phase 5: 最適化（1週間）
1. グリッド検索最適化
2. 2D表示のLOD実装
3. モバイル向けパフォーマンス調整

## 🎮 グリッド移動の実装例

```cpp
// BTタスク：グリッドセルへの移動
UCLASS()
class UBTTask_MoveToGridCell : public UBTTaskNode
{
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, 
                                          uint8* NodeMemory) override
    {
        auto* Character = Cast<AC_IdleCharacter>(OwnerComp.GetAIOwner()->GetPawn());
        auto* BB = OwnerComp.GetBlackboardComponent();
        
        // 目標グリッド座標取得
        FIntPoint TargetGrid = BB->GetValueAsVector("TargetGridPosition");
        
        // パス計算
        TArray<FIntPoint> Path = GridMap->FindPath(
            Character->CurrentGridPosition, 
            TargetGrid
        );
        
        if (Path.Num() > 0)
        {
            // スプライン移動開始
            Character->StartGridMovement(Path);
            return EBTNodeResult::InProgress;
        }
        
        return EBTNodeResult::Failed;
    }
};

// キャラクターのグリッド移動
void AC_IdleCharacter::StartGridMovement(const TArray<FIntPoint>& Path)
{
    // ワールド座標パスに変換
    TArray<FVector> WorldPath;
    for (const auto& GridPos : Path)
    {
        WorldPath.Add(GridMap->GridToWorld(GridPos));
    }
    
    // スプライン作成
    USplineComponent* MovementSpline = NewObject<USplineComponent>(this);
    for (int32 i = 0; i < WorldPath.Num(); i++)
    {
        MovementSpline->AddSplinePoint(WorldPath[i], ESplineCoordinateSpace::World);
    }
    
    // 移動開始
    CurrentSplineDistance = 0.0f;
    TotalSplineLength = MovementSpline->GetSplineLength();
}
```

## 💡 追加最適化

### モバイル向け最適化
```cpp
// LODシステム
void UpdateLOD(float DistanceToCamera)
{
    if (DistanceToCamera > 5000.0f)
    {
        // 遠距離：非表示
        SetActorHiddenInGame(true);
        SetActorTickEnabled(false);
    }
    else if (DistanceToCamera > 2000.0f)
    {
        // 中距離：簡易表示
        SpriteComponent->SetSprite(SimplifiedSprite);
        SetActorTickInterval(0.5f);
    }
    else
    {
        // 近距離：詳細表示
        SpriteComponent->SetSprite(DetailedSprite);
        SetActorTickInterval(0.0f);
    }
}
```

### グリッド空間分割
```cpp
// 大規模グリッド用の最適化
class UGridMapComponent_Optimized : public UGridMapComponent
{
    // チャンク分割
    static constexpr int32 ChunkSize = 10;
    TMap<FIntPoint, FGridChunk> Chunks;
    
    // 視界内のチャンクのみ更新
    void UpdateVisibleChunks(const FVector& ViewerPosition)
    {
        FIntPoint ViewerChunk = GetChunkCoord(WorldToGrid(ViewerPosition));
        
        // 周辺チャンクのみアクティブ化
        for (int32 dx = -1; dx <= 1; dx++)
        {
            for (int32 dy = -1; dy <= 1; dy++)
            {
                ActivateChunk(ViewerChunk + FIntPoint(dx, dy));
            }
        }
    }
};
```

## 🎯 結論

**この改訂版計画により、UE_IdleWSLは以下を実現します：**

### 達成される成果
1. **グリッドベース場所管理** - 明確で効率的な空間管理
2. **2D表示最適化** - モバイル向け軽量表示
3. **属性システム最適化** - 現行システム維持＋拡張性確保
4. **AI標準化** - Behavior Treeによる視覚的デバッグ
5. **パフォーマンス** - グリッド検索とLODによる最適化

### メリット
- **開発効率**: グリッドによる明確な場所概念
- **拡張性**: GameplayTagsとModifierシステム
- **保守性**: UE標準機能の活用
- **性能**: モバイル最適化済み
- **視認性**: 2D表示で分かりやすい

**「毎ターン新しい判断」設計思想は、グリッドシステムとBehavior TreeのRestartLogic()で完璧に実現されます。**