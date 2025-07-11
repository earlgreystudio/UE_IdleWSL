# 戦闘システムシミュレーター

このディレクトリには、UE_Idleの戦闘システムをPythonで再実装したシミュレーションツールが含まれています。

現在採用されているシステムに対応したツールのみを保持しています。

## ファイル詳細

### combat_simulator_full_balance.py
- **目的**: 最終バランス版シミュレーション
- **特徴**: 実際のゲーム実装に最も近い計算式
- **内容**: 
  - 部活動ボーナス37.5%調整済み
  - DerivedStatsシステム対応
  - 武器・防具バリエーション対応
- **用途**: 現在のゲームバランス検証

### combat_claw_system_test.py
- **目的**: 爪牙システム（自然武器システム）の検証
- **特徴**: 人間と動物キャラクターの戦闘バランス
- **内容**:
  - 種族別自然武器データ
  - 人間の素手vs動物の爪・牙
  - 武器の価値確認
- **用途**: 種族間バランスの確認

### combat_step_by_step_verification.py
- **目的**: 戦闘計算の詳細検証
- **特徴**: ステップバイステップのログ出力
- **内容**:
  - 詳細な計算過程の表示
  - C++実装との比較用
  - デバッグ情報の詳細出力
- **用途**: 計算ミスやバグの特定

## 使用例

### 現在のゲームバランス検証
```bash
python combat_simulator_full_balance.py
```

### 爪牙システムの確認
```bash
python combat_claw_system_test.py
```

### 詳細デバッグ
```bash
python combat_step_by_step_verification.py
```

## 開発者向け情報

### 新しいシミュレーターの追加

1. 既存のシミュレーターをベースに新しいファイルを作成
2. 計算式の変更部分を実装
3. テストケースを追加
4. このREADMEに説明を追加

### 注意事項

- Pythonの浮動小数点計算とC++の計算に微小な誤差が生じる場合があります
- 乱数シードを固定してテストの再現性を確保してください
- 大幅な計算式変更時は複数のシミュレーターで相互確認してください

## 対応する実装

これらのシミュレーターは以下のC++クラスに対応しています：

- **CombatCalculator.cpp** - 戦闘計算の実装
- **CharacterStatusComponent.cpp** - DerivedStatsの計算
- **ActionSystemComponent.cpp** - 戦闘進行の制御

計算式を変更する際は、シミュレーターと実装の両方を更新してください。