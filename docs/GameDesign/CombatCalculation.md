# 戦闘システム計算式

## 概要

キャラクターの戦闘における命中・回避・ダメージ計算の仕様書。
**爪牙システム**により人間の素手戦闘と動物の自然武器を区別し、バランスの取れた戦闘を実現する。

## 爪牙システム（Natural Weapon System）

### 基本コンセプト
- **人間キャラクター**: 素手は弱く、武器を使うことで強くなる
- **動物キャラクター**: 生来の爪・牙・毒牙等で適切な戦闘力を持つ
- **武器価値の確立**: 素手と武器の明確な性能差を維持

### 自然武器データ管理
自然武器の性能は**CharacterPresets.csv**で種族ごとに定義：

```csv
RowName,Name,NaturalWeaponId,NaturalWeaponPower,NaturalWeaponName
"rat","ネズミ","rat_bite",4,"ネズミの歯"
"goblin","ゴブリン","goblin_claw",6,"ゴブリンの爪"
"wolf","狼","wolf_fang",10,"狼の牙"
"bear","クマ","bear_claw",15,"クマの爪"
"","人間","",2,"素手"
```

### 武器選択優先度
1. **装備武器がある場合**: 装備武器を使用
2. **装備武器がない場合**: 種族の自然武器を使用
3. **人間で武器なし**: 弱い素手戦闘

## キャラクター生成システム（バランス調整済み）

### プレイヤーキャラクター生成
```
基本能力値 = ランダム値(1-12)  ※調整済み
部活ボーナス = ランダム値(ボーナス値 × 0.5 ～ ボーナス値 × 0.375)  ※37.5%に縮小
最終能力値 = 基本能力値 + 部活ボーナス (上限100)
スキル値 = ランダム値(1-20) + 部活ボーナススキル
```

### 主要部活ボーナス例（調整済み）
**剣道部:**
- 力+3.75、頑丈+3.75、器用+3.75、敏捷+5.625、意志+7.5
- 片手武器+3.75、両手武器+11.25、受け流し+9.375

**野球部:**
- 力+5.625、頑丈+5.625、知能+1.875、器用+5.625、敏捷+5.625
- 格闘+1.875

**敵キャラクター:**
- CharacterPresets.csvの固定値使用

## ステータス計算

### HP計算
```
最大HP = (頑丈 × 3) + (力 × 1) + 50
```

### スタミナ計算
```
最大スタミナ = (頑丈 × 2) + (敏捷 × 2) + 30
```

### メンタル計算
```
最大メンタル = (意志 × 4) + (知能 × 1) + 20
```

### 積載量計算
```
積載量 = (力 × 1.5) + (頑丈 × 1.0) + (敏捷 × 0.5) + 20
```

## 装備重量ペナルティ

### 装備重量率の計算
```
装備重量率 = 装備総重量 ÷ 積載量
```

### ペナルティ段階

| 装備重量率 | 状況 | ペナルティ |
|-----------|------|-----------|
| 70%以上 | ほぼ動けない | 90% |
| 50-70% | 重装備 | 60-90% |
| 20-50% | 中装備 | 20-60% |
| 10-20% | 軽装備 | 5-20% |
| 10%未満 | ほぼ影響なし | 0-5% |

### 詳細計算式
```cpp
if (装備重量率 >= 0.7f)
    ペナルティ = 90%
else if (装備重量率 >= 0.5f)
    ペナルティ = 60 + (装備重量率 - 0.5) × 150
else if (装備重量率 >= 0.2f)
    ペナルティ = 20 + (装備重量率 - 0.2) × 133
else if (装備重量率 >= 0.1f)
    ペナルティ = 5 + (装備重量率 - 0.1) × 150
else
    ペナルティ = 装備重量率 × 50
```

## 命中・回避判定

### 回避率
```
基本回避率 = 10 + (敏捷 × 2) + (回避スキル × 2)
最終回避率 = 基本回避率 - 装備ペナルティ
盾装備時ペナルティ = 最終回避率 × 0.5 (半減)
```

