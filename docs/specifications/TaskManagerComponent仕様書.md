# TaskManagerComponent 仕様書

## 概要

`UTaskManagerComponent`は、アイドルゲームにおけるグローバルタスク（全チーム共通の目標）の管理を行うActorComponentです。タスクの追加・削除・優先度管理・進行状況追跡・完了処理などの包括的な機能を提供します。

## 基本情報

- **クラス名**: `UTaskManagerComponent`
- **継承**: `UActorComponent`
- **配置**: `Source/UE_Idle/Components/TaskManagerComponent.h/.cpp`
- **依存関係**: `TaskTypes.h`, `TeamTypes.h`, `InventoryComponent`, `TeamComponent`
- **Tick無効**: `PrimaryComponentTick.bCanEverTick = false`

## 主要機能

### 1. タスク管理機能

#### タスクの追加
```cpp
int32 AddGlobalTask(const FGlobalTask& NewTask)
```
- 新しいグローバルタスクを追加
- 最大20個まで登録可能（MaxGlobalTasks設定）
- 重複する優先度は自動調整（既存タスクの優先度を+1）
- タスクの有効性を自動検証（ValidateTask）
- 関連スキルの自動設定（UTaskTypeUtils::GetTaskRelatedSkills）
- 作成時刻の自動設定（FDateTime::Now）
- 処理中フラグチェック（bProcessingTasks）
- 戻り値：追加されたタスクのインデックス（失敗時は-1）

#### タスクの削除
```cpp
bool RemoveGlobalTask(int32 TaskIndex)
```
- 指定インデックスのタスクを削除
- 処理中フラグチェック（bProcessingTasks）
- インデックス有効性検証（IsValidTaskIndex）
- 削除後は優先度を自動再計算（RecalculatePriorities）
- イベント通知機能付き（OnGlobalTaskRemoved）
- 詳細ログ出力

#### タスクの取得
```cpp
TArray<FGlobalTask> GetGlobalTasksByPriority() const
FGlobalTask GetGlobalTask(int32 TaskIndex) const
const TArray<FGlobalTask>& GetGlobalTasks() const
```
- 優先度順でのソート取得
- 個別タスクの取得
- 全タスクリストの参照取得

### 2. 優先度管理機能

#### 優先度の調整
```cpp
bool UpdateTaskPriority(int32 TaskIndex, int32 NewPriority)
bool SwapTaskPriority(int32 TaskIndex1, int32 TaskIndex2)
bool MoveTaskUp(int32 TaskIndex)
bool MoveTaskDown(int32 TaskIndex)
void RecalculatePriorities()
```
- 数値が小さいほど高優先度（1が最高）
- タスクの上下移動
- 優先度の入れ替え
- 連番での優先度再計算

### 3. タスク選択・実行機能

#### 実行可能タスクの選択
```cpp
FGlobalTask GetNextAvailableTask(int32 TeamIndex) const
bool CanTeamExecuteTask(int32 TeamIndex, const FGlobalTask& Task) const
```
- 「全て」モード用の自動タスク選択
- チームのスキル・リソース条件をチェック
- 優先度順で最適なタスクを選択

### 4. リソース監視・判定機能

#### リソース要件のチェック
```cpp
bool CheckResourceRequirements(const FTeamTask& Task) const
bool CheckGlobalTaskRequirements(const FGlobalTask& Task, int32 TeamIndex) const
int32 GetTotalResourceAmount(const FString& ResourceId) const
```
- グローバルインベントリ + 全チームインベントリの合計計算
- タスクタイプ別の具体的要件チェック：
  - **建築** (`ETaskType::Construction`): 木材10 + 石材5
  - **料理** (`ETaskType::Cooking`): 食材1（ingredient）
  - **採集** (`ETaskType::Gathering`): 常に実行可能
  - **製作** (`ETaskType::Crafting`): 材料1（material）
  - **その他**: デフォルトで実行可能
- null参照チェックとエラーハンドリング
- VeryVerboseレベルでの詳細ログ出力

### 5. チームタスク優先度ベースマッチングシステム

#### 新しいタスク選択ロジック
```cpp
FString GetTargetItemForTeam(int32 TeamIndex, const FString& LocationId) const
```
- **チームタスク優先**: グローバルタスクよりチームタスクの優先度を優先
- **優先度順マッチング**: チームタスクの優先度順でグローバルタスクとマッチング
- **毎ターン再計算**: 状態変化に即座に対応
- **ターンベース設計**: 完全な状態リセットによる確実な動作

