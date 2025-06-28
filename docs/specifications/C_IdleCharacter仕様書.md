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
- **UCharacterStatusComponent** - ステータス・才能・部活動データ管理、派生ステータス（DerivedStats）事前計算
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

### 4. キャラクター種族管理

#### GetCharacterRace
```cpp
UFUNCTION(BlueprintCallable, Category = "Character")
FString GetCharacterRace() const;
```
**機能**: キャラクターの種族を取得  
**戻り値**: FString（種族名、CharacterPresets.csvのRowNameに対応）  
**実装**: CharacterRaceプロパティを返す

#### SetCharacterRace
```cpp
UFUNCTION(BlueprintCallable, Category = "Character")
void SetCharacterRace(const FString& NewRace);
```
**機能**: キャラクターの種族を設定  
**パラメータ**: NewRace - 新しい種族名（"human", "goblin", "wolf"等）  
**実装**: CharacterRaceプロパティを更新、爪牙システムで使用

## コンポーネント詳細

### UCharacterStatusComponent
**場所**: `Source/UE_Idle/Components/CharacterStatusComponent.h`  
**管理データ**:
- **FCharacterStatus Status** - HP、スタミナ、精神力などの変動ステータス
- **ESpecialtyType SpecialtyType** - 所属部活動
- **FCharacterTalent Talent** - 基本能力値（筋力、体力、知力、器用さ、敏捷、精神力）とスキル
- **FDerivedStats DerivedStats** - 事前計算済み派生ステータス（戦闘・作業能力）

**主要機能**:
```cpp
// ステータス関連
FCharacterStatus GetStatus() const;
void SetStatus(const FCharacterStatus& NewStatus);
float GetCurrentHealth() const;
float GetMaxHealth() const;

// 部活動関連
ESpecialtyType GetSpecialtyType() const;
void SetSpecialtyType(ESpecialtyType NewSpecialtyType);

// 才能関連
FCharacterTalent GetTalent() const;
void SetTalent(const FCharacterTalent& NewTalent);

// 派生ステータス関連（最適化済み）
FDerivedStats GetDerivedStats() const;
float GetConstructionPower() const;
float GetAttackSpeed() const;
float GetHitChance() const;
float GetDPS() const;
void RecalculateDerivedStats();
void OnEquipmentChanged();
```

**パフォーマンス最適化**:
- **事前計算**: 戦闘・作業関連の計算結果をDerivedStatsに保存
- **O(1)アクセス**: 計算済み値の直接取得でリアルタイム処理を高速化
- **再計算タイミング**: 装備変更・才能変更時のみ再計算実行

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
    
    // キャラクター基本データ初期化
    CharacterName = TEXT("Idle Character");
    CharacterRace = TEXT("human");  // デフォルトは人間
    bIsActive = true;
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

### FDerivedStats（派生ステータス）
```cpp
struct FDerivedStats {
    // 戦闘関連（事前計算済み）
    float AttackSpeed = 1.0f;
    float HitChance = 50.0f;
    float DodgeChance = 10.0f;
    float ParryChance = 5.0f;
    float CriticalChance = 5.0f;
    int32 BaseDamage = 1;
    int32 DefenseValue = 0;
    
    // 作業関連
    float ConstructionPower = 10.0f;
    float ProductionPower = 10.0f;
    float GatheringPower = 10.0f;
    float CookingPower = 10.0f;
    float CraftingPower = 10.0f;
    
    // 表示用総合値
    float DPS = 1.0f;
    float CombatPower = 10.0f;
    float WorkPower = 10.0f;
    float TotalDefensePower = 0.0f;
};
```

### ESpecialtyType（部活動種類）
- Baseball（野球部）
- Kendo（剣道部）
- Chemistry（化学部）
- など38種類の部活動

