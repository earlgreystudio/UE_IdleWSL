# AC_IdleCharacter 仕様書

## 概要

UE_Idleプロジェクトのメインキャラクタークラス。IIdleCharacterInterfaceを実装し、Blueprintからキャストレスでアクセス可能な高速なキャラクター管理機能を提供。

## クラス構成

### 継承関係
```cpp
AActor                           IIdleCharacterInterface
└── AC_IdleCharacter ─────────→ (インターフェース実装)
```

### インターフェース設計
- **IIdleCharacterInterface** - キャストレスでのアクセスを提供
- **BlueprintNativeEvent** - C++実装をBlueprint経由で呼び出し

### 主要コンポーネント
- **UCharacterStatusComponent** - ステータス・才能・部活動データ管理
- **UCharacterInventoryComponent** - 個人インベントリ管理

## 利用可能なBlueprintインターフェース関数

### 1. 基本キャラクター情報

#### GetCharacterName
```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
FString GetCharacterName();
```
**機能**: キャラクターの名前を取得  
**戻り値**: FString（キャラクター名）  
**実装**: CharacterNameプロパティを返す

**使用例**:
```cpp
// C++（キャストレス）
if (auto* Interface = Cast<IIdleCharacterInterface>(Character)) {
    FString Name = Interface->Execute_GetCharacterName(Character);
}

// Blueprint（キャストレス）
Character Reference → GetCharacterName
```

#### IsActive
```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
bool IsActive();
```
**機能**: キャラクターがアクティブ状態かチェック  
**戻り値**: bool（アクティブ状態）  
**実装**: bIsActiveプロパティを返す

### 2. コンポーネントアクセス

#### GetCharacterStatusComponent
```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
UCharacterStatusComponent* GetCharacterStatusComponent();
```
**機能**: ステータスコンポーネントの参照を取得  
**戻り値**: UCharacterStatusComponent*  
**実装**: StatusComponentプロパティを返す

**使用例**:
```cpp
// C++
auto* StatusComp = IIdleCharacterInterface::Execute_GetCharacterStatusComponent(Character);
if (StatusComp) {
    FCharacterStatus Status = StatusComp->GetStatus();
}

// Blueprint
Character Reference → GetCharacterStatusComponent → GetStatus
```

#### GetCharacterInventoryComponent
```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
UCharacterInventoryComponent* GetCharacterInventoryComponent();
```
**機能**: インベントリコンポーネントの参照を取得  
**戻り値**: UCharacterInventoryComponent*  
**実装**: InventoryComponentプロパティを返す

### 3. 詳細データアクセス

#### GetCharacterTalent
```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
FCharacterTalent GetCharacterTalent();
```
**機能**: キャラクターの才能データを取得  
**戻り値**: FCharacterTalent構造体  
**実装**: StatusComponentからTalentデータを取得、コンポーネントがnullの場合はデフォルト値を返す

**使用例**:
```cpp
// C++
FCharacterTalent Talent = IIdleCharacterInterface::Execute_GetCharacterTalent(Character);
float Strength = Talent.Strength;

// Blueprint
Character Reference → GetCharacterTalent → Break FCharacterTalent → Strength値使用
```

## コンポーネント詳細

### UCharacterStatusComponent
**場所**: `Source/UE_Idle/Components/CharacterStatusComponent.h`  
**管理データ**:
- **FCharacterStatus Status** - HP、スタミナ、精神力などの変動ステータス
- **EClubType ClubType** - 所属部活動
- **FCharacterTalent Talent** - 基本能力値（筋力、体力、知力、器用さ、敏捷、精神力）とスキル

**主要機能**:
```cpp
// ステータス関連
FCharacterStatus GetStatus() const;
void SetStatus(const FCharacterStatus& NewStatus);
float GetCurrentHealth() const;
float GetMaxHealth() const;

// 部活動関連
EClubType GetClubType() const;
void SetClubType(EClubType NewClubType);

// 才能関連
FCharacterTalent GetTalent() const;
void SetTalent(const FCharacterTalent& NewTalent);
```

### UCharacterInventoryComponent
**場所**: `Source/UE_Idle/Components/CharacterInventoryComponent.h`  
**機能**:
- 個人所持アイテム管理
- 装備品管理
- アイテムの追加・削除・検索

## セットアップ

### コンストラクタ
```cpp
AC_IdleCharacter::AC_IdleCharacter() {
    PrimaryActorTick.bCanEverTick = true;
    
    // コンポーネント自動作成
    StatusComponent = CreateDefaultSubobject<UCharacterStatusComponent>(TEXT("StatusComponent"));
    InventoryComponent = CreateDefaultSubobject<UCharacterInventoryComponent>(TEXT("InventoryComponent"));
}
```