#### タスクマッチング用ヘルパー関数群
```cpp
FString FindMatchingGlobalTask(const FTeamTask& TeamTask, int32 TeamIndex, const FString& LocationId) const
FString FindMatchingGatheringTask(int32 TeamIndex, const FString& LocationId) const
FString FindMatchingAdventureTask(int32 TeamIndex, const FString& LocationId) const
FString FindMatchingConstructionTask(int32 TeamIndex) const
FString FindMatchingCookingTask(int32 TeamIndex) const
FString FindMatchingCraftingTask(int32 TeamIndex) const
```
- **タスクタイプ別の専用マッチング**: 各タスクタイプに特化した判定ロジック
- **拡張可能設計**: 新しいタスクタイプの追加が容易
- **ログ統合**: VeryVerboseレベルでの詳細マッチング過程追跡

### 6. 採集数量タイプの詳細管理

#### 3つの採集数量タイプ
```cpp
enum class EGatheringQuantityType {
    Unlimited,    // 無制限採集（永続実行）
    Keep,         // キープ型（拠点全体で指定数量維持）
    Specified     // 個数指定型（目標達成で自動削除）
}
```

#### Keep タスクの満足度判定
```cpp
// Keep タスクは拠点全体の在庫量で判定
int32 TotalCurrentAmount = GetTotalResourceAmount(Task.TargetItemId);
if (TotalCurrentAmount >= Task.TargetQuantity) {
    // 満足済み → 次の優先度タスクへ移行
}
```
- **拠点全体在庫**: 拠点倉庫 + 全チーム所持の合計で判定
- **適正数量維持**: 既存在庫を考慮した必要量のみ採集
- **優先度システム連携**: 満足時は次優先度タスクに自動移行

### 8. タスク完了処理

#### 進行状況の管理
```cpp
void ProcessTaskCompletion(const FString& TaskId, int32 CompletedAmount)
bool UpdateTaskProgress(const FString& TaskId, int32 ProgressAmount)
void ClearCompletedTasks()
```
- 重複処理防止機能
- 完了時の自動イベント通知
- 完了タスクの一括削除

#### 数量タイプ別完了ロジック
```cpp
switch (Task.GatheringQuantityType) {
    case EGatheringQuantityType::Unlimited:
        // 無制限採集は永続的に実行（完了しない）
        return true;
    case EGatheringQuantityType::Keep:
        // キープ型は永続的に実行（完了しない）
        return true;
    case EGatheringQuantityType::Specified:
        // 個数指定型のみ完了判定・自動削除
        if (Task.CurrentProgress >= Task.TargetQuantity) {
            Task.bIsCompleted = true;
            GlobalTasks.RemoveAt(TaskIndex);
        }
        return true;
}
```

### 7. 安全性確保機能

#### 処理の排他制御
```cpp
bool IsProcessingTasks() const
template<typename FunctionType>
bool ExecuteSafely(FunctionType Function)
```
- `bProcessingTasks`フラグによる重複処理防止
- タスク処理中の追加・削除・更新を制限
- データ整合性の保証
- テンプレート関数による安全な処理実行

#### 内部ヘルパー関数
```cpp
bool ValidateTask(const FGlobalTask& Task) const
bool HasDuplicatePriority(int32 Priority, int32 ExcludeIndex = -1) const
bool IsValidArrayAccess(int32 Index) const
void LogTaskOperation(const FString& Operation, const FGlobalTask& Task) const
void LogError(const FString& ErrorMessage) const
```
- タスクの整合性チェック（重複ID検証含む）
- 優先度の重複検出
- 配列アクセスの安全性確保
- 詳細なログ出力（操作内容、エラー）

## データ構造

### 主要プロパティ
```cpp
TArray<FGlobalTask> GlobalTasks;              // 全体タスクリスト
int32 MaxGlobalTasks = 20;                    // 最大タスク数
bool bProcessingTasks = false;                // 処理中フラグ
UInventoryComponent* GlobalInventoryRef;      // グローバルインベントリ参照
UTeamComponent* TeamComponentRef;             // チームコンポーネント参照
```

### 設定パラメータ
- **MaxGlobalTasks**: 最大タスク数（デフォルト：20、EditAnywhere）
- **bProcessingTasks**: 処理中フラグ（安全性確保用、BlueprintReadOnly）

### 参照コンポーネント
- **GlobalInventoryRef**: グローバルインベントリへの参照（UInventoryComponent*）
- **TeamComponentRef**: チーム管理コンポーネントへの参照（UTeamComponent*）

### 自動管理機能
- **関連スキル設定**: タスクタイプに基づく自動スキル関連付け
- **優先度調整**: 重複優先度の自動調整
- **作成時刻記録**: タスク追加時の自動タイムスタンプ

## ライフサイクル管理

### 初期化 (BeginPlay)
```cpp
virtual void BeginPlay() override
```
- 基底クラスのBeginPlay呼び出し
- 初期化完了ログ出力

