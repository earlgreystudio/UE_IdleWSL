# LocationEventManager 仕様書

## 概要

`ULocationEventManager`は、ゲーム内の場所でのイベント（戦闘、採取など）をトリガーするActorComponentです。チームが場所に派遣された際の敵生成とイベント発火を担当し、戦闘システムとの橋渡しを行います。

## クラス構成

### 継承関係
```cpp
UActorComponent
└── ULocationEventManager (Component)
```

### 依存関係
```cpp
ULocationEventManager
├── UCharacterPresetManager (GameInstance Subsystem) - 敵キャラクタ生成
├── UCombatComponent (同一Owner) - 戦闘処理委託
└── LocationData DataTable - 場所情報・敵出現設定
```

## 主要機能

### 1. 戦闘イベントトリガー

#### TriggerCombatEvent
```cpp
UFUNCTION(BlueprintCallable, Category = "Location Events")
bool TriggerCombatEvent(const FString& LocationId, const TArray<AC_IdleCharacter*>& AllyTeam);
```

**処理フロー:**
1. 味方チーム有効性チェック (`AllyTeam.Num() > 0`)
2. 場所での戦闘可能性チェック (`CanTriggerCombatAtLocation`)
3. 敵チーム生成 (`CreateEnemyTeamForLocation`)
4. 敵チーム登録 (`RegisterEnemyTeam`)
5. CombatComponentに戦闘委託 (`StartCombat`)
6. イベント通知 (`OnLocationEventTriggered.Broadcast`)

**戻り値:** 戦闘開始成功時 `true`、失敗時 `false`

### 2. 敵チーム生成システム

#### CreateEnemyTeamForLocation
```cpp
UFUNCTION(BlueprintCallable, Category = "Location Events", meta = (WorldContext = "WorldContextObject"))
TArray<AC_IdleCharacter*> CreateEnemyTeamForLocation(
    UObject* WorldContextObject,
    const FString& LocationId,
    const FVector& SpawnLocation,
    int32 EnemyCount = 1
);
```

**敵選択システム:**
- `SelectRandomEnemyForLocation` - 場所の `EnemySpawnListString` から確率ベースで選択
- `CharacterPresetManager->GetRandomEnemyFromLocation` に委託

**スポーン位置計算:**
```cpp
FVector CalculateEnemySpawnPosition(const FVector& CenterLocation, float Radius, int32 Index)
{
    // 敵を円形に配置
    float Angle = (360.0f / MaxEnemiesPerTeam) * Index * (PI / 180.0f);
    float Distance = FMath::RandRange(Radius * 0.5f, Radius);
    
    FVector Offset(
        Distance * FMath::Cos(Angle),
        Distance * FMath::Sin(Angle),
        0.0f
    );
    
    return CenterLocation + Offset;
}
```

### 3. 場所データ管理

#### 場所情報取得
```cpp
// 場所データ取得（CharacterPresetManager経由）
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
FLocationDataRow GetLocationData(const FString& LocationId) const;

// 日本語表示名取得
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
FString GetLocationDisplayName(const FString& LocationId) const;

// 戦闘可能性チェック
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
bool CanTriggerCombatAtLocation(const FString& LocationId) const;
```

#### 場所ID管理
```cpp
// 全場所ID取得（現在はハードコード）
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
TArray<FString> GetAllLocationIds() const;
// 戻り値: { "base", "plains", "swamp", "cave" }

// 有効な場所ID取得（将来DataTable動的取得予定）
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
TArray<FString> GetValidLocationIds() const;
```

### 4. 敵チーム管理

#### 登録・削除システム
```cpp
// 敵チーム登録（戦闘開始時）
UFUNCTION(BlueprintCallable, Category = "Location Events")
void RegisterEnemyTeam(const TArray<AC_IdleCharacter*>& EnemyTeam, const FString& LocationId);

// 敵チーム削除（戦闘終了時）
UFUNCTION(BlueprintCallable, Category = "Location Events")
void UnregisterEnemyTeam(const TArray<AC_IdleCharacter*>& EnemyTeam);

// 全敵チームクリア
UFUNCTION(BlueprintCallable, Category = "Location Events")
void ClearAllEnemyTeams();
```

**ActiveEnemyTeams管理:**
```cpp
// 場所別敵チーム管理
TMap<FString, TArray<AC_IdleCharacter*>> ActiveEnemyTeams;
```

- **登録**: 場所IDをキーとして敵チーム配列を保存
- **削除**: 敵キャラクタの `Destroy()` を呼び出してメモリ解放
- **クリア**: 全ての敵を即座に削除してメモリリーク防止

### 5. イベントシステム

#### イベントディスパッチャー
```cpp
// 場所イベント発火通知
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLocationEventTriggered, 
    const FString&, LocationId, 
    const FString&, EventType, 
    const TArray<AC_IdleCharacter*>&, ParticipatingTeam);

// 敵チーム生成完了通知
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyTeamCreated, 
    const TArray<AC_IdleCharacter*>&, EnemyTeam, 
    const FString&, LocationId);
```

#### 発火タイミング
- **OnLocationEventTriggered**: `TriggerCombatEvent` または `TriggerGatheringEvent` 実行時
- **OnEnemyTeamCreated**: `CreateEnemyTeamForLocation` で敵生成成功時

## 設定プロパティ

### Blueprint設定項目
```cpp
// 最大敵数（チームあたり）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
int32 MaxEnemiesPerTeam = 3;

// 敵スポーン半径
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
float EnemySpawnRadius = 500.0f;
```

## 内部処理詳細

