# AC_PlayerController 仕様書

## 概要

UE_Idleプロジェクトのメインプレイヤーコントローラークラス。グローバルインベントリとチーム管理機能を統合し、インターフェースによりキャストレスでBlueprint互換性を保ちながらC++で高速動作を実現。

## クラス構成

### 継承関係
```cpp
APlayerController                 IPlayerControllerInterface
└── AC_PlayerController ─────────→ (インターフェース実装)
```

### インターフェース設計
- **IPlayerControllerInterface** - キャストレスでのアクセスを提供
- **BlueprintImplementableEvent** - 自動的にC++実装を呼び出し

### 主要コンポーネント
- **UGlobalInventoryComponent** - アイテム管理システム
- **UTeamComponent** - キャラクター管理システム

## 主要機能

### 1. アイテム管理

#### AddItemToStorage（インターフェース経由）
```cpp
// インターフェース関数（BlueprintImplementableEvent）
UFUNCTION(BlueprintImplementableEvent, Category = "Player Controller Interface")
void AddItemToStorage(const FString& ItemId, int32 Quantity);

// C++実装関数
UFUNCTION(BlueprintCallable, Category = "Player Controller")
virtual void AddItemToStorage_Implementation(const FString& ItemId, int32 Quantity);
```
**機能**: 指定されたアイテムをグローバルストレージに追加  
**パラメータ**:
- `ItemId`: アイテムID（DataTableのRowName）
- `Quantity`: 追加数量

**使用例**:
```cpp
// C++（キャストレス）
if (auto* Interface = Cast<IPlayerControllerInterface>(GetPlayerController())) {
    Interface->Execute_AddItemToStorage(GetPlayerController(), "short_sword", 1);
}

// Blueprint（キャストレス）
Get Player Controller → AddItemToStorage("short_sword", 1)
```

#### GetGlobalInventoryComp
```cpp
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Controller")
UGlobalInventoryComponent* GetGlobalInventoryComp();
```
**機能**: グローバルインベントリコンポーネントの参照を取得  
**戻り値**: UGlobalInventoryComponent*

**使用例**:
```cpp
// C++
auto* Inventory = PlayerController->GetGlobalInventoryComp();
if (Inventory) {
    Inventory->GetAllStorageSlots();
}

// Blueprint
Get Global Inventory Comp -> Get All Storage Slots
```

### 2. チーム管理

#### AddCharacter
```cpp
UFUNCTION(BlueprintCallable, Category = "Player Controller")
void AddCharacter(AC_IdleCharacter* IdleCharacter);
```
**機能**: キャラクターをチームに追加  
**パラメータ**:
- `IdleCharacter`: 追加するキャラクターの参照

**使用例**:
```cpp
// C++
PlayerController->AddCharacter(NewCharacter);

// Blueprint
Add Character(Character Reference)
```

#### GetCharacterList
```cpp
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Controller")
TArray<AC_IdleCharacter*> GetCharacterList();
```
**機能**: 現在のチームキャラクター一覧を取得  
**戻り値**: TArray<AC_IdleCharacter*>

**使用例**:
```cpp
// C++
TArray<AC_IdleCharacter*> Team = PlayerController->GetCharacterList();
for (auto* Character : Team) {
    // キャラクター処理
}

// Blueprint
Get Character List -> For Each Loop
```

## コンポーネント詳細

### UGlobalInventoryComponent
**場所**: `Source/UE_Idle/Components/GlobalInventoryComponent.h`  
**機能**: 
- アイテムの共有ストレージ管理
- 所持金管理
- アイテムの追加・削除・検索
- イベント通知（OnStorageChanged, OnMoneyChanged）

### UTeamComponent
**場所**: `Source/UE_Idle/Components/TeamComponent.h`  
**機能**:
- キャラクター配列の管理
- キャラクターの追加・削除
- チーム情報の取得

```cpp
class UTeamComponent : public UActorComponent {
public:
    // キャラクターリスト
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    TArray<AC_IdleCharacter*> AllPlayerCharacters;
    
    // 主要機能
    void AddCharacter(AC_IdleCharacter* IdleCharacter);
    TArray<AC_IdleCharacter*> GetCharacterList() const;
    bool RemoveCharacter(AC_IdleCharacter* IdleCharacter);
    int32 GetCharacterCount() const;
};
```

## セットアップ

### コンストラクタ
```cpp
AC_PlayerController::AC_PlayerController() {
    // コンポーネント自動作成
    GlobalInventory = CreateDefaultSubobject<UGlobalInventoryComponent>(TEXT("GlobalInventory"));
    TeamComponent = CreateDefaultSubobject<UTeamComponent>(TEXT("TeamComponent"));
}
```

### 必要な依存関係
```cpp
#include "Components/GlobalInventoryComponent.h"
#include "Components/TeamComponent.h"
#include "Actor/C_IdleCharacter.h"
```

## インターフェースの利点

