# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Unreal Engine idle game project (UE_IdleWSL) featuring a comprehensive DataTable-based item management system. The project implements a unified system for weapons, armor, and consumables with quality variants, equipment slots, and character inventories.

## Repository Structure

```
UE_IdleWSL/
├── Source/UE_Idle/                    # C++ source code
│   ├── Types/
│   │   ├── ItemDataTable.h            # Core item data structures and enums
│   │   └── CharacterTypes.h           # Character-related types
│   ├── Managers/
│   │   ├── ItemDataTableManager.h/.cpp # DataTable-based item management
│   │   └── ItemManager.h/.cpp         # Legacy JSON-based manager (deprecated)
│   └── Components/
│       ├── GlobalInventoryComponent.h/.cpp    # Shared storage system
│       └── CharacterInventoryComponent.h/.cpp # Individual character inventory
├── Content/Data/
│   └── ItemData.csv                   # DataTable source CSV
└── docs/
    └── アイテム仕様書.md               # Item system specification (Japanese)
```

## Core Architecture

### Item Management System

The project uses Unreal Engine's DataTable system for item management:

- **Primary Manager**: `UItemDataTableManager` (GameInstance Subsystem)
- **Data Source**: CSV file imported as DataTable
- **Row Structure**: `FItemDataRow` (inherits from `FTableRowBase`)
- **Item Identification**: RowName serves as ItemId (e.g., "short_sword", "iron_helmet")

### Inventory Architecture

**Two-tier inventory system:**

1. **GlobalInventoryComponent** - Shared storage across all characters
   - Attached to PlayerController
   - Handles global item storage and money
   - Event dispatchers: `OnStorageChanged`, `OnMoneyChanged`

2. **CharacterInventoryComponent** - Individual character inventory
   - Attached to character instances
   - Manages personal inventory and equipment
   - Event dispatcher: `OnInventoryChanged`

### Key Data Structures

```cpp
// Item instance for individual tracking
struct FItemInstance {
    FGuid InstanceId;                    // Unique identifier
    FString ItemId;                      // References DataTable RowName
    int32 CurrentDurability;             // Current durability
    TMap<FString, float> CustomData;     // Extensible data (enchantments, etc.)
};

// Inventory slot combining quantity and instances
struct FInventorySlot {
    FString ItemId;                      // Item type identifier
    int32 Quantity;                      // Total quantity
    TArray<FItemInstance> ItemInstances; // Individual item instances
};
```

## Item System Features

### Item Types (EItemTypeTable)
- `Weapon` - Equippable weapons
- `Armor` - Defensive equipment and accessories  
- `Consumable` - Potions, food, temporary effects
- `Material` - Crafting components
- `Quest` - Quest-specific items
- `Misc` - General items

### Equipment Slots (EEquipmentSlotTable)
- `Weapon`, `Shield` - Combat equipment
- `Head`, `Body`, `Legs`, `Hands`, `Feet` - Armor pieces
- `Accessory` - Rings, amulets (2 slots available)
- `None` - Non-equippable items

### Quality System (EItemQualityTable)
- `Poor` (0.7x modifier) - Substandard items
- `Common` (1.0x modifier) - Standard items
- `Good` (1.3x modifier) - Well-crafted items
- `Masterwork` (1.6x modifier) - Expert craftsmanship
- `Legendary` (2.0x modifier) - Exceptional items

Quality affects: AttackPower, Defense, MaxDurability, BaseValue

## Development Workflow

### Build Commands
```bash
# Build the project (replace with actual build command when project exists)
UnrealBuildTool UE_Idle Win64 Development

# Package the project
# Use Unreal Editor's packaging tools or automation commands
```

### CSV Data Management

**CSV Structure Requirements:**
- **RowName**: Must be unique ItemId (e.g., "short_sword")
- **Name**: Display name (supports Japanese text)
- **Headers**: Must match FItemDataRow properties exactly

**Import Settings for DataTable:**
- Ignore Extra Fields: ON
- Ignore Missing Fields: ON
- Preserve Existing Values: OFF
- Import Key Field: (leave blank)

**Sample CSV entry:**
```csv
RowName,Name,Description,ItemType,StackSize,Weight,BaseValue...
"short_sword","ショートソード","軽量で扱いやすい短剣","Weapon",1,1.2,150...
```

### Blueprint Integration

**Key Blueprint Functions:**

**Item Management:**
- `AddItemToStorage(ItemId, Quantity)` - Add to global storage
- `AddItem(ItemId, Quantity)` - Add to character inventory
- `TransferToCharacter(ItemId, Quantity)` - Storage → Character
- `TransferFromCharacter(ItemId, Quantity)` - Character → Storage

