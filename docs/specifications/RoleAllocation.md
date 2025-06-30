# Role Allocation - UE_IdleWSL

各コンポーネントやアクターの役割を一目で把握できるよう簡潔にまとめています。

## 設計原則

### 時間管理の統一原則
・**TimeManagerComponent が唯一の時間管理者**（単一タイマー原則）
・**全ての時間ベース処理は TimeManager からのトリガーで実行**
・**他のコンポーネントは独立タイマーを持たない**
・将来の更新頻度変更時は TimeManager のみ修正

### 責任分担の明確化
```
TimeManagerComponent（時間統括）
↓ 毎秒トリガー
TaskManagerComponent（チームタスク判定・対象決定）
↓ 判定結果に応じて処理委譲
├─ 採集タスク → GatheringComponent::ProcessGathering()
├─ 戦闘タスク → CombatComponent::ProcessCombat()
├─ 製作タスク → CraftingComponent::ProcessCrafting()
└─ 建築タスク → ConstructionComponent::ProcessConstruction()
```

### カプセル化原則
・**TaskManager**: 「何をすべきか」の判定
・**各専門Component**: 「どのように実行するか」の実装
・**目的外処理の禁止**: 指定されたタスクのみ実行（例：木タスクなら木のみ採集）

### ターンベース設計原則（新追加）
・**毎ターン完全再計算**: 状態機械を排除し、毎回フレッシュな判定実行
・**チームタスク優先**: チームタスクの優先度がグローバルタスクより優先
・**状態キャッシュなし**: 前回状態は保持せず、常に最新データで判定
・**確実な変化対応**: タスク削除・追加・在庫変化への即座反映

## Actor

### C_BP_GenerateCharacter
・部活や素質からランダムなキャラクターを生成する
・C_IdleCharacterをポップさせる
・プリセットからキャラクター生成も可能
・場所に応じた敵キャラクター生成機能

### C_IdleCharacter
・全てのキャラクターの元になるクラス
・CharacterStatusComponent、InventoryComponent（キャラクター設定）を持つ
・IIdleCharacterInterfaceを実装

### C_PlayerController
・プレイヤーコントローラーのメインクラス
・InventoryComponent（ストレージ設定）、TeamComponent、BaseComponentを持つ
・IPlayerControllerInterfaceを実装
・プレイヤー固有の機能（チーム管理、インベントリ、拠点管理）に特化
・タスク管理システム（TaskManager、TimeManager、CraftingComponent）統合
・拠点管理システム（BaseComponent）統合

### C_GameInstance
・カスタムGameInstanceクラス
・DataTableの初期化を管理
・全てのサブシステムへDataTableを配布

## Components

### InventoryComponent
・統一されたインベントリシステム
・設定に応じてキャラクター用またはストレージ用として動作
・アイテム管理、装備管理、お金管理の統合機能
・装備スロット管理とステータス計算
・キャラクター間でのアイテム受け渡し機能
・後方互換性のため旧コンポーネントのメソッド名も保持

### CharacterStatusComponent
・キャラクターのステータス（HP、能力値）を管理
・部活動、才能データを保持
・各種ステータス変更イベントを発信

### BaseComponent
・拠点システムの中核管理コンポーネント
・PlayerControllerに付与して拠点全体を管理
・リソース管理、人口管理、建設・アップグレード処理
・ワーカー配置と生産効果の計算
・FacilityManagerとの連携で施設操作を実行

### TeamComponent
・キャラクターのチーム編成管理
・複数チームの作成と任務割り当て
・チームメンバーの追加・削除機能
・**タスク削除時の自動アイドル状態リセット（新機能）**
・**UI状態とタスクリスト状態の同期保証**
・BattleSystemManagerを通じて冒険開始を委託
・TimeManagerComponentとの連携によるタスク実行状態管理

### TaskManagerComponent
・グローバルタスク（全チーム共通目標）の管理システム
・タスクの追加・削除・優先度管理・進行状況追跡
・「全て」モード用のタスク自動選択機能
・リソース要件チェックとタスク実行可能性判定
・**チームタスク優先度ベースマッチングシステム（新機能）**
・**毎ターン完全再計算によるタスク選択（ターンベース化）**
・**3つの採集数量タイプ対応（無制限・Keep・指定）**
・**Keep タスクの拠点全体在庫量判定（新機能）**
・**チーム毎のタスク判定と対象アイテム決定（TimeManagerからの委譲）**
・**個別チームの実行すべきタスクとターゲットの特定**
・**タスクマッチング用ヘルパー関数群（拡張可能設計）**
・安全性確保のための排他制御とエラーハンドリング
・イベントディスパッチャーによるUI連携