### キャラクター種族
- **CharacterRace**: FString型、CharacterPresets.csvのRowNameに対応
- **人間**: "human"（デフォルト、素手攻撃力2）
- **動物**: "goblin", "wolf", "bear"等（自然武器を持つ）
- **爪牙システム**: 種族により自然武器データが異なる

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
        StatusComp->SetTalent(CurrentTalent);  // 派生ステータス自動再計算
    }
}
```

### 派生ステータス利用の例
```cpp
void AGameManager::GetCombatStats(AActor* Character) {
    if (auto* StatusComp = IIdleCharacterInterface::Execute_GetCharacterStatusComponent(Character)) {
        // 事前計算済み値の高速取得
        float AttackSpeed = StatusComp->GetAttackSpeed();
        float HitChance = StatusComp->GetHitChance();
        float DPS = StatusComp->GetDPS();
        int32 BaseDamage = StatusComp->GetDerivedStats().BaseDamage;
        
        UE_LOG(LogTemp, Warning, TEXT("Combat Stats - DPS: %.1f, Hit: %.1f%%"), DPS, HitChance);
    }
}
```

### 種族・自然武器システムの例
```cpp
void AGameManager::SetupCharacterRace(AActor* Character, const FString& Race) {
    if (auto* IdleChar = Cast<AC_IdleCharacter>(Character)) {
        IdleChar->SetCharacterRace(Race);  // "goblin", "wolf"等
        
        // 派生ステータスが自動で種族の自然武器を反映
        if (auto* StatusComp = IdleChar->GetStatusComponent()) {
            StatusComp->RecalculateDerivedStats();  // 必要に応じて手動再計算
        }
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
- **戦闘計算**: 派生ステータス（DerivedStats）の事前計算済み値を使用
- **作業効率**: 建設・生産・採集能力は派生ステータスから取得
- **状態チェック**: IsActive、GetCurrentHPなど頻繁なチェック

### 派生ステータスシステムのメリット
- **高速アクセス**: O(1)で戦闘・作業関連の計算済み値を取得
- **メモリ効率**: 必要時のみ再計算、通常は保存済み値を使用
- **一貫性**: 装備・才能変更時に全関連ステータスが同期更新
- **UI対応**: DPS、戦闘力等の表示用数値も事前計算済み

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
    SpecialtyType = ESpecialtyType::Baseball;
    Talent = FCharacterTalent();    // デフォルト値で初期化
    DerivedStats = FDerivedStats(); // 派生ステータス初期化
}
```

#### 4. 派生ステータスが更新されない
```cpp
// 才能変更後は自動で派生ステータスが再計算される
StatusComp->SetTalent(NewTalent);  // RecalculateDerivedStats()が自動実行

// 手動で再計算が必要な場合
StatusComp->RecalculateDerivedStats();
StatusComp->OnEquipmentChanged();  // 装備変更時
```

## 今後の拡張予定

### 予定機能
- スキル成長システム
- 装備品による能力値補正
- 部活動ボーナス適用
- 装備システムの正式実装（現在は暫定的にインベントリから検索）

### 拡張方法
```cpp
// 例: スキル上昇（派生ステータス自動更新）
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Idle Character")
bool ImproveSkill(ESkillType SkillType, float Amount);

// 例: 装備システム正式対応
UFUNCTION(BlueprintCallable, Category = "Character")
bool EquipItem(const FString& ItemId, EEquipmentSlotTable Slot);
```

### 注意事項
- **派生ステータス更新**: 才能・装備変更時は必ずRecalculateDerivedStats()が自動実行される
- **種族システム**: CharacterPresets.csvに新種族追加時は自然武器データも同時に定義
- **装備システム**: 現在は暫定実装、正式装備システム実装時に大幅変更予定

## 関連ドキュメント

- [PlayerController仕様書](./PlayerController.md)
- [CLAUDE.md](../CLAUDE.md) - プロジェクト全体仕様
- [CharacterTypes.h](../Source/UE_Idle/Types/CharacterTypes.h) - データ構造定義
- UCharacterStatusComponent仕様書（予定）
- UCharacterInventoryComponent仕様書（予定）