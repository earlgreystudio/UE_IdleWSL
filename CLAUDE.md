# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Unreal Engine idle game project (UE_IdleWSL) targeting **iOS and Android mobile platforms**. The project features a comprehensive DataTable-based item management system designed for mobile gameplay. The project implements a unified system for weapons, armor, and consumables with quality variants, equipment slots, and character inventories.

## Repository Structure

```
UE_IdleWSL/
├── Source/UE_Idle/                    # C++ source code
│   ├── Types/                         # Data structures and enums
│   ├── Managers/                      # GameInstance Subsystems
│   ├── Components/                    # Actor components
│   ├── Actor/                         # Custom actors
│   └── UI/                           # UI widgets
├── Content/Data/                      # DataTable CSV files
└── docs/
    ├── specifications/                # Detailed component specs
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

### Task Management System
- **TaskManagerComponent**: Global task management and prioritization
- **TimeManagerComponent**: Timer-based task execution and time progression
- **TeamComponent**: Team formation and task assignment

### Item System Features
- **Item Types**: Weapon, Armor, Consumable, Material, Quest, Misc
- **Equipment Slots**: Weapon, Shield, Head/Body/Legs/Hands/Feet, Accessory (2 slots)
- **Quality System**: Poor (0.7x) → Common (1.0x) → Good (1.3x) → Masterwork (1.6x) → Legendary (2.0x)

## Development Workflow

### CSV Data Management
- **RowName**: Unique ItemId (e.g., "short_sword")
- **Headers**: Must match struct properties exactly
- **Import Settings**: Ignore Extra/Missing Fields ON, Preserve Values OFF

### Key Blueprint Functions
- **Item Management**: `AddItemToStorage`, `AddItem`, `TransferToCharacter`
- **Equipment**: `EquipWeapon`, `EquipArmor`, `CanEquipItem`
- **UI Updates**: Event dispatchers for real-time inventory changes

## Design Philosophy

### Turn-Based Recalculation Architecture
**CRITICAL DESIGN PRINCIPLE**: Every turn, TimeManagerComponent completely recalculates what each team should do based on current state only.

#### Team Task Priority System
- **Team Tasks Override Global Tasks**: Team task priorities take precedence over global task priorities
- **Keep Task Satisfaction**: Keep quantity tasks check total base inventory (base storage + all team inventories) for satisfaction
- **Three Gathering Types**: Unlimited (永続), Keep (維持), Specified (個数指定) with different completion behaviors

- **Complete Recalculation**: Each turn is independent - no dependency on past states or future plans
- **No State Memory**: Never assume "we're moving for 10 turns so skip thinking" - ALWAYS recalculate
- **Simple Logic**: Each turn asks only "What should this team do RIGHT NOW?"
- **Current Situation Only**: Decisions based solely on: current location, current tasks, current inventory
- **No Complex State Machines**: Avoid storing movement states, action sequences, or multi-turn plans

**Example Flow**:
1. Turn 1: "Team at base, task needs wood at plains" → Start movement to plains
2. Turn 2: "Team moving to plains" → Continue movement  
3. Turn 3: "Team at plains, task needs wood" → Execute gathering
4. Turn 4: "Team at plains, task complete" → Start return to base

Each turn decision is made independently without reference to previous turns.

### Interface-Driven Architecture
- Use `UINTERFACE` for actor communication contracts
- Avoid expensive Cast<> operations - use interface checks instead
- Implement `_Implementation` functions in C++ for performance
- Prefer composition over inheritance for complex behaviors

## Code Conventions
- **Enums**: `ETypeNameTable`, **Structs**: `FStructName`, **Classes**: `UClassName`
- **Blueprint Exposure**: Use `UFUNCTION(BlueprintCallable)` and `UPROPERTY(BlueprintReadWrite)`
- **Categories**: Organize by functionality ("Item", "Task", "Team", etc.)

## Important Notes

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