**UI Support:**
- `GetAllStorageSlots()` - Returns TArray<FInventorySlot> for UI display
- `GetAllInventorySlots()` - Character inventory for UI
- Event dispatchers for real-time UI updates

**Equipment System:**
- `EquipWeapon(ItemInstanceId)` - Equip weapon
- `EquipArmor(ItemInstanceId, Slot)` - Equip armor to specific slot
- `CanEquipItem(ItemId)` - Check equipment compatibility

## Design Philosophy

### Interface-Driven Architecture

**Core Principle**: Use interfaces to minimize dependencies between actors and components.

**Key Benefits**:
- **Decoupling**: Classes depend on abstractions, not concrete implementations
- **Testability**: Easy to mock interfaces for unit testing
- **Performance**: Avoid expensive Cast<> operations in Blueprint and C++
- **Maintainability**: Changes to implementation don't affect consumers

**Implementation Guidelines**:
```cpp
// ✅ Good: Interface-based communication
if (Actor->GetClass()->ImplementsInterface(UPlayerControllerInterface::StaticClass())) {
    IPlayerControllerInterface::Execute_AddItemToStorage(Actor, "item", 1);
}

// ❌ Avoid: Direct casting creates tight coupling
if (auto* CustomPC = Cast<AC_PlayerController>(Actor)) {
    CustomPC->AddItemToStorage("item", 1);
}
```

**Interface Strategy**:
- Define `UINTERFACE` for actor communication contracts
- Use `BlueprintImplementableEvent` for seamless Blueprint integration
- Implement `_Implementation` functions in C++ for performance
- Prefer composition over inheritance for complex behaviors

## Code Conventions

### Naming Patterns
- **Enums**: `ETypeNameTable` (e.g., `EItemTypeTable`)
- **Structs**: `FStructName` (e.g., `FItemDataRow`)
- **Classes**: `UClassName` (e.g., `UItemDataTableManager`)
- **Components**: `UComponentName` (e.g., `UGlobalInventoryComponent`)

### File Organization
- **Types**: Core data structures and enums
- **Managers**: Subsystem classes for game logic
- **Components**: Actor components for specific functionality

### Blueprint Exposure
- Use `UFUNCTION(BlueprintCallable)` for functions needed in Blueprints
- Use `UPROPERTY(BlueprintReadWrite)` for data access
- Category organization: "Basic", "Weapon", "Armor", "Consumable Effects"

## Important Migration Notes

**DataTable Migration Complete:**
- System migrated from JSON-based ItemManager to DataTable-based ItemDataTableManager
- CSV RowName serves as primary ItemId (removed redundant ItemId field)
- All item lookup functions use RowName directly for performance

**USTRUCT Limitations:**
- `UFUNCTION` macros cannot be used in `USTRUCT` definitions
- Use helper functions in manager classes instead
- Quality modifier functions implemented as static methods

## Common Issues & Solutions

**Compilation Errors:**
- Ensure include path: `"Subsystems/GameInstanceSubsystem.h"` (not `"Engine/GameInstanceSubsystem.h"`)
- Remove UFUNCTION from USTRUCT definitions
- Verify all enum values match CSV data exactly

**Blueprint Event Binding:**
- Use "Bind Event to OnStorageChanged" node (not direct event assignment)
- Ensure same component instance for binding and function calls
- Custom events must match delegate signature exactly

**Item Lookup Failures:**
- Verify CSV RowName matches ItemId parameters exactly
- Check DataTable is properly assigned to ItemDataTableManager
- Ensure case-sensitive matching (e.g., "short_sword" not "Short_Sword")

## Testing & Verification

**Essential Test Cases:**
- Item addition to both storage and character inventory
- Equipment slot compatibility and restrictions
- Quality modifier calculations
- Event dispatcher functionality for UI updates
- CSV import with Japanese text support

**Debugging Tips:**
- Add logging to ItemDataTableManager for item lookup failures
- Verify DataTable contains expected RowNames
- Check Blueprint component references are valid
- Test event binding with simple debug prints

## Dependencies

**Required Modules** (UE_Idle.Build.cs):
- Core, CoreUObject, Engine, InputCore (standard)
- Json, JsonUtilities (legacy, can be removed after full DataTable migration)

**Optional Modules for Future Features:**
- Slate, SlateCore (advanced UI)
- OnlineSubsystem (multiplayer features)

## Future Expansion

**Planned Features:**
- Crafting system using Material items
- Equipment enhancement via CustomData
- Advanced status effects beyond basic consumables
- Trading/marketplace functionality
- Equipment durability and repair mechanics

The system architecture supports these extensions through the CustomData system and flexible DataTable structure.