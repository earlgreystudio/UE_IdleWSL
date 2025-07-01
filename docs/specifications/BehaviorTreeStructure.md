# Behavior Tree Structure Specification

## Overview
This document defines the main Behavior Tree structure for the autonomous character system, implementing the "毎ターン新しい判断" (every turn fresh decision) philosophy using Unreal Engine's Behavior Tree system.

## Main Behavior Tree: BT_IdleCharacter

### Root Structure
```
BT_IdleCharacter
├── Root
    └── Selector "Main Decision Tree"
        ├── Sequence "Combat Branch" [Decorator: Has Combat Task]
        │   ├── BTTask_MoveToGridCell (to combat location)
        │   └── BTTask_ExecuteCombat
        │
        ├── Sequence "Gathering Branch" [Decorator: Has Gathering Task] 
        │   ├── BTTask_FindGatheringGrid
        │   ├── BTTask_MoveToGridCell (to gathering location)
        │   └── BTTask_ExecuteGathering
        │
        ├── Sequence "Crafting Branch" [Decorator: Has Crafting Task]
        │   ├── BTTask_MoveToGridCell (to base/crafting location)
        │   └── BTTask_ExecuteCrafting
        │
        ├── Sequence "Return to Base" [Decorator: Inventory Full]
        │   ├── BTTask_MoveToGridCell (to base)
        │   └── BTTask_UnloadItems
        │
        └── BTTask_Wait "Default Idle"
```

## Blackboard Keys

### BB_IdleCharacter

| Key Name | Type | Description | Updated By |
|----------|------|-------------|------------|
| CurrentLocation | String | Current location name | BTService_UpdateCurrentTask |
| CurrentGridPosition | Vector | Current grid coordinates | BTService_UpdateCurrentTask |
| TeamIndex | Int | Character's team index | BTService_UpdateCurrentTask |
| InventoryFull | Bool | Is inventory near capacity | BTService_UpdateCurrentTask |
| TargetItem | String | Current gathering/crafting target | BTDecorator_HasGatheringTask |
| TargetEnemy | String | Current combat target | BTDecorator_HasCombatTask |
| TargetGridCell | Vector | Destination grid cell | BTTask_FindGatheringGrid |
| TaskResult | Bool | Last task execution result | All BT Tasks |

## Services Configuration

### BTService_UpdateCurrentTask
- **Interval**: 0.5 seconds
- **Scope**: Root node (runs continuously)
- **Purpose**: Keep blackboard updated with current situation

## Decorator Logic

### BTDecorator_HasGatheringTask
- **Query**: TaskManager->GetNextAvailableTask(TeamIndex)
- **Condition**: TaskType == ETaskType::Gathering && !bIsCompleted
- **Output**: Sets TargetItem in blackboard

### BTDecorator_HasCombatTask  
- **Query**: TaskManager->GetNextAvailableTask(TeamIndex)
- **Condition**: TaskType == ETaskType::Combat && !bIsCompleted
- **Output**: Sets TargetEnemy in blackboard

### BTDecorator_InventoryFull
- **Query**: Character->GetInventoryComponent()->GetCapacityUsageRatio()
- **Condition**: Usage >= 0.8f (80% capacity)
- **Output**: Boolean condition

## Task Execution Flow

### Turn-Based Restart
1. **TimeManager** calls Character->OnTurnTick()
2. **Character** calls AIController->RestartBehaviorTree()
3. **BT** evaluates all decorators fresh (毎ターン新しい判断)
4. **BT** selects appropriate branch based on current situation

### Priority Order
1. **Combat** (highest priority - safety first)
2. **Gathering** (main productive activity)  
3. **Crafting** (base activities)
4. **Return to Base** (inventory management)
5. **Wait** (fallback idle state)

## Grid Integration

### Movement Tasks
- All movement uses **BTTask_MoveToGridCell**
- Uses **GridMapComponent** for A* pathfinding
- Integrates with existing **APawn** movement system

### Location Finding
- **BTTask_FindGatheringGrid** finds optimal resource locations
- Uses **GameplayTags** for resource types (Resource.Wood, etc.)
- Considers proximity and resource availability

## Service Integration

### TaskManagerComponent
- Provides task data via **GetNextAvailableTask()**
- Updates task progress via **UpdateTaskProgress()**
- Manages global task priorities

### InventoryComponent
- Capacity checking for return-to-base logic
- Item management during gathering/crafting

### GridMapComponent
- Pathfinding for all movement
- Location type queries for positioning

## Implementation Notes

### Blueprint vs C++
- **BT Structure**: Created in Blueprint Editor (Content/AI/BT_IdleCharacter.uasset)
- **Tasks/Decorators/Services**: Implemented in C++ for performance
- **Blackboard**: Created in Blueprint Editor (Content/AI/BB_IdleCharacter.uasset)

### Performance Considerations
- BT restart every turn (1 second) is acceptable for idle game
- Service updates at 0.5s provide responsive blackboard data
- C++ task implementations ensure fast execution

### Integration with Existing Systems
- Maintains compatibility with existing UI systems
- Works with current TaskManager and TeamComponent
- Preserves character progression and inventory systems