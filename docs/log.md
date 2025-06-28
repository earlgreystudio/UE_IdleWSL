● ウィジェットの使い方：

  UC_TeamList
  - 全チームを表示するメインコンテナ
  - ウィジェット名: TeamsWrapBox（チームカード用）、CreateTeamButton（チーム作成用）

  UC_TeamCard
  - 個々のチーム情報を表示
  - ウィジェット名: TeamNameText、CurrentTaskText、TeamStatusText、MemberCountText、Ch
  aracterCardsContainer、TeamTaskCardsContainer

  UC_TaskCard
  - 汎用タスク表示ウィジェット
  - ウィジェット名: TaskNameText、TaskStatusText、TaskDescriptionText、ProgressText、E
  stimatedTimeText、TaskProgressBar、StartButton、PauseButton、CancelButton

  命名規則：
  - コンテナ: [内容]Container または [内容]WrapBox
  - テキスト: [プロパティ]Text
  - ボタン: [アクション]Button
  - プログレスバー: [プロパティ]ProgressBar
