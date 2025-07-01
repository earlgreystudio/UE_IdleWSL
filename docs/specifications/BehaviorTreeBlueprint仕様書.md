# Behavior Tree Blueprint 作成仕様書

## 概要

UE機能活用修正計画で実装されたC++クラス群を使用して、実際のBehavior TreeとBlackboardアセットをBlueprint Editor内で作成するための詳細仕様書です。

## 1. Blackboard Asset 作成 (BB_IdleCharacter)

### 作成手順
1. Content Browser → 右クリック → Artificial Intelligence → Blackboard
2. 名前: `BB_IdleCharacter`
3. 保存場所: `Content/AI/BB_IdleCharacter.uasset`

### Blackboard Keys 設定

| Key Name | Type | Description | Default Value |
|----------|------|-------------|---------------|
| **CurrentLocation** | String | 現在の場所名 | "" |
| **CurrentGridPosition** | Vector | 現在のグリッド座標(X,Y,0) | (0,0,0) |
| **TeamIndex** | Int | キャラクターのチーム番号 | 0 |
| **InventoryFull** | Bool | インベントリ満杯状態 | False |
| **TargetItem** | String | 採集/製作対象アイテム | "" |
| **TargetEnemy** | String | 戦闘対象敵 | "" |
| **TargetGridCell** | Vector | 移動先グリッドセル | (0,0,0) |
| **TaskResult** | Bool | 最終タスク実行結果 | False |
| **MovementInProgress** | Bool | 移動中フラグ | False |
| **CombatResult** | Bool | 戦闘結果 | False |
| **CraftingResult** | Bool | 製作結果 | False |
| **UnloadResult** | Bool | 荷下ろし結果 | False |

## 2. Behavior Tree Asset 作成 (BT_IdleCharacter)

### 作成手順
1. Content Browser → 右クリック → Artificial Intelligence → Behavior Tree
2. 名前: `BT_IdleCharacter`
3. 保存場所: `Content/AI/BT_IdleCharacter.uasset`
4. Blackboard Asset: `BB_IdleCharacter` を設定

### Behavior Tree 構造

```
BT_IdleCharacter
├── Root
    └── Composite: Selector "Main Decision Tree"
        ├── Composite: Sequence "Combat Branch"
        │   ├── Decorator: BTDecorator_HasCombatTask
        │   │   └── Blackboard Key: TargetEnemy
        │   ├── Task: BTTask_MoveToGridCell
        │   │   ├── Target Grid Cell Key: TargetGridCell
        │   │   └── Movement Result Key: TaskResult
        │   └── Task: BTTask_ExecuteCombat
        │       ├── Target Enemy Key: TargetEnemy
        │       └── Combat Result Key: CombatResult
        │
        ├── Composite: Sequence "Gathering Branch"
        │   ├── Decorator: BTDecorator_HasGatheringTask
        │   │   └── Blackboard Key: TargetItem
        │   ├── Task: BTTask_FindGatheringGrid
        │   │   ├── Target Item Key: TargetItem
        │   │   └── Target Grid Key: TargetGridCell
        │   ├── Task: BTTask_MoveToGridCell
        │   │   ├── Target Grid Cell Key: TargetGridCell
        │   │   └── Movement Result Key: TaskResult
        │   └── Task: BTTask_ExecuteGathering
        │       ├── Target Item Key: TargetItem
        │       └── Gathering Result Key: TaskResult
        │
        ├── Composite: Sequence "Crafting Branch"
        │   ├── Decorator: BTDecorator_HasCraftingTask
        │   │   └── Blackboard Key: TargetItem
        │   ├── Task: BTTask_MoveToGridCell (to base)
        │   │   ├── Target Grid Cell Key: TargetGridCell
        │   │   └── Movement Result Key: TaskResult
        │   └── Task: BTTask_ExecuteCrafting
        │       ├── Target Item Key: TargetItem
        │       └── Crafting Result Key: CraftingResult
        │
        ├── Composite: Sequence "Return to Base"
        │   ├── Decorator: BTDecorator_InventoryFull
        │   │   └── Full Threshold: 0.8
        │   ├── Task: BTTask_MoveToGridCell (to base)
        │   │   ├── Target Grid Cell Key: TargetGridCell
        │   │   └── Movement Result Key: TaskResult
        │   └── Task: BTTask_UnloadItems
        │       └── Unload Result Key: UnloadResult
        │
        └── Task: BTTask_Wait "Default Idle"
            └── Wait Time: 1.0
```

### Service 設定

**Root ノードに追加:**
- Service: `BTService_UpdateCurrentTask`
  - Interval: 0.5
  - Random Deviation: 0.1
  - Current Location Key: CurrentLocation
  - Team Index Key: TeamIndex
  - Inventory Full Key: InventoryFull
  - Current Grid Position Key: CurrentGridPosition

## 3. 各ノードの詳細設定

### BTDecorator_HasCombatTask
```
Node Name: "Has Combat Task"
Observer Aborts: Lower Priority
Flow Control:
  - Success: Continue
  - Failure: Skip to next branch
Target Enemy Key: TargetEnemy
```

### BTDecorator_HasGatheringTask
```
Node Name: "Has Gathering Task"
Observer Aborts: Lower Priority
Flow Control:
  - Success: Continue
  - Failure: Skip to next branch
Target Item Key: TargetItem
```

### BTDecorator_HasCraftingTask
```
Node Name: "Has Crafting Task"
Observer Aborts: Lower Priority
Flow Control:
  - Success: Continue
  - Failure: Skip to next branch
Target Item Key: TargetItem
```

