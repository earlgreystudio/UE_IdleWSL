# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Unreal Engine idle game project (UE_IdleWSL) targeting **iOS and Android mobile platforms**. The project features a comprehensive DataTable-based item management system designed for mobile gameplay. The project implements a unified system for weapons, armor, and consumables with quality variants, equipment slots, and character inventories.

## Repository Structure

```
UE_IdleWSL/
├── Source/UE_Idle/                    # C++ source code
│   ├── Types/                         # Data structures and enums
│   │   ├── CharacterTypes.h           # FCharacterSituation, FCharacterAction, ECharacterPersonality
│   │   ├── TeamTypes.h                # FTeamStrategy, FTeamInfo, ETaskType
│   │   ├── CommonTypes.h              # ETeamActionState (shared enums)
│   │   └── ...
│   ├── Managers/                      # GameInstance Subsystems  
│   ├── Components/                    # Actor components
│   │   ├── CharacterBrain.h           # Individual character decision engine
│   │   ├── TimeManagerComponent.h     # Simplified turn notification system
│   │   ├── TeamComponent.h            # Team coordination (not command)
│   │   └── ...
│   ├── Services/                      # Autonomous system services
│   │   ├── MovementService.h          # Movement operations
│   │   ├── GatheringService.h         # Resource collection
│   │   ├── CombatService.h            # Combat coordination
│   │   └── TaskInformationService.h   # Task recommendations
│   ├── Actor/                         # Custom actors
│   │   └── C_IdleCharacter.h          # Autonomous character with brain
│   └── UI/                           # UI widgets
├── Content/Data/                      # DataTable CSV files
└── docs/
    ├── specifications/                # Detailed component specs
    ├── ImplementationPlan/            # Migration documentation
    └── RoleAllocation.md              # Quick reference
```

## Documentation

### RoleAllocation.md

**Purpose**: Comprehensive role summary document (`docs/RoleAllocation.md`) that provides concise, bulleted explanations of every component, actor, manager, and class in the `Source/UE_Idle/` directory.

**Structure**: Organized by category (Actor, Components, Managers, etc.) for quick reference and system understanding.

**Usage**: Reference this document when:
- Understanding the overall system architecture
- Determining which component handles specific functionality
- Planning new features and their integration points
- Onboarding new developers to the project

**IMPORTANT**: When adding, modifying, or removing any files in `Source/UE_Idle/`, you MUST update `docs/RoleAllocation.md` to reflect these changes. This ensures the documentation remains accurate and useful for system comprehension.

### Detailed Specifications

**Location**: `docs/specifications/` directory contains detailed specifications for major components and systems.

**Available Specifications**:
- Component specifications (TaskManagerComponent, TimeManagerComponent, etc.)
- Actor specifications (C_IdleCharacter, PlayerController, etc.)
- System specifications (アイテム仕様書, 拠点システム仕様書, etc.)

**Usage**: Reference these detailed specifications when implementing, debugging, or extending specific components. These documents provide comprehensive implementation details, API references, and usage examples.

## Core Architecture

### Item Management System
- **Primary Manager**: `UItemDataTableManager` (GameInstance Subsystem)
- **Data Source**: CSV files imported as DataTables
- **Item Identification**: RowName serves as ItemId

### Inventory System
- **Unified Component**: `InventoryComponent` (configurable for character/storage use)
- **Two-tier Storage**: Global storage (PlayerController) + Character inventories
- **Event-driven UI**: Real-time updates via event dispatchers

### Autonomous Character System (Post-Migration Architecture)
- **CharacterBrain**: Individual character decision-making and autonomous behavior
- **TimeManagerComponent**: Simplified turn notification system (1 second = 1 turn)
- **TeamComponent**: Team coordination and strategy suggestions (not command)
- **Service Architecture**: MovementService, GatheringService, CombatService, TaskInformationService
- **TaskManagerComponent**: Task information provider (reduced responsibility)

### Item System Features
- **Item Types**: Weapon, Armor, Consumable, Material, Quest, Misc
- **Equipment Slots**: Weapon, Shield, Head/Body/Legs/Hands/Feet, Accessory (2 slots)
- **Quality System**: Poor (0.7x) → Common (1.0x) → Good (1.3x) → Masterwork (1.6x) → Legendary (2.0x)

## Development Workflow

### CSV Data Management
- **RowName**: Unique ItemId (e.g., "short_sword")
- **Headers**: Must match struct properties exactly
- **Import Settings**: Ignore Extra/Missing Fields ON, Preserve Values OFF

### Key Systems Integration
- **Item Management**: `AddItemToStorage`, `AddItem`, `TransferToCharacter`
- **Equipment**: `EquipWeapon`, `EquipArmor`, `CanEquipItem`
- **Autonomous Characters**: `OnTurnTick`, `AnalyzeMySituation`, `DecideMyAction`
- **Team Coordination**: `GetTeamStrategy`, `CoordinateWithTeammates`
- **Service Integration**: MovementService, GatheringService, CombatService
- **UI Updates**: Event dispatchers for real-time inventory and team changes

## Design Philosophy

### Autonomous Character Design (Bottom-Up Architecture)
**CRITICAL DESIGN PRINCIPLE**: Each character makes independent decisions based on their situation analysis and team coordination.

