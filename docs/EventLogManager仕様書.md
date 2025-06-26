# EventLogManager 仕様書

## 概要

UE_Idleプロジェクトのイベントログ管理システム。戦闘、採集、建築など全ゲームイベントを統一的に記録・管理し、後方互換性を保ちながら新しいサマリー機能を提供。

## クラス構成

### 継承関係
```cpp
UActorComponent
└── UEventLogManager (Component)
```

### 主要機能
- **イベントログ記録**: 全ゲームイベントの統一記録システム
- **戦闘ログ**: 既存の戦闘システムとの後方互換性
- **イベントサマリー**: 戦闘結果等の要約自動生成
- **ログフィルタリング**: カテゴリ・タイプ・時間による柔軟な検索
- **UI支援**: フォーマット済みテキストの自動生成

## データ構造

### FEventLogEntry（個別ログエントリ）
```cpp
struct FEventLogEntry
{
    float Timestamp;                    // イベント発生時刻
    EEventCategory EventCategory;       // Combat/Gathering/Construction等
    EEventLogType EventType;           // Hit/Miss/Success等の詳細タイプ
    EEventPriority Priority;           // ログの重要度
    FString FormattedText;             // UI表示用テキスト
    
    // イベント別詳細データ
    FCombatEventData CombatData;       // 戦闘：攻撃者・防御者・ダメージ等
    FGatheringEventData GatheringData; // 採集：採集者・資源・場所等
    FConstructionEventData ConstructionData; // 建築：建物・進捗等
};
```

### FEventSummary（イベント要約）
```cpp
struct FEventSummary
{
    EEventCategory EventCategory;      // イベントカテゴリ
    FString Title;                     // タイトル（例：「チーム1が森で敵と戦闘」）
    FString ResultText;                // 結果（例：「チーム1の勝利」）
    bool bIsSuccess;                   // 成功フラグ
    float StartTime;                   // 開始時刻
    float EndTime;                     // 終了時刻
    TArray<FEventLogEntry> DetailedLogs; // 詳細ログ一覧
};
```

## 主要機能

### 1. 戦闘ログ機能（後方互換性）

#### 基本戦闘ログ
```cpp
// 既存インターフェース（CombatLogManagerと同一）
void AddCombatLog(ECombatLogType CombatLogType, AC_IdleCharacter* Actor, 
                 AC_IdleCharacter* Target = nullptr, 
                 const FString& WeaponOrItemName = TEXT(""), 
                 int32 DamageValue = 0, 
                 const FString& AdditionalInfo = TEXT(""));
```

#### 戦闘計算結果の自動ログ化
```cpp
// FCombatCalculationResultから自動でログ生成
void AddCombatCalculationLog(AC_IdleCharacter* Attacker, AC_IdleCharacter* Defender, 
                            const FString& WeaponName, 
                            const FCombatCalculationResult& Result);
```

#### 戦闘開始・終了の特別処理
```cpp
// 戦闘開始：チーム情報を記録し、サマリー作成準備
void LogCombatStart(const TArray<AC_IdleCharacter*>& AllyTeam, 
                   const TArray<AC_IdleCharacter*>& EnemyTeam, 
                   const FString& LocationName);

// 戦闘終了：勝敗判定し、自動でサマリー作成
void LogCombatEnd(const TArray<AC_IdleCharacter*>& Winners, 
                 const TArray<AC_IdleCharacter*>& Losers, 
                 float CombatDuration);
```

### 2. 新統一イベントログ機能

#### 汎用イベントログ追加
```cpp
void AddEventLog(const FEventLogEntry& EventEntry);
```

#### イベント別専用関数
```cpp
// 戦闘イベント（HPも自動取得）
void AddCombatEvent(EEventLogType EventType, AC_IdleCharacter* Attacker, 
                   AC_IdleCharacter* Defender, const FString& WeaponName, 
                   int32 Damage = 0, bool bIsCritical = false);

// 採集イベント
void AddGatheringEvent(EEventLogType EventType, AC_IdleCharacter* Gatherer, 
                      const FString& ResourceType, int32 Amount, 
                      const FString& LocationName, float SuccessRate = 100.0f);

// 建築イベント
void AddConstructionEvent(EEventLogType EventType, const FString& BuildingName, 
                         const FString& BuilderName, float Progress);
```

### 3. イベントサマリー機能

#### サマリー作成
```cpp
// 手動サマリー作成
void CreateEventSummary(EEventCategory Category, const FString& Title, 
                       const FString& ResultText, bool bSuccess = true);

// 現在のログからサマリー生成
FEventSummary GenerateCurrentEventSummary(EEventCategory Category, 
                                         const FString& Title) const;
```

#### 自動サマリー生成
- `bAutoCreateSummaries = true`の場合、戦闘終了時に自動生成
- タイトル形式：「{味方チーム}が{場所}で{敵チーム}と戦闘」
- 結果形式：「{勝利チーム}の勝利」または「{敗者チーム}が敗北」

### 4. ログ取得・検索機能

