# 安全な修正リスト

## 適用可能な修正（コンパイルエラーなし）

### 1. CharacterTypes.h - FCharacterTalent構造体
```cpp
// 追加するフィールド
UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
float Wisdom;

UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes") 
float Charisma;

UPROPERTY(BlueprintReadWrite, Category = "Talent|Attributes")
float Luck;

// コンストラクタにも追加
Wisdom = 1.0f;
Charisma = 1.0f;
Luck = 1.0f;
```

### 2. LocationTypes.h - FLocationDataRow構造体
```cpp
// MovementCost, bIsWalkable, MovementDifficulty, DifficultyLevel追加
// Distance削除
```

### 3. GridCellActor.cpp - IsEmpty修正
```cpp
// 変更前: CurrentCellType.IsEmpty()
// 変更後: !CurrentCellType.IsValid()
```

## 避けるべき修正（後回し）

- AI関連フォルダ全体
- MapGeneratorComponent
- C_IdleCharacterのPawn変更
- FloatingPawnMovementComponent関連

## 段階的適用手順

1. **WindowsプロジェクトからAIフォルダを削除**
2. **上記の安全な修正のみ適用** 
3. **リビルド確認**
4. **動作確認後、段階的に機能追加**