### 必要な依存関係
```cpp
#include "../Interfaces/IdleCharacterInterface.h"
#include "../Components/CharacterStatusComponent.h"
#include "../Components/CharacterInventoryComponent.h"
```

## データ構造

### FCharacterTalent（才能データ）
```cpp
struct FCharacterTalent {
    // 基本能力値 (1-30)
    float Strength;     // 筋力
    float Toughness;    // 体力
    float Intelligence; // 知力
    float Dexterity;    // 器用さ
    float Agility;      // 敏捷
    float Willpower;    // 精神力
    
    // スキル (1-4個, 各1-20)
    TArray<FSkillTalent> Skills;
};
```

### EClubType（部活動種類）
- Baseball（野球部）
- Kendo（剣道部）
- Chemistry（化学部）
- など38種類の部活動

## 使用例

### キャラクター情報取得（キャストレス）
```cpp
void AGameManager::DisplayCharacterInfo(AActor* Character) {
    if (Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass())) {
        // 基本情報取得
        FString Name = IIdleCharacterInterface::Execute_GetCharacterName(Character);
        bool IsActive = IIdleCharacterInterface::Execute_IsActive(Character);
        
        // 才能データ取得
        FCharacterTalent Talent = IIdleCharacterInterface::Execute_GetCharacterTalent(Character);
        
        UE_LOG(LogTemp, Warning, TEXT("Character: %s, Strength: %.1f"), 
               *Name, Talent.Strength);
    }
}
```

### UI更新の例（インターフェース使用）
```cpp
void UCharacterWidget::UpdateCharacterDisplay(AActor* Character) {
    if (Character->GetClass()->ImplementsInterface(UIdleCharacterInterface::StaticClass())) {
        // 名前表示
        NameText->SetText(FText::FromString(
            IIdleCharacterInterface::Execute_GetCharacterName(Character)
        ));
        
        // 才能表示
        FCharacterTalent Talent = IIdleCharacterInterface::Execute_GetCharacterTalent(Character);
        StrengthText->SetText(FText::AsNumber(Talent.Strength));
        IntelligenceText->SetText(FText::AsNumber(Talent.Intelligence));
    }
}
```

### ステータス管理の例
```cpp
void AGameManager::ModifyCharacterStats(AActor* Character, float StrengthBonus) {
    if (auto* StatusComp = IIdleCharacterInterface::Execute_GetCharacterStatusComponent(Character)) {
        FCharacterTalent CurrentTalent = StatusComp->GetTalent();
        CurrentTalent.Strength += StrengthBonus;
        StatusComp->SetTalent(CurrentTalent);
    }
}
```

## パフォーマンス

### インターフェース＋C++実装のメリット
- **キャストレス**: Cast<AC_IdleCharacter>()のオーバーヘッドなし
- **高速処理**: Blueprint実装より約3-5倍高速
- **メモリ効率**: 直接関数呼び出しでノードオーバーヘッドなし
- **デバッグ容易**: C++デバッガーでステップ実行可能

### 推奨使用方法
- **UI更新**: インターフェース経由でキャストレス
- **ゲーム計算**: 才能値の計算などはインターフェース経由
- **状態チェック**: IsActive、GetCurrentHPなど頻繁なチェック

## トラブルシューティング

### よくある問題

#### 1. コンポーネントがnullptr
```cpp
// 解決方法: null チェック
auto* StatusComp = IIdleCharacterInterface::Execute_GetCharacterStatusComponent(Character);
if (StatusComp) {
    FCharacterTalent Talent = StatusComp->GetTalent();
}
```

#### 2. Blueprint関数が見つからない
- プロジェクトを再コンパイル
- Blueprintエディターで「Compile」ボタンクリック
- "Idle Character"カテゴリを確認

#### 3. 才能データが初期化されない
```cpp
// CharacterStatusComponentのコンストラクタで初期化済み
UCharacterStatusComponent::UCharacterStatusComponent() {
    Status = FCharacterStatus();
    ClubType = EClubType::Baseball;
    Talent = FCharacterTalent();  // デフォルト値で初期化
}
```

## 今後の拡張予定

### 予定機能
- スキル成長システム
- 装備品による能力値補正
- 部活動ボーナス適用

### 拡張方法
```cpp
// 例: スキル上昇
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
bool ImproveSkill(ESkillType SkillType, float Amount);
```

## 関連ドキュメント

- [PlayerController仕様書](./PlayerController.md)
- [CLAUDE.md](../CLAUDE.md) - プロジェクト全体仕様
- [CharacterTypes.h](../Source/UE_Idle/Types/CharacterTypes.h) - データ構造定義
- UCharacterStatusComponent仕様書（予定）
- UCharacterInventoryComponent仕様書（予定）