#### 基本取得
```cpp
TArray<FEventLogEntry> GetAllLogs() const;
TArray<FEventLogEntry> GetRecentLogs(int32 Count) const;
TArray<FEventSummary> GetAllEventSummaries() const;
```

#### カテゴリ・タイプ別取得
```cpp
TArray<FEventLogEntry> GetLogsByCategory(EEventCategory Category) const;
TArray<FEventLogEntry> GetLogsByType(EEventLogType EventType) const;
```

#### 高度フィルタリング
```cpp
// FEventLogFilter構造体によるフィルタリング
TArray<FEventLogEntry> GetFilteredLogs(const FEventLogFilter& Filter) const;
```

### 5. UI支援機能

#### フォーマット済みテキスト取得
```cpp
// ログのフォーマット済みテキスト取得
TArray<FString> GetFormattedLogs(int32 RecentCount = -1) const;

// サマリーのフォーマット済みテキスト取得
TArray<FString> GetFormattedEventSummaries(int32 RecentCount = -1) const;
```

#### 戦闘ログの特別フォーマット
```
// Hit/Criticalの場合の表示例：
レイナ(85/100) <===[VS]===> ゴブリン戦士(45/60)
          ショートソード攻撃 クリティカル！→ （23）

// Miss/Dodgeの場合の表示例：
レイナがショートソードでゴブリン戦士を攻撃 → 回避された
```

## 設定プロパティ

### 基本設定
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
int32 MaxLogEntries = 1000;           // 最大ログ保持数

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
int32 MaxSummaryEntries = 100;        // 最大サマリー保持数

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
bool bAutoFormatLogs = true;           // 自動フォーマット有効

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
bool bAutoCreateSummaries = true;      // 自動サマリー作成有効
```

## イベントディスパッチャー

### 後方互換性用
```cpp
UPROPERTY(BlueprintAssignable, Category = "Combat Log Events")
FOnCombatLogAddedEvent OnCombatLogAdded;

UPROPERTY(BlueprintAssignable, Category = "Combat Log Events")
FOnCombatLogClearedEvent OnCombatLogCleared;
```

### 新しいイベント
```cpp
UPROPERTY(BlueprintAssignable, Category = "Event Log Events")
FOnEventLogAdded OnEventLogAdded;

UPROPERTY(BlueprintAssignable, Category = "Event Log Events")
FOnEventSummaryCreated OnEventSummaryCreated;

UPROPERTY(BlueprintAssignable, Category = "Event Log Events")
FOnEventLogCleared OnEventLogCleared;
```

## 内部処理詳細

### ログ自動整理
- ログ数が`MaxLogEntries`を超えると古いものから削除
- サマリー数が`MaxSummaryEntries`を超えると古いものから削除

### キャラクター名取得
- `IIdleCharacterInterface`経由で安全に名前取得
- インターフェース未実装の場合は「名無し」を返す

### HP情報取得
- `UCharacterStatusComponent`から現在HP・最大HPを取得
- 安全性のためtry-catch構造とIsValid()チェック
- 取得失敗時はデフォルト値（100/100）を設定

### 戦闘ログ変換
- 既存の`ECombatLogType`を新しい`EEventLogType`に変換
- レガシーデータも保持して完全な後方互換性を確保

## Blueprint使用例

### 戦闘ログ追加
```
// 攻撃ヒット時
EventLogManager->AddCombatLog(Hit, Attacker, Defender, "ショートソード", 25, "");

// 戦闘計算結果から自動ログ化
EventLogManager->AddCombatCalculationLog(Attacker, Defender, "ショートソード", CombatResult);
```

### サマリー確認
```
// 最新10件のサマリー取得
TArray<FEventSummary> RecentSummaries = EventLogManager->GetRecentEventSummaries(10);

// UI表示用フォーマット済みテキスト
TArray<FString> FormattedSummaries = EventLogManager->GetFormattedEventSummaries(10);
```

### イベントバインド
```
// Blueprint側でイベントバインド
EventLogManager->OnEventSummaryCreated.AddDynamic(this, &UMyWidget::OnNewSummaryCreated);
```

## 注意事項

### パフォーマンス
- 大量のログ生成時は`MaxLogEntries`を適切に設定
- UI更新頻度を考慮してイベントバインドを最適化

### メモリ管理
- DetailedLogsはサマリー作成時にコピーされるため、メモリ使用量に注意
- 長時間のゲームセッションでは定期的なクリアを検討

### デバッグ
- 戦闘関連は詳細なUE_LOGを出力（LogTemp, Error/Log/Warning）
- サマリー作成プロセスは特に詳細にロギング

## 拡張可能性

### 新しいイベントタイプ追加
1. `EEventLogType`に新しい値追加
2. 対応する`FEventData`構造体作成（必要に応じて）
3. `FormatLogEntry()`に新しいフォーマット処理追加
4. `DetermineEventCategory()`に分類ロジック追加

### 新しいフィルタリング条件
1. `FEventLogFilter`に新しいプロパティ追加
2. `GetFilteredLogs()`のラムダ式に条件追加

この統一システムにより、ゲーム内の全イベントが一貫した方法で記録・管理され、UI表示からデバッグまで幅広く活用できます。