### 終了処理 (BeginDestroy)
```cpp
virtual void BeginDestroy() override
```
- 処理中フラグのクリア (`bProcessingTasks = false`)
- 参照コンポーネントのクリア (`GlobalInventoryRef = nullptr`, `TeamComponentRef = nullptr`)
- 全タイマーのクリア (`World->GetTimerManager().ClearAllTimersForObject(this)`)
- 基底クラスのBeginDestroy呼び出し

## イベントシステム

### イベントディスパッチャー
```cpp
FOnGlobalTaskAdded OnGlobalTaskAdded          // タスク追加時
FOnGlobalTaskCompleted OnGlobalTaskCompleted // タスク完了時
FOnTaskPriorityChanged OnTaskPriorityChanged // 優先度変更時
FOnGlobalTaskRemoved OnGlobalTaskRemoved     // タスク削除時
```

### UI連携
- TaskListコンポーネントが自動的にイベントをリッスン
- リアルタイムでのUI更新
- タスクの追加・削除・優先度変更を即座に反映

## 新機能・ユーティリティ

### 追加されたユーティリティ関数
```cpp
int32 FindTaskByID(const FString& TaskId) const              // タスクID検索
int32 GetIncompleteTaskCount() const                         // 未完了タスク数取得
int32 GetCompletedTaskCount() const                          // 完了タスク数取得
bool IsValidTaskIndex(int32 TaskIndex) const                 // インデックス有効性チェック
```

### 実行可能性チェック
```cpp
bool CanTeamExecuteTask(int32 TeamIndex, const FGlobalTask& Task) const
```
- チームの有効性チェック (`Team.IsValidTeam()`)
- タスクの有効性・完了状態チェック
- リソース要件の確認
- チームサイズの確認（最低1人必要）

### 優先度管理の改善
- **MoveTaskUp/MoveTaskDown**: 直感的な優先度移動
- **RecalculatePriorities**: 連番での優先度再計算
- **SwapTaskPriority**: 2つのタスク間での優先度交換

## 使用例

### 基本的な使用方法
```cpp
// 1. 参照の設定
TaskManager->SetGlobalInventoryReference(GlobalInventory);
TaskManager->SetTeamComponentReference(TeamComponent);

// 2. タスクの追加
FGlobalTask NewTask;
NewTask.TaskId = TEXT("cooking_task_001");
NewTask.DisplayName = TEXT("料理: スープ x5");
NewTask.TaskType = ETaskType::Cooking;
NewTask.TargetQuantity = 5;
NewTask.Priority = 1;

int32 TaskIndex = TaskManager->AddGlobalTask(NewTask);

// 3. 進行状況の更新
TaskManager->UpdateTaskProgress(TEXT("cooking_task_001"), 1);

// 4. 完了チェック
if (TaskManager->GetGlobalTask(TaskIndex).bIsCompleted) {
    TaskManager->ClearCompletedTasks();
}
```

### チーム向けタスク選択
```cpp
// 指定チームに最適なタスクを自動選択
FGlobalTask NextTask = TaskManager->GetNextAvailableTask(TeamIndex);
if (NextTask.IsValid()) {
    // チームにタスク割り当て
    TeamComponent->AssignTaskToTeam(TeamIndex, NextTask);
}
```

## 注意事項

### パフォーマンス考慮
- タスク数の上限（20個）による制限
- 優先度ソート処理のコスト
- リソースチェックの頻度

### 安全性
- 処理中フラグによる重複処理防止
- 無効なインデックスアクセスの防止
- null参照チェック

### 拡張性
- タスクタイプ別の要件チェック拡張可能
- カスタムバリデーション追加可能
- イベントシステムによる柔軟な連携

## ログ出力

### 情報レベル (LogTemp, Log)
- タスクの追加・削除・完了（詳細情報付き）
- 優先度の変更・交換
- 参照の設定確認
- 優先度再計算の実行
- 完了タスクの一括削除

### 詳細レベル (LogTemp, VeryVerbose)
- 次実行可能タスクの検索結果
- リソース不足の詳細
- タスク進行状況の更新

### 警告レベル (LogTemp, Warning)
- 優先度の重複調整
- 無効なタスクインデックスアクセス

### エラーレベル (LogTemp, Error)
- null参照エラー
- タスクの検証失敗
- 処理中の操作試行
- 重複タスクID
- 最大タスク数超過

## UI連携システム

### タスクUI階層構造

```
タスク管理UI階層:
├── C_TaskList (グローバルタスク一覧表示)
│   └── C_GlobalTaskCard[] (個別グローバルタスク表示・操作)
├── C_TaskMakeSheet (グローバルタスク作成)
│   └── 作成後 → C_TaskList に追加
└── C_TeamCard (チーム情報表示)
    ├── C_TeamTaskCard[] (チーム別タスク表示・操作)
    └── C_TeamTaskMakeSheet (チーム用タスク作成) ※実装予定
```

