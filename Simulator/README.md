# UE_Idle Simulator

このディレクトリには、UE_Idleゲームのゲームバランス調整やシステム検証のためのシミュレーションツールが含まれています。

## ディレクトリ構成

### combat/
戦闘システムのシミュレーションとバランス調整ツール群

#### ファイル一覧

現在採用されているシステムに対応したツールのみを保持：

1. **combat_simulator_full_balance.py** - 最終バランス版
   - 実際のゲーム実装に最も近い計算式
   - 部活動ボーナス37.5%調整済み
   - DerivedStatsシステム対応

2. **combat_claw_system_test.py** - 爪牙システム検証
   - 自然武器システムのテスト
   - 人間vs動物の戦闘バランス検証
   - 種族別自然武器データ対応

3. **combat_step_by_step_verification.py** - デバッグツール
   - 戦闘計算の詳細ステップを確認
   - C++実装との比較用
   - デバッグ用の詳細ログ出力

## 使用方法

### 現在のゲームバランス検証
```bash
cd /home/yo314/UE_IdleWSL/Simulator/combat
python combat_simulator_full_balance.py
```

### 爪牙システムのテスト
```bash
python combat_claw_system_test.py
```

### 詳細デバッグ
```bash
python combat_step_by_step_verification.py
```

## 開発履歴

これらのシミュレーターは、UE_Idleの戦闘システム開発過程で作成されました。
現在は最終採用版のみを保持し、開発過程のファイルは整理済みです。

- **爪牙システム対応** (combat_claw_system_test.py) - 自然武器システム追加
- **最終版** (combat_simulator_full_balance.py) - 完成版の実装
- **デバッグツール** (combat_step_by_step_verification.py) - 詳細検証用

## 関連ドキュメント

- [戦闘システム仕様書](../docs/GameDesign/CombatCalculation.md)
- [キャラクター仕様書](../docs/C_IdleCharacter.md) 
- [プロジェクト全体仕様](../CLAUDE.md)

## 注意事項

- これらのシミュレーターはPython製のため、実際のC++実装と完全に同一ではありません
- バランス調整の参考値として使用してください
- 新しい計算式を試す際は、対応するシミュレーターも更新してください