### 受け流し率
```
基本受け流し率 = 5 + (器用 × 1.5) + (受け流しスキル × 3)
最終受け流し率 = 基本受け流し率 - 装備ペナルティ
```

### 盾防御率 (新システム)
```
基本発動率 = スキル5未満: スキル値 × 6
           スキル5以上: 30 + (スキル値 - 5) × (60/95)
器用さボーナス = 器用 × 0.3
盾防御率 = 基本発動率 + 器用さボーナス (最大95%)

盾ダメージカット率計算:
防御値 = 盾防御力 + 盾スキル値
ダメージ倍率 = max(0.02, 1 / (1 + 防御値 × 0.5))
カット率 = (1 - ダメージ倍率) × 100%

例: 鉄盾(防御5) + スキル10 = 防御値15 → 89%カット
```

### 命中率
```
基本命中率 = 50 + (スキルレベル × 2) + (器用 × 1.5) - (武器重量 ÷ (力 × 0.5))
```

### 判定順序
1. **回避判定** → 成功ならダメージ0で終了
2. **命中判定** → 失敗ならダメージ0で終了
3. **受け流し判定** → 成功ならダメージ80%カット
4. **盾防御判定** → 成功なら盾カット率適用
5. **クリティカル判定** → 成功ならダメージ2倍

## ダメージ計算（爪牙システム対応）

### 武器判定フロー
1. **装備武器の確認**: キャラクターの装備武器をチェック
2. **自然武器の判定**: 装備なしの場合、CharacterPresetsから自然武器データを取得
3. **攻撃種別の決定**: 素手/自然武器 または 人工武器

### 素手・自然武器戦闘ダメージ
```
自然武器ダメージ = 自然武器攻撃力 + (格闘スキル × 0.8) + (力 × 0.7)
```

**適用条件:**
- 装備武器なし かつ CharacterPresetsに自然武器データあり
- または 装備武器なし かつ 人間（基本攻撃力2）

**例:**
- **ゴブリン（爪）**: 6 + (格闘スキル8 × 0.8) + (力15 × 0.7) = 6 + 6.4 + 10.5 = 22.9 → 23
- **人間（素手）**: 2 + (格闘スキル10 × 0.8) + (力12 × 0.7) = 2 + 8 + 8.4 = 18.4 → 18

### 人工武器戦闘ダメージ（スキル効率システム）
```
スキル効率 = スキル未満: 0.2 + (実際スキル ÷ 必要スキル) × 0.8
          スキル以上: 1.0 + (超過スキル × 0.05) （上限なし）

武器命中補正 = 武器攻撃力 × 0.8 × スキル効率
武器ダメージ補正 = 武器攻撃力 × 1.5 × スキル効率
能力補正 = 力 × 0.5 (近接武器) または 器用 × 0.5 (遠距離武器)

最終命中力 = 基本命中力 + 武器命中補正
基本ダメージ = タレントレベル × 2 + 武器ダメージ補正 + 能力補正
```

**新システムの特徴:**
- **武器攻撃力 = 必要スキルレベル**: 棍棒5, ショートソード10, ロングソード15
- **スキル効率の重要性**: 適正スキル未満では大幅に弱体化（20%～100%）
- **超過スキルの価値**: 適正以上で5%ずつ性能向上
- **スキルなし武器の弱さ**: 効率20%で素手以下の性能

### 防御計算
```
防御値 = 防具防御力 + (頑丈 × 0.3)
最終ダメージ = 基本ダメージ × (100 ÷ (100 + 防御値))
```

### クリティカルヒット
```
クリティカル率 = 5 + (器用 × 0.5) + (スキルレベル × 0.3)
クリティカルダメージ = 基本ダメージ × 2
```

### 受け流し効果
```
受け流し時ダメージ = 基本ダメージ × 0.2 (80%カット)
```

### 盾防御効果 (新システム)
```
盾防御時ダメージ = 基本ダメージ × 盾ダメージ倍率
盾ダメージ倍率 = max(0.02, 1 / (1 + (盾防御力 + 盾スキル) × 0.5))

実例:
- 木盾(防御2) + スキル10 = 防御値12 → 86%カット (ダメージ倍率0.14)
- 鉄盾(防御5) + スキル10 = 防御値15 → 89%カット (ダメージ倍率0.11)
- 鋼盾(防御8) + スキル10 = 防御値18 → 92%カット (ダメージ倍率0.08)
```