### TimeManagerComponent
・**全ての時間管理の統括者（唯一のタイマー管理者）**
・タイマーベースでの自動時間更新処理（デフォルト1秒間隔、将来変更可能）
・**全コンポーネントへの処理トリガー発行（単一起点原則）**
・**ProcessTimeUpdate() → TaskManager判定 → 各コンポーネント実行の統制**
・全体タスク進行とチームタスク進行の監視・制御
・リソース条件変化の検出と動的タスク切り替え
・戦闘状態の監視と戦闘終了後の安全なタスク復帰
・遅延キューによる安全なタスク切り替え機能
・「全て」モードでの動的タスク選択と実行
・TaskManagerとTeamComponentsの統合管理
・**各専門コンポーネントの協調制御（Gathering/Combat/Crafting等）**
・デバッグ用高速処理モードと詳細ログ出力

### CombatComponent
・戦闘システムの中核管理
・戦闘状態（Inactive/Preparing/InProgress/Ending）の管理
・戦闘開始条件の検証と戦闘セットアップ
・ActionSystemComponentの制御（開始・停止）
・戦闘終了処理とイベント通知
・LocationEventManagerとの連携による敵チーム管理
・BattleSystemManagerによって管理される

### ActionSystemComponent
・戦闘中のキャラクター行動処理
・AI行動決定とターン管理
・攻撃・防御・回避の実行処理
・独自タイマーによる行動実行（即座実行モード対応）
・アクションキューの管理（味方・敵別）
・戦闘ログの記録とEventLogManagerへの通知

### EventLogManager
・全ゲームイベントの統一ログ管理システム
・戦闘、採集、建築等のイベントログ記録
・自動サマリー生成機能（戦闘結果要約等）
・既存CombatLogManagerとの後方互換性維持
・UI表示用フォーマット済みテキスト提供

### GatheringComponent
・**採集処理の実装専門コンポーネント（タイマー機能なし）**
・**TimeManagerからのトリガーによる指定アイテムのみ採集処理**
・移動と採集の状態管理（MovingToSite/Gathering/MovingToBase/Unloading）
・GatheringPowerベースの採取量計算と確率判定
・運搬キャラ優先のアイテム配分ロジック
・Resourceカテゴリアイテムの自動荷下ろし
・距離ベースの移動システム
・**TaskManagerが決定した対象アイテムのみを採集（目的外アイテム無視）**
・**タスク完了時の自動拠点帰還機能**
・**独立タイマーなし - 全てTimeManagerComponent統制下で動作**

### CraftingComponent
・**製作処理の実装専門コンポーネント（タイマー機能なし）**
・**TimeManagerからのトリガーによる指定レシピのみ製作処理**
・製作状態管理とレシピ実行処理
・素材消費と完成品生成のロジック
・製作時間とスキル要件の計算
・**TaskManagerが決定した対象レシピのみを製作**
・**独立タイマーなし - 全てTimeManagerComponent統制下で動作**

### LocationMovementComponent
・**場所間移動の専門コンポーネント（汎用移動システム）**
・**TimeManagerからのトリガーによる移動処理**
・距離ベースの移動計算と進捗管理（0.0-1.0）
・チーム移動速度の計算（メンバー能力・装備・運搬状況考慮）
・移動完了時のイベント発行（各専門コンポーネントに通知）
・**採集・冒険・調教・交易等の全タスクで共通利用**
・移動状態管理（MovingToDestination/MovingToBase/Stationary）
・**残り時間表示機能（分：秒形式）でUI連携**
・**独立タイマーなし - TimeManagerComponent統制下で動作**

### LocationEventManager
・場所でのイベント発生管理
・戦闘イベントや採取イベントのトリガー
・場所別の敵チーム生成機能（DataTableベース）
・CharacterPresetManagerとの連携による敵キャラクター生成
・CombatComponentへの戦闘開始依頼
・敵チームのライフサイクル管理（生成・削除）
・場所データの取得と検証機能（採集データ含む）
・BattleSystemManagerによって保持・管理される

## Managers

### ItemDataTableManager (GameInstanceSubsystem)
・DataTableを使用したアイテム管理システム
・アイテムデータの取得と検索機能
・品質修正値の計算とアイテム評価

### FacilityManager (GameInstanceSubsystem)
・拠点施設システムの管理サブシステム
・DataTableベースの施設データ管理
・施設インスタンスの生成・削除・状態管理
・建設・アップグレード・依存関係の処理
・施設効果の計算とアンロック判定

### CharacterPresetManager (GameInstanceSubsystem)
・キャラクタープリセットの管理
・プリセットからのキャラクター生成
・場所データの管理も兼任

### BattleSystemManager (GameInstanceSubsystem)
・戦闘システム全体の統括管理
・LocationEventManagerとCombatComponentを保持・管理
・チームの冒険開始処理（TeamComponentから委譲）
・戦闘イベントのトリガーと場所データ管理
・敵チーム生成と戦闘開始の仲介
・PlayerControllerから戦闘関連の責任を分離
・戦闘終了時のクリーンアップ処理

