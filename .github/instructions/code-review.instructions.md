# Code Review Instructions for Copilot

## Project Context

This is a 2D Metroidvania game engine called "Echoes of Slumber" built with:
- **Language**: C++17 (MSVC/Visual Studio 2022)
- **Build System**: Premake5 generating VS2022 solutions
- **Graphics**: SDL3 3.2.22+
- **Physics**: Box2D 3.1.1+ (C API)
- **XML Parsing**: pugixml 1.14+
- **Profiling**: Tracy 0.11.1+

## Architecture

The engine follows a Module-based architecture with lifecycle methods:
```
Awake → Start → (PreUpdate → Update → PostUpdate) → CleanUp
```

Modules are updated in order: Window → Input → Textures → Audio → Physics → Map → Scene → EntityManager → UIManager → Cinematics → Render

## Coding Standards

### Naming Conventions
| Element | Convention | Example |
|---------|------------|---------|
| Classes & Structs | PascalCase | `PlayerController`, `AnimationSet` |
| Public Methods | PascalCase | `GetPosition()`, `CreateEntity()` |
| Local Variables | camelCase | `playerSpeed`, `isJumping` |
| Private Members | camelCase + trailing underscore | `cameraTargetX_`, `deadZoneWidth_` |
| Constants & Macros | UPPER_SNAKE_CASE | `MAX_ENTITIES`, `PIXELS_PER_METER` |
| Enums | PascalCase type, UPPER_SNAKE_CASE values | `EntityType::PLAYER` |

### Memory Management
- Use `std::shared_ptr` for module ownership
- Entity creation through `EntityManager` factory
- **Deferred deletion pattern**: Mark `pendingToDelete = true`, clean up after iteration
- Physics bodies also use deferred deletion via `Physics::DeletePhysBody()`

### Performance Requirements
- Target: 60 FPS (16.6ms frame budget)
- Max RAM: 1.5 GB
- Max VRAM: 500 MB
- Max entities per room: 50

## Review Guidelines

When reviewing code, check for:

1. **Naming conventions** - Ensure private members have trailing underscore (`member_`)
2. **Input validation** - Clamp values to sensible ranges (see `SetDeadZone`, `SetCameraSmoothSpeed`)
3. **Scale consistency** - Camera/rendering code must account for window scale
4. **Module lifecycle** - Don't modify collections during iteration; use deferred deletion
5. **Memory leaks** - Verify cleanup in `CleanUp()` methods
6. **Box2D thread safety** - Never delete bodies during physics step
7. **Frame-order dependencies** - Be aware that modules update in a fixed order

## Commit Message Format

Use Conventional Commits:
```
feat: Add double jump mechanic
fix: Resolve crash on map load
docs: Update TDD with physics section
art: Add player spritesheet idle animation
chore: Configure CI pipeline
refactor: Extract pathfinding into standalone class
```

## Branch Naming

All branches must follow these patterns:
- `feat/*` - New features
- `fix/*` - Bug fixes
- `art/*` - Art assets
- `docs/*` - Documentation
- `level/*` - Level design
- `audio/*` - Audio assets
- `refactor/*` - Code restructuring
- `chore/*` - Build system, CI, maintenance

## Known Architectural Limitations

1. **Module Update Order**: Map renders tiles in `Update()` before EntityManager updates Player. Camera updates in Player cause 1-frame jitter. Future fix: move all world rendering to `PostUpdate`.
2. **Synchronous Asset Loading**: All loading is blocking. Pre-load adjacent rooms.
3. **WAV-only Audio**: No codec dependencies, but larger file sizes.
