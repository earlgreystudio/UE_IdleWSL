LogTemp: Team 0 gathered 1 wood (target)
LogTemp: Warning: ProcessGatheringExecutionWithTarget: Calling ReduceSpecifiedTaskQuantity for wood x1
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Called for wood x1
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Found 2 total tasks
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Task 0 - Type: 2, ItemId: plains, GatheringType: 0, Completed: No
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Task 1 - Type: 5, ItemId: wood, GatheringType: 1, Completed: No
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Item wood - Task default_gathering_001 reduced from 2 to 1
LogTemp: Warning: UpdateTaskTargetQuantity: Task default_gathering_001 quantity changed from 2 to 1
LogTemp: ✅ Gathering initiated for wood
LogTemp: Warning: ProcessSpecificTask: Team 0 executing TaskType=採集
LogTemp: Warning: 🔍 TASK MATCHING: Team 0 at plains
LogTemp: Warning: 📋 Team has 1 tasks
LogTemp: Warning: 🎯 Priority 1: 採集
LogTemp: Warning: ✅ MATCHED: wood for 採集
LogTemp: Warning: TASK PROGRESS: default_gathering_001 (0/1)
LogTemp: 📋 Gathering Plan: Execute gathering for wood
LogTemp: 📋 Delegating gathering plan to TeamComponent: Execute gathering for wood at plains
LogTemp: 📋 TeamComponent: Executing plan for team 0 - Execute gathering for wood at plains
LogTemp: 🌾 TeamComponent: Delegating gathering of wood for team 0
LogTemp: Warning: 🌾 ExecuteGathering: Team 0 at location plains
LogTemp: Warning: 🌾 SetTeamTargetLocation: Team 0 target location set to plains
LogTemp: Warning: 🎯 GatheringComponent: ProcessTeamGatheringWithTarget Team 0 targeting wood
LogTemp: Warning: 🎯 GatheringComponent: Team 0 current state: 2
LogTemp: Warning: 🎯 GatheringComponent: Team 0 executing gathering for wood
LogTemp: Warning: 🌾 ProcessGatheringExecutionWithTarget: Team 0 targeting wood
LogTemp: Warning: 🌾 ProcessGatheringExecutionWithTarget: Team 0 has target location
LogTemp: Warning: DistributeItemToTeam: Attempting to distribute 2 wood to team 0
LogTemp: Warning: DistributeItemToTeam: Team 0 has 1 members
LogTemp: Warning: DistributeItemToTeam: Starting distribution of 2 items
LogTemp: Warning: DistributeItemToTeam: Member 0 (BP_IdleCharacter_C_0) - Capacity: 54.64, Current: 4.00
LogTemp: Warning: DistributeItemToTeam: Member 0 (BP_IdleCharacter_C_0) can add 2 items
LogTemp: Warning: DistributeItemToTeam: Added 2 items to member 0, remaining: 0
LogTemp: Warning: DistributeItemToTeam: Distribution complete, remaining: 0, success: true
LogTemp: Team 0 gathered 2 wood (target)
LogTemp: Warning: ProcessGatheringExecutionWithTarget: Calling ReduceSpecifiedTaskQuantity for wood x2
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Called for wood x2
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Found 2 total tasks
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Task 0 - Type: 2, ItemId: plains, GatheringType: 0, Completed: No
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Task 1 - Type: 5, ItemId: wood, GatheringType: 1, Completed: No
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Item wood - Task default_gathering_001 reduced from 1 to 0
LogTemp: Warning: UpdateTaskTargetQuantity: Task default_gathering_001 quantity changed from 1 to 0
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Task default_gathering_001 reached 0 quantity, completing and removing
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Stopping teams gathering wood before task removal
LogTemp: Warning: StopGatheringForItem: Called for item wood
LogTemp: Warning: StopGatheringForItem: Team 0 will be stopped (gathering wood at plains)
LogTemp: Warning: StopGathering: Called for team 0
LogTemp: Warning: StopGathering: Team 0 current state: 2 (Gathering)
LogTemp: Warning: StopGathering: Team 0 needs to return to base (current state: Gathering)
LogTemp: SetTeamActionState: Team 0 state changed from 作業中 to 移動中
LogTemp: UC_TeamList::OnTeamsUpdated
LogTemp: Warning: MovementComponent: Started movement for team 0 from plains (100.0m) to base (0.0m) - Distance: 100.0, Speed: 30.0, Time: 3.3s
LogTemp: Warning: StopGathering: Team 0 started returning to base from plains (distance: 100.0m)
LogTemp: Warning: StopGatheringForItem: Team 0 returning to base, keeping Gathering task until return completes
LogTemp: Warning: StopGatheringForItem: Stopped team 0 from gathering wood
LogTemp: Warning: StopGatheringForItem: Stopped 1 teams from gathering wood
LogTemp: Warning: CompleteTask: Task default_gathering_001 marked as completed
LogTemp: TaskManager Removed: 採集: 木材 x10 (ID: default_gathering_001, Priority: 4, Type: 採集)
LogTemp: Warning: === GlobalTaskCard::InitializeWithTask START ===
LogTemp: Warning: Task: 冒険: 平原 (Index: 0)
LogTemp: Warning: Calling UpdateDisplay...
LogTemp: Warning: === GlobalTaskCard::UpdateDisplay START ===
LogTemp: Warning: UpdateTaskTypeDisplay: TaskTypeText = OK
LogTemp: Warning: TaskTypeText set to: 冒険
LogTemp: Warning: === GetItemDisplayName START: ItemId = 'plains' ===
LogTemp: Error: GetItemDisplayName: ItemDataManager is NULL
LogTemp: Error: GetItemDisplayName: FALLBACK - returning ItemId 'plains'
LogTemp: Warning: === GlobalTaskCard::UpdateDisplay COMPLETED ===
LogTemp: Warning: === GlobalTaskCard::InitializeWithTask COMPLETED ===
LogTemp: UC_TaskList::CreateTaskCard - Created card for task debug_adventure_plains
LogTemp: UC_TaskList::RefreshTaskList - Created 0 task cards
LogTemp: Warning: === GlobalTaskCard::InitializeWithTask START ===
LogTemp: Warning: Task: 冒険: 平原 (Index: 0)
LogTemp: Warning: Calling UpdateDisplay...
LogTemp: Warning: === GlobalTaskCard::UpdateDisplay START ===
LogTemp: Warning: UpdateTaskTypeDisplay: TaskTypeText = OK
LogTemp: Warning: TaskTypeText set to: 冒険
LogTemp: Warning: === GetItemDisplayName START: ItemId = 'plains' ===
LogTemp: Error: GetItemDisplayName: ItemDataManager is NULL
LogTemp: Error: GetItemDisplayName: FALLBACK - returning ItemId 'plains'
LogTemp: Warning: === GlobalTaskCard::UpdateDisplay COMPLETED ===
LogTemp: Warning: === GlobalTaskCard::InitializeWithTask COMPLETED ===
LogTemp: UC_TaskList::CreateTaskCard - Created card for task debug_adventure_plains
LogTemp: UC_TaskList::RefreshTaskList - Created 0 task cards
LogTemp: RecalculatePriorities: Recalculated priorities for 1 tasks
LogTemp: Warning: ReduceSpecifiedTaskQuantity: Task default_gathering_001 successfully removed
LogTemp: SetTeamActionState: Team 0 state changed from 移動中 to 作業中
LogTemp: Warning: 🔍 TASK MATCHING: Team 0 at plains
LogTemp: Warning: 📋 Team has 1 tasks
LogTemp: Warning: 🎯 Priority 1: 採集
LogTemp: Warning: ❌ NO MATCH for 採集
LogTemp: Warning: ❌ NO MATCHES FOUND - Team 0 returning to base