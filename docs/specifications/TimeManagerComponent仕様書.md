# TimeManagerComponent仕様書

## 概要
`UTimeManagerComponent` はアイドルゲームにおける時間進行とタスク管理を統括するコアコンポーネントです。チームのタスク実行、リソース条件監視、タスク切り替えなどをタイマーベースで自動処理します。

## 基本機能

### 時間システム制御
- **開始/停止/一時停止/再開**: 時間システムの状態制御
- **タイマーベース処理**: 設定間隔での自動更新（デフォルト1秒間隔）
- **デバッグモード**: 高速処理モード（0.1倍速）

### 主要処理フロー
1. **全体タスク進行チェック** - グローバルタスクの監視
2. **チームタスク進行処理** - 各チームの状態別処理
3. **リソース条件監視** - 「全て」モードでの動的タスク切り替え
4. **タスク切り替え判定** - 遅延キューによる安全な切り替え
5. **時間イベント発信** - 1時間経過通知など

## クラス構造

### 主要プロパティ

#### 時間管理設定
```cpp
bool bTimeSystemActive;          // システムアクティブフラグ
float TimeUpdateInterval;        // 更新間隔（秒）
bool bFastProcessingMode;        // 高速処理モード
float IdleThreshold;             // 放置判定閾値（300秒 = 5分）
```

#### 安全性設定
```cpp
bool bEnableDefensiveProgramming;  // 安全チェック有効化
int32 MaxProcessingRetries;        // 最大リトライ回数
bool bProcessingTimeUpdate;        // 重複実行防止フラグ
```

#### 参照管理
```cpp
UTaskManagerComponent* TaskManager;        // タスクマネージャー参照
TArray<UTeamComponent*> TeamComponents;    // チームコンポーネント配列
TArray<FDelayedTaskSwitch> PendingTaskSwitches; // 遅延タスク切り替えキュー
```

#### 時間管理
```cpp
float CurrentGameTime;    // 現在のゲーム時間
float LastProcessTime;    // 最後の処理時刻
FTimerHandle TimeUpdateTimerHandle; // タイマーハンドル
```

#### 統計情報
```cpp
int32 TotalUpdatesProcessed;  // 総更新回数
int32 TaskSwitchesPerformed;  // タスク切り替え回数
float TotalProcessingTime;    // 総処理時間
```

## 主要メソッド

### 時間システム制御

#### `StartTimeSystem()`
- システム開始と初期化
- 参照の有効性チェック
- タイマー設定とイベント通知

#### `StopTimeSystem()`
- システム停止とクリーンアップ
- タイマークリアと遅延キューリセット

#### `PauseTimeSystem()` / `ResumeTimeSystem()`
- 一時停止と再開機能
- タイマーの状態管理

### 時間進行処理

#### `ProcessTimeUpdate()`
- **メイン処理ループ**
- 重複実行防止機能
- 安全モード対応
- 統計情報更新

#### `ProcessGlobalTasks()`
- 全体タスクの進行監視
- 完了状況の確認
- ログ出力（VeryVerboseレベル）

#### `ProcessTeamTasks()`
- 各チームの状態別処理
- 安全モードでの無効コンポーネント除去
- チーム状態に応じた分岐処理

#### `CheckResourceConditions()`
- 「全て」モードでのリソース条件監視
- 利用可能タスクの動的検出
- タスク切り替えキューへの追加

### 個別タスク処理

#### `ProcessAllModeTask(int32 TeamIndex)`
- 優先度順のタスク検索
- 次回実行可能タスクの決定
- 待機状態への移行処理

#### `ProcessSpecificTask(int32 TeamIndex, ETaskType TaskType)`
- 特定タスクタイプの処理
- 将来的な詳細実装予定

#### `MonitorLockedAction(int32 TeamIndex)`
- 中断不可能アクション監視
- 戦闘状態と一般ロック状態の分離処理
- 完了時の自動待機移行