### BeginPlay初期化
```cpp
void ULocationEventManager::BeginPlay()
{
    // CharacterPresetManagerの取得（GameInstance Subsystem）
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        PresetManager = GameInstance->GetSubsystem<UCharacterPresetManager>();
    }

    // CombatComponentの取得（同一Owner）
    if (AActor* Owner = GetOwner())
    {
        CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
    }
}
```

### FTeam構造体変換
```cpp
// CombatComponent.StartCombat用のFTeam構造体作成
FTeam AllyTeamStruct;
AllyTeamStruct.Members = AllyTeam;
AllyTeamStruct.TeamName = TEXT("派遣チーム");
AllyTeamStruct.bInCombat = true;

bool bCombatStarted = CombatComponent->StartCombat(AllyTeamStruct, EnemyTeam, LocationId);
```

### エラーハンドリング
- **PresetManager未取得**: エラーログ出力、敵生成失敗
- **CombatComponent未取得**: 警告ログ出力、戦闘開始失敗
- **敵生成失敗**: 敵チーム登録解除、戦闘開始キャンセル
- **戦闘開始失敗**: 敵チーム削除、処理ロールバック

## 使用例

### 基本的な戦闘開始
```cpp
// TeamComponent.cpp での使用例
bool UTeamComponent::StartAdventure(int32 TeamIndex, const FString& LocationId)
{
    FTeam& Team = Teams[TeamIndex];
    Team.AdventureLocationId = LocationId;
    Team.bInCombat = true;

    // LocationEventManagerに戦闘委託
    ULocationEventManager* EventManager = GetOwner()->FindComponentByClass<ULocationEventManager>();
    if (EventManager)
    {
        return EventManager->TriggerCombatEvent(LocationId, Team.Members);
    }
    
    return false;
}
```

### イベントバインディング
```cpp
// Blueprint/C++でのイベントバインディング
LocationEventManager->OnLocationEventTriggered.AddDynamic(
    this, &AMyActor::OnLocationEventHandler);

LocationEventManager->OnEnemyTeamCreated.AddDynamic(
    this, &AMyActor::OnEnemyTeamCreatedHandler);
```

### カスタム敵生成
```cpp
// 特定の場所で敵チーム生成
TArray<AC_IdleCharacter*> CustomEnemies = LocationEventManager->CreateEnemyTeamForLocation(
    this, "cave", GetActorLocation(), 3);

if (CustomEnemies.Num() > 0)
{
    // カスタム処理
    LocationEventManager->RegisterEnemyTeam(CustomEnemies, "cave");
}
```

## 場所タイプシステム

### ELocationType列挙型
```cpp
UENUM(BlueprintType)
enum class ELocationType : uint8
{
    Base,      // 拠点（敵出現なし）
    Plains,    // 平野
    Swamp,     // 沼地
    Cave,      // 洞窟
    Count      // 総数（使用しない）
};
```

### タイプ変換関数
```cpp
// ELocationTypeを文字列に変換
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
static FString LocationTypeToString(ELocationType LocationType);

// 全場所タイプ取得
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Location Events")
static TArray<ELocationType> GetAllLocationTypes();
```

## 将来の拡張予定

### 1. 採取イベント完全実装
```cpp
// 現在はスタブ実装
bool ULocationEventManager::TriggerGatheringEvent(const FString& LocationId, const TArray<AC_IdleCharacter*>& AllyTeam)
{
    // 将来拡張用の採取イベント
    UE_LOG(LogTemp, Log, TEXT("Gathering event triggered at location %s (not implemented yet)"), *LocationId);
    
    OnLocationEventTriggered.Broadcast(LocationId, TEXT("Gathering"), AllyTeam);
    return true;
}
```

### 2. DataTable動的取得
```cpp
// 計画中の実装
TArray<FString> ULocationEventManager::GetValidLocationIds() const
{
    if (PresetManager)
    {
        // PresetManagerからDataTableのRowNameを動的取得
        return PresetManager->GetAllLocationRowNames();
    }
    
    return GetAllLocationIds(); // フォールバック
}
```

### 3. マルチイベントサポート
- 同時複数イベントの管理
- イベント優先度システム
- イベント条件判定（時間帯、天候等）

### 4. イベント履歴システム
- 発生したイベントの記録
- 場所別統計情報
- イベント成功率分析

## デバッグ・ログ機能

### ログ出力例
```
LocationEventManager: Combat event triggered at location plains with 3 allies vs 1 enemies
LocationEventManager: Created enemy rat for location plains
LocationEventManager: Registered enemy team (1 enemies) for location plains
LocationEventManager: Failed to get CharacterPresetManager
LocationEventManager: CombatComponent not found on owner
```

### デバッグ支援
```cpp
// 現在のアクティブ敵チーム確認
void PrintActiveEnemyTeams()
{
    for (auto& Pair : ActiveEnemyTeams)
    {
        UE_LOG(LogTemp, Log, TEXT("Location: %s, Enemies: %d"), 
            *Pair.Key, Pair.Value.Num());
    }
}
```

## パフォーマンス考慮事項

### 最適化ポイント
1. **敵生成制限**: `MaxEnemiesPerTeam` による大量生成防止
2. **即座削除**: 戦闘終了時の敵アクター即座削除
3. **参照キャッシュ**: BeginPlayでの依存コンポーネント事前取得
4. **軽量通知**: イベントディスパッチャーでの効率的な通知

### メモリ管理
- **敵チーム**: 戦闘終了時に自動削除 (`Enemy->Destroy()`)
- **参照管理**: TObjectPtrの使用でメモリリーク防止
- **マップクリア**: ClearAllEnemyTeams でのマップ完全クリア

この統一システムにより、場所でのイベント発火から敵生成、戦闘開始まで一貫した処理フローが実現され、将来の機能拡張も容易になります。