### BTDecorator_InventoryFull
```
Node Name: "Inventory Full"
Observer Aborts: Lower Priority
Flow Control:
  - Success: Continue
  - Failure: Skip to next branch
Full Threshold: 0.8 (80% capacity)
```

### BTTask_MoveToGridCell
```
Node Name: "Move to Grid Cell"
Target Grid Cell Key: TargetGridCell
Movement Result Key: TaskResult
Max Movement Time: 10.0 seconds
Movement Speed: 500.0 units/sec
```

### BTTask_ExecuteGathering
```
Node Name: "Execute Gathering"
Target Item Key: TargetItem
Gathering Result Key: TaskResult
Max Gathering Time: 5.0 seconds
```

### BTTask_ExecuteCombat
```
Node Name: "Execute Combat"
Target Enemy Key: TargetEnemy
Combat Result Key: CombatResult
Max Combat Time: 10.0 seconds
```

### BTTask_ExecuteCrafting
```
Node Name: "Execute Crafting"
Target Item Key: TargetItem
Crafting Result Key: CraftingResult
Max Crafting Time: 5.0 seconds
```

### BTTask_UnloadItems
```
Node Name: "Unload Items"
Unload Result Key: UnloadResult
Max Unload Time: 3.0 seconds
```

### BTTask_Wait
```
Node Name: "Default Idle"
Wait Time: 1.0 seconds
Random Deviation: 0.2 seconds
```

### BTTask_FindGatheringGrid
```
Node Name: "Find Gathering Grid"
Target Item Key: TargetItem
Target Grid Key: TargetGridCell
Search Radius: 5 grid cells
Max Search Time: 2.0 seconds
```

## 4. AIController設定

### C++ AIController クラス設定
- Class: `AIdleAIController`
- Behavior Tree Asset: `BT_IdleCharacter`
- Blackboard Asset: `BB_IdleCharacter`

### Character設定
```cpp
// AC_IdleCharacter constructor
AIControllerClass = AIdleAIController::StaticClass();
AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
```

## 5. GameplayTags 設定

### DefaultGameplayTags.ini 追加
```ini
[/Script/GameplayTags.GameplayTagsSettings]
+GameplayTagList=(Tag="Location.Base",DevComment="拠点")
+GameplayTagList=(Tag="Location.Forest",DevComment="森")
+GameplayTagList=(Tag="Location.Plains",DevComment="平原")
+GameplayTagList=(Tag="Location.Mountain",DevComment="山")
+GameplayTagList=(Tag="Location.River",DevComment="川")
+GameplayTagList=(Tag="Resource.Wood",DevComment="木材")
+GameplayTagList=(Tag="Resource.Stone",DevComment="石材")
+GameplayTagList=(Tag="Resource.Food",DevComment="食料")
+GameplayTagList=(Tag="Task.Gathering",DevComment="採集タスク")
+GameplayTagList=(Tag="Task.Combat",DevComment="戦闘タスク")
+GameplayTagList=(Tag="Task.Crafting",DevComment="製作タスク")
+GameplayTagList=(Tag="State.Moving",DevComment="移動中")
+GameplayTagList=(Tag="State.Working",DevComment="作業中")
+GameplayTagList=(Tag="State.InCombat",DevComment="戦闘中")
```

## 6. テスト手順

### 1. 基本動作テスト
1. Character を配置
2. TimeManager を開始
3. BT が1秒毎に再開されることを確認
4. ログでターン処理を確認

### 2. タスク統合テスト
1. TaskManager にタスクを追加
2. キャラクターが適切なブランチを選択することを確認
3. タスク完了後の状態変化を確認

### 3. 移動テスト
1. GridMapComponent の設定確認
2. A*パスファインディングの動作確認
3. グリッド移動の視覚確認

### 4. UI統合テスト
1. 既存UI が正常に動作することを確認
2. Event Dispatcher が正常に発火することを確認
3. リアルタイム更新の動作確認

## 7. デバッグ設定

### Blueprint Debugger
- Behavior Tree デバッガーを有効化
- Blackboard 値の変化を監視
- BT ノードの実行フローを確認

### C++ デバッグログ
```cpp
// LogTemp カテゴリでログレベル設定
LogTemp.VeryVerbose = すべてのBT詳細ログ
LogTemp.Verbose = BT実行フローログ
LogTemp.Log = 重要なイベントログ
LogTemp.Warning = エラー・警告ログ
```

## 8. パフォーマンス考慮事項

### モバイル最適化
- BT再開頻度: 1秒（アイドルゲームに適切）
- Service更新頻度: 0.5秒（レスポンシブ性維持）
- LOD システムによる遠距離キャラクター最適化

### メモリ管理
- GridMap チャンクシステムによる空間分割
- 不要なBTインスタンスの自動クリーンアップ
- TWeakObjectPtr による循環参照回避

## 9. 実装完了確認リスト

- [ ] BB_IdleCharacter 作成・設定完了
- [ ] BT_IdleCharacter 作成・構造完了  
- [ ] 全BTTask/Decorator/Service 設定完了
- [ ] AIController 連携確認
- [ ] タスクマッチング動作確認
- [ ] グリッド移動動作確認
- [ ] UI統合動作確認
- [ ] パフォーマンステスト完了

**この仕様書に従ってBlueprint Editorで実際のアセットを作成することで、「毎ターン新しい判断」設計思想を完全実装できます。**