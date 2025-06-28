# TeamTaskMakeSheet Blueprint Setup Guide

## 問題
TeamTaskMakeSheetウィジェットが作成されているが、必要なUI要素がバインドされていないため動作しない。

## エラーメッセージ
```
UC_TeamTaskMakeSheet::NativeConstruct - CreateTaskButton is NULL! Check Blueprint binding
UC_TeamTaskMakeSheet::NativeConstruct - CancelButton is NULL! Check Blueprint binding  
UC_TeamTaskMakeSheet::NativeConstruct - TaskTypeComboBox is NULL! Check Blueprint binding
```

## 必要な手順

### 1. WBP_TeamTaskMakeSheetを作成
1. Content Browserで右クリック
2. User Interface > Widget Blueprint を選択
3. 親クラスとして `UC_TeamTaskMakeSheet` を選択
4. 名前を `WBP_TeamTaskMakeSheet` とする

### 2. 必要なUI要素を配置
ウィジェットデザイナーで以下の要素を配置:

```
[Canvas Panel] (ルート)
└── [Border] (背景用)
    └── [Vertical Box]
        ├── [Text] "チームタスク作成"
        ├── [Horizontal Box]
        │   ├── [Text] "タスクタイプ:"
        │   └── [ComboBoxString] (変数名: TaskTypeComboBox)
        ├── [Horizontal Box] 
        │   ├── [Text] "場所:"
        │   └── [ComboBoxString] (変数名: LocationComboBox)
        └── [Horizontal Box]
            ├── [Button] (変数名: CreateTaskButton)
            │   └── [Text] "作成"
            └── [Button] (変数名: CancelButton)
                └── [Text] "キャンセル"
```

### 3. 変数バインディング設定
各UI要素を選択して、Details パネルで:
1. "Is Variable" にチェック
2. 変数名を正確に設定:
   - `TaskTypeComboBox`
   - `LocationComboBox`
   - `CreateTaskButton`
   - `CancelButton`

### 4. WBP_TeamCardの設定
1. WBP_TeamCardを開く
2. Details パネルで Team Card セクションを探す
3. `Team Task Make Sheet Class` に `WBP_TeamTaskMakeSheet` を設定

## 重要な注意事項
- 変数名は大文字小文字を含めて正確に一致する必要がある
- `meta=(BindWidget)` でマークされた変数は必須
- LocationComboBoxは初期状態では非表示（Collapsed）にする

## デバッグ方法
1. Widget Reflectorで実行時のウィジェット階層を確認
2. ログで各要素のNULLチェックを確認
3. Blueprint CompilerでBindWidget警告をチェック