### UIコンポーネント詳細

#### **C_TaskList** (グローバルタスク一覧)
- **機能**: 全グローバルタスクの表示とリアルタイム更新
- **連携**: TaskManagerComponentのイベント（OnGlobalTaskAdded/Removed/PriorityChanged）
- **コンテナ**: `TaskCardPanel` → UC_GlobalTaskCard配列
- **自動初期化**: PlayerControllerから自動取得・初期化

#### **C_GlobalTaskCard** (グローバルタスク操作)
- **機能**: 
  - タスク情報表示（タイプ、アイテム名、数量、スキル、優先度、進行状況）
  - 優先度変更（Up/Down）、削除機能
- **連携**: TaskManagerComponent、ItemDataTableManager
- **操作**: 優先度変更 → TaskManager.UpdateTaskPriority/MoveTaskUp/Down

#### **C_TaskMakeSheet** (グローバルタスク作成)
- **機能**:
  - タスクタイプ選択（ETaskType）
  - 対象アイテム選択、数量・優先度設定
  - 場所選択、キープ数量オプション
- **連携**: TaskManagerComponent、CraftingComponent
- **検証**: 入力検証、重複チェック、ユニークID生成

#### **C_TeamTaskCard** (チーム用タスク操作)
- **機能**:
  - チーム用タスク表示（タイプ、優先度、スキル効果、状態）
  - チームメンバーのスキル合計計算・表示
  - 優先度変更、削除機能
- **連携**: TeamComponent
- **特殊機能**: チームメンバー変更時のスキル再計算

#### **C_TeamTaskMakeSheet** (チーム用タスク作成) ※実装予定
- **機能**:
  - チーム用タスク作成（FTeamTask）
  - 最小必要人数、推定完了時間設定
  - リソース要件・アイテム要件設定
- **連携**: TeamComponent
- **呼び出し**: C_TeamCard内のボタンから起動

### イベント連携フロー

#### **グローバルタスク管理**
```cpp
// TaskManagerComponent → C_TaskList
OnGlobalTaskAdded.Broadcast(NewTask)     → C_TaskList::OnGlobalTaskAdded()
OnGlobalTaskRemoved.Broadcast(Index)     → C_TaskList::OnGlobalTaskRemoved()
OnTaskPriorityChanged.Broadcast(Index)   → C_TaskList::OnTaskPriorityChanged()

// UI → TaskManagerComponent
UC_GlobalTaskCard::OnPriorityUpClicked() → TaskManager->MoveTaskUp()
UC_GlobalTaskCard::OnDeleteClicked()     → TaskManager->RemoveGlobalTask()
```

#### **チームタスク管理**
```cpp
// TeamComponent → C_TeamCard
OnTeamTaskStarted.Broadcast()    → C_TeamCard::UpdateTaskCards()
OnTeamTaskCompleted.Broadcast()  → C_TeamCard::UpdateTaskCards()
OnTeamTaskSwitched.Broadcast()   → C_TeamCard::UpdateTaskCards()

// UI → TeamComponent  
UC_TeamTaskCard::OnDeleteClicked() → TeamComponent->RemoveTeamTask()
```

### TimeManagerComponent連携

#### **時間進行とUI更新**
- **ProcessTimeUpdate()**: 1秒間隔でタスク進行状況更新
- **UI反映**: TaskManagerComponentのイベント経由でリアルタイム更新
- **「全て」モード**: GetNextAvailableTask() → 最適タスク自動選択・UI反映

#### **タスク実行状態管理**
```cpp
// TimeManagerComponent → UI
ProcessGlobalTasks()     → 進行状況更新 → UI自動反映
ProcessTeamTasks()       → チーム状態更新 → C_TeamCard更新
CheckResourceConditions() → リソース不足時の自動タスク切り替え
```

### 設計原則

#### **責任分離**
- **UC_GlobalTaskCard**: グローバルタスク表示・操作専用
- **UC_TeamTaskCard**: チーム用タスク表示・チーム特化機能専用
- **継承関係なし**: 用途が明確に異なるため個別設計が適切

#### **イベント駆動設計**
- Component間の直接参照を避けイベントディスパッチャー使用
- UI更新のリアルタイム性確保
- 疎結合による保守性向上

#### **自動初期化**
- PlayerControllerからの自動コンポーネント取得
- 手動セットアップ不要の設計

## 今後の拡張予定

1. **C_TeamTaskMakeSheet実装**
   - チーム用タスク作成UI完成
   - C_TeamCard内のボタンからの起動

2. **タスクテンプレート機能**
   - 事前定義されたタスクの自動生成

3. **条件付きタスク**
   - 特定条件での自動発生

4. **タスクの依存関係**
   - 前提タスクの完了チェック

5. **統計機能**
   - タスク完了時間の分析
   - 効率性の測定