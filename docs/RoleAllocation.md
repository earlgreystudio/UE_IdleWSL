# Role Allocation - UE_IdleWSL

各コンポーネントやアクターの役割を一目で把握できるよう簡潔にまとめています。

## Actor

### C_BP_GenerateCharacter
・部活や素質からランダムなキャラクターを生成する
・C_IdleCharacterをポップさせる
・プリセットからキャラクター生成も可能
・場所に応じた敵キャラクター生成機能

### C_IdleCharacter
・全てのキャラクターの元になるクラス
・CharacterStatusComponent、CharacterInventoryComponentを持つ
・IIdleCharacterInterfaceを実装

### C_PlayerController
・プレイヤーコントローラーのメインクラス
・GlobalInventoryComponent、TeamComponentを持つ
・IPlayerControllerInterfaceを実装
・プレイヤー固有の機能（チーム管理、インベントリ）に特化

### C_GameInstance
・カスタムGameInstanceクラス
・DataTableの初期化を管理
・全てのサブシステムへDataTableを配布

## Components

### CharacterInventoryComponent
・キャラクターの所持品と装備を管理する
・アイテムの受け渡し、装備、売却機能
・装備スロット管理とステータス計算

### CharacterStatusComponent
・キャラクターのステータス（HP、能力値）を管理
・部活動、才能データを保持
・各種ステータス変更イベントを発信

### GlobalInventoryComponent
・全キャラクター共有のストレージシステム
・アイテム管理とお金の管理
・キャラクター間でのアイテム受け渡し機能

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
・BattleSystemManagerを通じて冒険開始を委託

### CombatComponent
・戦闘システムの中核管理
・戦闘状態の管理と戦闘開始・終了処理
・ActionSystemComponentと連携して戦闘実行
・BattleSystemManagerによって管理される

### ActionSystemComponent
・戦闘中のキャラクター行動処理
・AI行動決定とターン管理
・攻撃・防御・回避の実行処理

### EventLogManager
・全ゲームイベントの統一ログ管理システム
・戦闘、採集、建築等のイベントログ記録
・自動サマリー生成機能（戦闘結果要約等）
・既存CombatLogManagerとの後方互換性維持
・UI表示用フォーマット済みテキスト提供

### LocationEventManager
・場所でのイベント発生管理
・戦闘イベントや採取イベントのトリガー
・場所別の敵チーム生成機能
・CharacterPresetManagerとCombatComponentとの連携
・敵チームのライフサイクル管理（生成・削除）

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
・LocationEventManagerとCombatComponentを保持
・チームの冒険開始処理
・PlayerControllerから戦闘関連の責任を分離

### CombatCalculator
・戦闘計算の専門クラス
・ダメージ、命中率、回避率の計算
・装備重量ペナルティの計算

## CharacterGenerator

### ClubSystem
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

## UI

### C_CharacterList
・キャラクター一覧表示のメインUIウィジェット
・PlayerControllerからキャラクター配列を取得
・C_CharacterCardを動的生成してPanelWidgetに配置
・キャラクターリスト変更の自動検知と更新機能

### C_CharacterCard
・個別キャラクターの基本情報表示カード
・キャラクター名、部活、DPS、防御力、HPを表示
・HP、スタミナ、メンタルのプログレスバー表示
・CharacterStatusComponentの変更イベントに対応

### C_CharacterSheet
・キャラクターの詳細ステータス表示ウィジェット
・全能力値、戦闘ステータス、作業能力を数値表示
・キャラクター特性配列とスキル配列をテキスト表示
・Enumの表示名変換機能を内蔵