#### Character-Driven Decision Making
- **Individual Autonomy**: Each character analyzes their situation and decides actions autonomously
- **Team Coordination**: Teams provide strategy suggestions, not commands
- **Situation-Based Logic**: Characters use FCharacterSituation to understand their context
- **Personality System**: ECharacterPersonality affects decision preferences and behavior

#### Service-Oriented Architecture
- **MovementService**: Handles all movement-related operations
- **GatheringService**: Manages resource collection activities  
- **CombatService**: Coordinates combat encounters
- **TaskInformationService**: Provides task-related information and recommendations

#### Turn-Based Autonomous Processing
- **Simple TimeManager**: Only sends turn notifications (OnTurnTick) to all characters
- **Character Brain**: Each character's UCharacterBrain processes the turn independently
- **No Central Control**: No single component controls all character actions
- **Event-Driven Coordination**: Characters coordinate through team strategies and events

#### CRITICAL DESIGN PRINCIPLE: "Every Turn Fresh Decision"
- **No State Carryover**: Each turn is completely independent from previous turns
- **Fresh Analysis Every Turn**: Characters analyze their current situation from scratch
- **No Memory of Previous Actions**: Previous turn's decisions don't influence current turn
- **Stateless Decision Making**: All decisions based solely on current game state
- **No Action Continuation**: Every action is decided and executed within a single turn

**Example Autonomous Flow**:
1. Turn 1: Character analyzes situation → Decides "need wood" → Decides "move to plains" → Executes movement
2. Turn 2: Character analyzes situation → Decides "still need wood" → Decides "move to plains" → Continues movement
3. Turn 3: Character analyzes situation → Decides "arrived at plains" → Decides "gather wood" → Executes gathering
4. Turn 4: Character analyzes situation → Decides "inventory full" → Decides "return to base" → Executes return

**KEY PRINCIPLE**: Each character makes completely independent decisions every turn based ONLY on current situation. No state is preserved between turns.

### Interface-Driven Architecture
- Use `UINTERFACE` for actor communication contracts
- Avoid expensive Cast<> operations - use interface checks instead
- Implement `_Implementation` functions in C++ for performance
- Prefer composition over inheritance for complex behaviors

### Backward Compatibility Strategy
- **UI Systems**: All existing UIs continue to work unchanged
- **Event Dispatchers**: Team and character events maintain existing signatures
- **Progressive Migration**: Old systems coexist with new autonomous systems
- **Service Integration**: New services provide functionality for both old and new systems

## Code Conventions
- **Enums**: `ETypeNameTable`, **Structs**: `FStructName`, **Classes**: `UClassName`
- **Blueprint Exposure**: Use `UFUNCTION(BlueprintCallable)` and `UPROPERTY(BlueprintReadWrite)`
- **Categories**: Organize by functionality ("Item", "Task", "Team", etc.)

## Important Notes

### Development Environment Limitations
- **WSL Environment**: Claude Code runs in WSL (Windows Subsystem for Linux) because Claude Code cannot be used directly on Windows
- **No Unreal Engine in WSL**: WSL does not have Unreal Engine installed
- **Manual File Transfer Required**: Code modifications must be manually copied from WSL to Windows for compilation and testing
- **No Direct Compilation**: Cannot compile or run Unreal Engine projects directly from WSL environment
- **Testing Workflow**: 
  1. Modify code in WSL using Claude Code
  2. Copy modified files to Windows Unreal project
  3. Compile and test in Windows Unreal Editor
  4. Report results back to Claude Code session

### Key Limitations
- No `UFUNCTION` in `USTRUCT` definitions - use helper functions in manager classes
- Use correct include path: `"Subsystems/GameInstanceSubsystem.h"`
- Ensure CSV RowName matches ItemId parameters exactly (case-sensitive)

### Mobile Platform Target
- **Platforms**: iOS and Android
- **Design**: Touch-first UI, performance optimized for mobile hardware

## Code Modification Guidelines

### Live Coding vs Full Rebuild Requirements

**Live Coding Sufficient** (Hot reload while editor is running):
- Implementation changes in `.cpp` files only
- Changing function bodies, logic, calculations
- Adding/removing code within existing functions
- Modifying variable values and constants
- Blueprint graph changes

**Full Rebuild Required** (Close editor, compile, reopen):
- Any changes to `.h` header files
- Adding/removing class members (variables, functions)
- Changing function signatures (parameters, return types)
- Adding/removing `UPROPERTY()` or `UFUNCTION()` macros
- Modifying class inheritance
- Adding/removing #include statements
- Changing enum definitions or struct layouts

**Example Scenarios**:
- Adding `UPROPERTY(BlueprintReadWrite) UImage* FacilityIconImage;` → **Full Rebuild Required**
- Changing `GetFacilityCount() const` → **Full Rebuild Required** 
- Modifying logic inside `UpdateDisplay()` function → **Live Coding Sufficient**
- Adding new Blueprint nodes or connections → **Live Coding Sufficient**

**Important**: After header changes, always close Unreal Editor completely, run full compile, then reopen editor. Live coding will not work properly with header modifications.

**When providing code modifications, always end with:**
- **「ライブコーディング可」** - if only .cpp function body changes
- **「リビルドが必要」** - if header changes, new functions, or UFUNCTION additions