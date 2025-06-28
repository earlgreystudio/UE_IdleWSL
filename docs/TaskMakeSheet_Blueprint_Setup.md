# TaskMakeSheet Blueprint Setup Guide (Global)

## 問題
グローバルTaskMakeSheetも動作していない：
```
LogTemp: Error: Cannot create TaskMakeSheetWidget - missing requirements
```

## 必要な手順

### 1. WBP_TaskMakeSheetを作成
1. Content Browserで右クリック
2. User Interface > Widget Blueprint を選択
3. 親クラスとして `UC_TaskMakeSheet` を選択（C++クラスが存在する場合）
4. 名前を `WBP_TaskMakeSheet` とする

### 2. BP_PlayerControllerの設定
1. BP_PlayerControllerを開く
2. Class Defaults > Details パネルで探す
3. `Task Make Sheet Widget Class` に `WBP_TaskMakeSheet` を設定

## エラーの原因
PlayerControllerの以下のコードでWidgetClassがnullのため作成に失敗:
```cpp
if (!TaskMakeSheetWidgetClass)
{
    UE_LOG(LogTemp, Error, TEXT("Cannot create TaskMakeSheetWidget - missing requirements"));
    return;
}
```

## チェックリスト
- [ ] WBP_TaskMakeSheetが作成されている
- [ ] BP_PlayerControllerが正しく設定されている
- [ ] TaskMakeSheetWidgetClassプロパティが設定されている
- [ ] 親クラスが正しく指定されている