### CombatCalculator
・戦闘計算の専門クラス
・ダメージ、命中率、回避率の計算
・装備重量ペナルティの計算

## CharacterGenerator

### SpecialtySystem
・ランダムな部活動選択機能
・部活動タイプの提供

### CharacterTalentGenerator
・ランダムな才能値生成
・部活動による才能ボーナス適用

### CharacterStatusManager
・才能値から最大ステータス値を計算
・キャラクター能力値の決定

## Interfaces

### IIdleCharacterInterface
・キャラクター基本情報の統一インターフェース
・名前、ステータス、インベントリ取得機能
・キャラクター間の共通操作を定義

### IPlayerControllerInterface
・プレイヤーコントローラーの統一インターフェース
・アイテム管理、キャラクター管理の操作
・グローバルストレージとチーム管理へのアクセス

## UI

### C_CharacterList
・キャラクターカードのメインコンテナウィジェット
・キャラクターリストを自動取得・表示
・TeamComponentのイベントと連携した自動更新機能
・キャラクターカードの動的生成と管理

### C_CharacterCard
・個別キャラクター情報表示カード
・名前、専門、DPS、防御力、体力を表示
・プログレスバーで体力・スタミナ・メンタル状態を視覚化
・各UIパネル間で移動可能な設計（不変性維持）

### C_CharacterSheet
・キャラクターの詳細ステータス表示
・全属性、戦闘能力、作業能力を日本語ラベル付きで表示
・特性とスキルの一覧表示機能
・ExposeOnSpawn対応でBlueprint連携強化

### C_TeamList
・全チーム表示用のメインコンテナウィジェット
・WrapBoxレイアウトによるチームカード配置
・チーム作成ボタンとチーム管理機能
・TeamComponentイベントとの連携による動的更新

### C_TeamCard
・個別チーム情報表示カード
・チーム名、現在タスク、状態、メンバー数表示
・キャラクターカード（WrapBoxコンテナ）とタスクカードの管理
・チーム状態に応じた動的UI更新

### C_TeamTaskMakeSheet
・チーム用タスク作成シート
・シンプルなチーム別タスク作成UI（基本選択：タスクタイプのみ）
・冒険時のみ目的地選択が追加表示
・FTeamTask構造体ベースのタスク作成
・C_TeamCard内のボタンから起動

### C_BaseFacilityList
・拠点設備一覧表示ウィジェット
・PlayerControllerのBaseComponentを自動検出して初期化
・設備タイプ・状態別フィルタリング機能
・設備総数・稼働中数のサマリー表示
・FacilityManager/BaseComponentイベント連携による自動更新

### C_FacilityCard
・個別設備詳細情報カード（表示専用設計）
・設備名、レベル、タイプ、状態、耐久度、ワーカー情報表示
・継続効果、アンロック効果、自動生産、コスト情報の動的表示
・状態別色分け表示（計画中/建設中/稼働中/損傷/破壊/アップグレード中）
・アップグレード・建設は全体タスクシステムで実行（UI上はアクションボタンなし）

## Types

### ItemDataTable
・アイテムの構造体定義とenumリスト
・DataTable用のアイテムデータ行構造

### FacilityDataTable
・拠点施設の構造体定義とenumリスト
・施設タイプ、状態、効果タイプの定義
・DataTable用の施設データ行構造
・建設コスト、アップグレードコスト、依存関係の管理
・施設インスタンスとエフェクト構造の定義

### CharacterTypes
・キャラクター関連の構造体とenum
・ステータス、才能、部活動の定義

### CombatTypes
・戦闘システム関連の構造体とenum
・戦闘結果、ログ、行動データの定義

### TeamTypes
・チーム管理関連の構造体とenum
・チーム情報と任務タイプの定義
・チーム状態管理（Idle/Working/InCombat/Locked）
・戦闘状態とアクション中断可能性の判定機能
・運搬手段とアクション時間管理

### TaskTypes
・タスク管理システム用の構造体とenum
・グローバルタスクとチームタスクの構造定義
・タスク切り替えタイプ（PostCombat/ResourceChange/Forced）
・遅延タスク切り替えとスナップショット機能
・タスク関連スキル情報とユーティリティ関数
・イベントディスパッチャー用デリゲート宣言

### LocationTypes
・場所関連の構造体定義
・場所データと敵出現データの管理

### EventLogTypes
・イベントログシステム用の構造体とenum
・戦闘・採集・建築等のイベントデータ構造
・ログフィルタリングとサマリー構造の定義

### CharacterPresetTypes
・キャラクタープリセット用の構造体
・プリセットデータ行の定義

### ItemTypes
・旧JSON用アイテム構造体（非推奨）
・DataTable移行により使用終了予定