### キャストレスアクセス
```cpp
// 従来の方法（キャスト必要）
auto* CustomPC = Cast<AC_PlayerController>(GetPlayerController());
if (CustomPC) {
    CustomPC->AddItemToStorage("item", 1);
}

// インターフェース使用（キャストレス）
if (GetPlayerController()->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass())) {
    IPlayerControllerInterface::Execute_AddItemToStorage(GetPlayerController(), "item", 1);
}
```

### Blueprint互換性

| インターフェース関数 | 実装関数 | 説明 |
|-------------------|----------|------|
| AddItemToStorage | AddItemToStorage_Implementation | アイテム追加 |
| AddCharacter | AddCharacter_Implementation | キャラクター追加 |
| GetGlobalInventoryComp | GetGlobalInventoryComp_Implementation | インベントリ取得 |
| GetCharacterList | GetCharacterList_Implementation | キャラクター一覧取得 |

### 移行方法
1. **既存のBlueprint**: インターフェース関数が自動で表示される
2. **新規Blueprint**: GetPlayerControllerから直接関数呼び出し可能
3. **C++から呼び出し**: Execute_関数名を使用してキャストレス呼び出し

## 使用例

### ゲーム開始時のセットアップ（キャストレス）
```cpp
void AGameMode::StartPlay() {
    Super::StartPlay();
    
    // PlayerControllerをインターフェース経由で使用
    if (auto* PC = GetWorld()->GetFirstPlayerController()) {
        if (PC->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass())) {
            // 初期アイテム追加（キャストレス）
            IPlayerControllerInterface::Execute_AddItemToStorage(PC, "healing_potion", 5);
            IPlayerControllerInterface::Execute_AddItemToStorage(PC, "bread", 10);
            
            // 初期キャラクター追加
            if (auto* StartCharacter = SpawnCharacter()) {
                IPlayerControllerInterface::Execute_AddCharacter(PC, StartCharacter);
            }
        }
    }
}
```

### UI更新の例（インターフェース使用）
```cpp
void UInventoryWidget::RefreshInventory() {
    if (auto* PC = GetOwningPlayer()) {
        if (PC->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass())) {
            if (auto* Inventory = IPlayerControllerInterface::Execute_GetGlobalInventoryComp(PC)) {
                auto Slots = Inventory->GetAllStorageSlots();
                // UI更新処理
            }
        }
    }
}
```

### チーム管理の例（インターフェース使用）
```cpp
void UTeamWidget::UpdateTeamDisplay() {
    if (auto* PC = GetOwningPlayer()) {
        if (PC->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass())) {
            auto Team = IPlayerControllerInterface::Execute_GetCharacterList(PC);
            TeamSize = Team.Num();
            
            for (int32 i = 0; i < Team.Num(); ++i) {
                if (Team[i]) {
                    // キャラクター情報表示
                    DisplayCharacter(i, Team[i]);
                }
            }
        }
    }
}
```

## パフォーマンス

### インターフェース＋C++実装のメリット
- **キャストレス**: パフォーマンス向上（キャストのオーバーヘッドなし）
- **高速処理**: Blueprint実装より約3-5倍高速
- **メモリ効率**: 無駄なノード処理がない
- **デバッグ容易**: C++デバッガーでステップ実行可能
- **Blueprint互換**: GetPlayerControllerから直接アクセス可能

### 推奨使用方法
- **頻繁な処理**: インターフェース経由でキャストレス呼び出し
- **UI更新**: Blueprint/C++どちらでもキャストレス
- **イベント処理**: インターフェースイベントディスパッチャーを活用

## トラブルシューティング

### よくある問題

#### 1. コンポーネントがnullptr
```cpp
// 解決方法: null チェック
if (GlobalInventory) {
    GlobalInventory->AddItemToStorage(ItemId, Quantity);
}
```

#### 2. キャラクター重複追加
```cpp
// UTeamComponentで自動的に重複チェック実装済み
void UTeamComponent::AddCharacter(AC_IdleCharacter* IdleCharacter) {
    if (IdleCharacter && !AllPlayerCharacters.Contains(IdleCharacter)) {
        AllPlayerCharacters.Add(IdleCharacter);
    }
}
```

#### 3. Blueprint関数が見つからない
- プロジェクトを再コンパイル
- Blueprintエディターで「Compile」ボタンクリック
- "Player Controller"カテゴリを確認

## 今後の拡張予定

### 予定機能
- キャラクター並び替え機能
- チーム編成プリセット
- アイテム自動整理機能
- バックアップ・復元機能

### 拡張方法
```cpp
// 例: キャラクター並び替え
UFUNCTION(BlueprintCallable, Category = "Player Controller")
void ReorderCharacter(int32 FromIndex, int32 ToIndex);

// 例: チーム情報保存
UFUNCTION(BlueprintCallable, Category = "Player Controller")
void SaveTeamConfiguration(const FString& PresetName);
```

## 関連ドキュメント

- [アイテム仕様書](./アイテム仕様書.md)
- [CLAUDE.md](../CLAUDE.md) - プロジェクト全体仕様
- UGlobalInventoryComponent仕様書（予定）
- UTeamComponent仕様書（予定）