## 攻撃速度計算

### 近接武器の攻撃速度
```
基本攻撃速度 = 2.0 + (敏捷 × 0.02) + (スキルレベル × 0.01)
武器重量ペナルティ = 武器重量 ÷ (力 × 0.3)
最終攻撃速度 = Max(0.1, 基本攻撃速度 - 武器重量ペナルティ)
```

### 遠距離武器の攻撃速度
```
基本攻撃速度 = 1.0 + (器用 × 0.01) + (スキルレベル × 0.03)
最終攻撃速度 = 基本攻撃速度
```

## 実戦例（爪牙システム・バランス調整済み）

### プレイヤー（素手）vs ゴブリン（爪）

**プレイヤー（野球部出身、人間の素手）:**
- 基本能力値: 力8.5、頑丈9.2、器用10.1、敏捷9.8（1-12 + 部活ボーナス37.5%）
- スキル: 格闘10.9（1-20 + ボーナス1.875）
- HP: (9.2×3) + (8.5×1) + 50 = 86.1
- 素手ダメージ: 2 + (10.9×0.8) + (8.5×0.7) = 2 + 8.7 + 6.0 = 16.7 → 17

**ゴブリン（爪）:**
- 能力値: 力15、頑丈16、器用16（CharacterPresets固定）
- スキル: 格闘8（固定）
- 自然武器: goblin_claw（攻撃力6）
- HP: (16×3) + (15×1) + 50 = 113
- 爪ダメージ: 6 + (8×0.8) + (15×0.7) = 6 + 6.4 + 10.5 = 22.9 → 23

**戦闘結果:**
- **バランス良好**: ゴブリンがやや有利（52%勝率想定）
- ダメージ差: 17 vs 23（ゴブリン有利）
- HP差: 86 vs 113（ゴブリン有利）

### プレイヤー（鋼の剣）vs ゴブリン（爪）

**プレイヤー（剣道部出身、鋼の剣装備）:**
- 基本能力値: 力8.9、頑丈8.7（1-12 + 剣道部ボーナス）
- スキル: 片手武器10.4（1-20 + ボーナス3.75）
- 武器: steel_sword（攻撃力15）
- 武器ダメージ: 15×(1+10.4/20) + (8.9×0.5) = 15×1.52 + 4.45 = 27.25 → 27

**戦闘結果:**
- **理想的バランス**: プレイヤー勝率50%想定
- ダメージ差: 27 vs 23（プレイヤー有利）
- **武器価値証明**: 素手17 → 武器27（+10の向上）

### バランス調整効果
爪牙システムにより以下を実現：
1. **武器価値の確立**: 素手より武器が明確に強い
2. **動物の適正戦闘力**: 爪・牙により自然な強さ
3. **理想的勝率**: 45-55%の均衡した戦闘

## 実装概要（DerivedStats最適化システム）

### パフォーマンス最適化
**従来の問題**: 戦闘計算を毎回リアルタイムで実行していたため、非効率的
**解決策**: **DerivedStats（派生ステータス）**システムによる事前計算

### DerivedStatsシステム
CharacterStatusComponentで戦闘に関連する全ての値を事前計算し、CombatCalculatorは計算済みの値を単純に取得するだけ。

**事前計算される値:**
- AttackSpeed（攻撃速度）
- HitChance（命中率）
- DodgeChance（回避率）
- ParryChance（受け流し率）
- CriticalChance（クリティカル率）
- BaseDamage（基本ダメージ）
- DefenseValue（防御値）

### 再計算タイミング
以下の場合にのみDerivedStatsを再計算：
- 装備変更時（OnEquipmentChanged）
- 才能・ステータス変更時（SetTalent）
- ステータス効果変更時（OnStatusEffectChanged）

### 現在の実装関数