### タスク切り替えロジック

#### `InterruptAndSwitchTask(int32 TeamIndex)`
- タスク中断可能性の判定
- 「全て」モードでの次タスク検索
- 安全なタスク切り替え実行

#### `StartTaskExecution(int32 TeamIndex, const FGlobalTask& Task)`
- タスク実行開始処理
- チーム状態の更新
- 推定完了時間の設定

#### `ProcessPendingTaskSwitches()`
- 遅延キューの安全な処理
- 切り替えタイプ別の分岐処理
- キューのクリア管理

### 戦闘終了処理

#### `OnCombatEndedHandler(int32 TeamIndex)`
- 戦闘終了イベントの受信
- 遅延キューへの安全な追加
- フレーム分離による安全処理

#### `ProcessPostCombatTask(int32 TeamIndex)`
- 戦闘後のタスク継続判定
- 「全て」モードでの優先度再チェック
- 冒険モードの継続処理

## 安全性機能

### Defensive Programming
- `ProcessTimeUpdateSafe()`: 安全チェック付き処理
- `ProcessTeamTaskSafe()`: チーム処理の二重チェック
- `MonitorCombatSafe()`: 戦闘監視の安全版
- `ProcessNormalTaskSafe()`: 通常タスクの安全処理

### エラーハンドリング
- 無効参照の自動除去
- 重複実行の防止
- リトライ機能（設定可能）
- 詳細なログ出力

## イベントシステム

### イベントディスパッチャー
```cpp
FOnTimeSystemStarted OnTimeSystemStarted;     // システム開始通知
FOnTimeSystemStopped OnTimeSystemStopped;     // システム停止通知
FOnHourPassed OnHourPassed;                   // 1時間経過通知
```

### 時間イベント
- **1時間経過**: ゲーム内1時間（3600秒）ごとの通知
- **デバッグモード**: 60秒ごとの通知
- **現在時刻**: ゲーム開始からの累積時間

## 設定・調整

### タイミング設定
- `TimeUpdateInterval`: 基本更新間隔（1.0秒推奨）
- `bFastProcessingMode`: デバッグ用高速モード
- `IdleThreshold`: 放置判定時間（300秒）

### 安全性設定
- `bEnableDefensiveProgramming`: 安全チェック有効化
- `MaxProcessingRetries`: エラー時のリトライ回数

## チーム状態との連携

### チーム状態別処理
- **Idle**: 新タスク検索と割り当て
- **Working**: 進行監視と完了判定
- **InCombat**: 戦闘監視と終了処理
- **Locked**: 中断不可アクションの監視

### タスク切り替えタイプ
- **PostCombat**: 戦闘終了後の自動切り替え
- **ResourceChange**: リソース条件変化による切り替え
- **Forced**: 強制的な状態リセット

## デバッグ・監視機能

### デバッグ出力
- `PrintDebugInfo()`: システム状態の詳細出力
- `GetProcessingStats()`: 統計情報の取得
- レベル別ログ出力（Log, Warning, Error, VeryVerbose）

### 統計情報
- 総更新回数
- タスク切り替え回数
- 累積処理時間
- 平均処理性能

## 使用上の注意

### セットアップ
1. `RegisterTaskManager()` でタスクマネージャーを登録
2. `RegisterTeamComponent()` でチームコンポーネントを登録
3. `StartTimeSystem()` でシステム開始

### パフォーマンス
- デフォルト1秒間隔での処理
- 大量のチーム数では負荷を考慮
- 安全モードは若干のオーバーヘッドあり

### エラー対応
- 無効なコンポーネント参照は自動的に除去
- エラー発生時は詳細ログで原因調査
- 重複実行は自動的に防止

## 将来の拡張予定
- スキル影響度を考慮したタスク効率計算
- より詳細な冒険モード処理
- オフライン進行計算機能
- カスタムタスクタイプの対応