#### 主要計算関数（最適化済み）
```cpp
// 全て事前計算済み値を直接返す（O(1)アクセス）
float UCombatCalculator::CalculateAttackSpeed(Character, WeaponItemId)
    → return StatusComp->GetAttackSpeed();

float UCombatCalculator::CalculateHitChance(Attacker, WeaponItemId)
    → return StatusComp->GetHitChance();

float UCombatCalculator::CalculateDodgeChance(Defender)
    → return StatusComp->GetDodgeChance();

int32 UCombatCalculator::CalculateBaseDamage(Attacker, WeaponItemId)
    → return StatusComp->GetDerivedStats().BaseDamage;

int32 UCombatCalculator::CalculateDefenseValue(Defender)
    → return StatusComp->GetDerivedStats().DefenseValue;
```

#### ヘルパー関数（爪牙システム対応）
```cpp
// CharacterPresetsから自然武器データ取得
FString UCombatCalculator::GetCharacterRace(AC_IdleCharacter* Character)
FString UCombatCalculator::GetEffectiveWeaponId(AC_IdleCharacter* Character)
int32 UCombatCalculator::GetNaturalWeaponPower(const FString& CharacterRace)
bool UCombatCalculator::IsNaturalWeapon(const FString& WeaponId)

// 武器・防具情報取得
float UCombatCalculator::GetWeaponWeight(const FString& WeaponItemId)
int32 UCombatCalculator::GetWeaponAttackPower(const FString& WeaponItemId)
bool UCombatCalculator::IsRangedWeapon(const FString& WeaponItemId)
ESkillType UCombatCalculator::GetWeaponSkillType(const FString& WeaponItemId)
```

#### 計算実装場所の移動
**従来**: CombatCalculatorで毎回計算
**現在**: CharacterStatusComponentで事前計算
- `CalculateCombatStats()` - 戦闘関連の全派生ステータスを計算
- `CalculateDisplayStats()` - UI表示用の総合値（DPS、CombatPower等）を計算

### CharacterPresets.csv 構造拡張
```csv
# 追加カラム
NaturalWeaponId,NaturalWeaponPower,NaturalWeaponName

# 例
"goblin","ゴブリン",...,"goblin_claw",6,"ゴブリンの爪"
```

## C++ 実装ガイド

### 1. CharacterPresets構造体拡張
```cpp
USTRUCT(BlueprintType)
struct FCharacterPresetData : public FTableRowBase
{
    // 既存フィールド...
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString NaturalWeaponId;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NaturalWeaponPower;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString NaturalWeaponName;
};
```

### 2. CombatCalculator修正方針
```cpp
int32 UCombatCalculator::CalculateBaseDamage(AC_IdleCharacter* Attacker, const FString& WeaponItemId)
{
    // 1. 効果的な武器IDを決定
    FString EffectiveWeaponId = GetEffectiveWeaponId(Attacker);
    
    // 2. 自然武器か人工武器かを判定
    bool bIsNaturalWeapon = IsNaturalWeapon(EffectiveWeaponId);
    
    // 3. 適切な計算式を適用
    if (bIsNaturalWeapon)
    {
        return CalculateNaturalWeaponDamage(Attacker, EffectiveWeaponId);
    }
    else
    {
        return CalculateArtificialWeaponDamage(Attacker, EffectiveWeaponId);
    }
}
```

## 更新履歴

- **2024年6月25日（初版）**: 実装に基づく完全書き直し
- **2024年6月25日（爪牙システム対応）**: 爪牙システム仕様追加
  - 人間の素手と動物の自然武器を分離
  - CharacterPresetsでの自然武器データ管理
  - バランス調整済み数値（1-12基本能力値、37.5%部活ボーナス）
  - 理想的勝率45-55%を達成する設計
  - 実装ガイドライン追加
- **2024年6月26日（DerivedStats最適化）**: パフォーマンス最適化実装
  - 事前計算システム（DerivedStats）の導入
  - CombatCalculatorの最適化（毎回計算 → 事前計算済み値の取得）
  - O(n)からO(1)へのアクセス時間改善
  - CharacterStatusComponentでの戦闘ステータス一元管理