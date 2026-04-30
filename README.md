# Echoes of Slumber

A 2D action-platformer with metroidvania elements, built on a custom C++ engine using SDL3 and Box2D. Developed by **Hidden Dream Studio** (Group 4) for Projecte II at CITM — UPC (2025–2026).

## About the Game

In a dark, mysterious world woven from fragmented memories, the player explores interconnected environments through the **Anchor System** — a core mechanic centered on memory fragments that shape both narrative and gameplay. The game features melee combat, multiple enemy types, boss encounters, and environmental storytelling.

**Key mechanics:**
- I-frames on damage and during special states
- Melee combat system
- Three enemy archetypes: melee rush, melee weapon, ranged projectile
- Multi-phase boss encounters
- Checkpoint and save/load system
- Collectibles (coins, health, skill power-ups)

## Team — Hidden Dream Studio

| Name | Role | Degree |
|------|------|--------|
| Zakaria Hamdaoui | Lead Dev, Engine | Video Game Design and Dev. |
| Bruno Gómez Stirparo | Producer, Engine | Video Game Design and Dev. |
| Júlia Aunión Goy | Producer, UX | Video Game Design and Dev. |
| Marc Pladellorens Pérez | Gameplay Dev | Video Game Design and Dev. |
| Monse Medina Chávez | Lead Game Design | Video Game Design and Dev. |
| Aidan Albero García | Gameplay, Level Design | Video Game Design and Dev. |
| Albert Frederic Bailen Ruesca | Gameplay, Level Design | Video Game Design and Dev. |
| Bernat Loza Cardoner | Gameplay, Tech Artist | Video Game Design and Dev. |
| Víctor Pérez Ortega | Tech Artist | Design, Animation & Digital Art |
| Dottir Cheong Parlon | Lead Art | Design, Animation & Digital Art |
| Maria Guest Brillas | Character Art, UX | Design, Animation & Digital Art |
| Nadia Power Vilaseca | Character Art, Narrative | Design, Animation & Digital Art |
| Nohuely Trinidad Peguero | Character Art | Design, Animation & Digital Art |
| Mikel Sanahuja Guirao | Art, UX | Design, Animation & Digital Art |
| Maryna Antoniuk | Level Concept | Design, Animation & Digital Art |
| Nicolás Olmos Freijo | Level Concept | Design, Animation & Digital Art |
| Lara Sequera Martin | Level Concept, Narrative | Design, Animation & Digital Art |
| Armin De Biase Rodríguez | Level Concept, Level Design | Design, Animation & Digital Art |

## Tech Stack

| Library | Version | Purpose |
|---------|---------|---------|
| **SDL3** | 3.2.24+ | Rendering, input, audio, windowing |
| **SDL3_image** | 3.2.4+ | PNG / JPEG image loading |
| **Box2D** | 3.1.1+ | 2D physics simulation |
| **pugixml** | 1.15+ | XML/TMX parsing (maps, config) |
| **libjpeg-turbo** | 3.1.2+ | JPEG support |
| **libpng** | 1.2.37+ | PNG support |
| **Tracy** | 0.11.1+ | Performance profiling |

All dependencies are downloaded automatically by the Premake5 build system on first run.

## Engine Architecture

```
Engine (Singleton)
├── Window        — SDL3 window (1280×720, configurable via config.xml)
├── Input         — Keyboard & gamepad
├── Render        — SDL3 GPU rendering (OpenGL / Vulkan / D3D11 / D3D12)
├── Textures      — SDL3_image asset loading
├── Audio         — Music & SFX
├── Physics       — Box2D 3.x integration
├── Animation     — Frame-based sprite animation (TSX support)
├── Map           — TMX orthographic map loading
├── Scene         — Multi-scene management
├── EntityManager — Entity lifecycle (Player, Enemy, Item, Boss)
├── UIManager     — GUI system (menus, HUD, buttons)
└── Pathfinding   — A* / Dijkstra for enemy AI
```

All game entities inherit from a base `Entity` class and follow the lifecycle: `Awake() → Start() → Update() → PostUpdate() → CleanUp()`.

## Project Structure

```
EchoesOfSlumber/
├── src/                        # 23 source files
│   ├── PlatformGame.cpp        # Entry point
│   ├── Engine.cpp              # Core engine
│   ├── Player.cpp              # Player controller
│   ├── Enemy.cpp               # Enemy AI
│   ├── EntityManager.cpp       # Entity system
│   ├── Map.cpp                 # TMX map loader
│   ├── Physics.cpp             # Box2D wrapper
│   ├── Pathfinding.cpp         # A* pathfinding
│   ├── Scene.cpp               # Scene management
│   ├── UIManager.cpp / UIButton.cpp
│   ├── Animation.cpp           # Sprite animation
│   ├── Render.cpp / Window.cpp / Input.cpp
│   ├── Audio.cpp / Textures.cpp
│   ├── TracyMemory.cpp         # Profiling
│   └── Log.cpp / Timer.cpp / PerfTimer.cpp / Vector2D.cpp
│
├── include/                    # 24 header files
│
├── assets/
│   ├── textures/               # Spritesheets, tilesets
│   ├── maps/                   # TMX level files
│   └── audio/
│       ├── music/
│       └── fx/
│
├── build/
│   ├── premake5.lua            # Build config (auto-downloads dependencies)
│   ├── premake5.exe
│   ├── build_files/            # Generated VS projects
│   └── external/               # Downloaded libraries
│
├── .github/
│   ├── workflows/codeql.yml    # CodeQL C++ security scanning
│   ├── CODEOWNERS              # Auto PR reviewer assignment by area
│   ├── ISSUE_TEMPLATE/         # Bug, feature, art-asset templates
│   └── pull_request_template.md
│
├── config.xml                  # Game settings (FPS, resolution, vsync)
├── build-VisualStudio2022.bat  # One-click build script
├── LICENSE                     # MIT
└── README.md
```

## Quick Start

### Windows

```bash
git clone https://github.com/HiddenDreamStudio/EchoesOfSlumber.git
cd EchoesOfSlumber
.\build-VisualStudio2022.bat
```

This downloads all dependencies, generates the VS 2022 solution, and opens it. Press **F5** to build and run (`x64 Debug` recommended).

### Linux

```bash
git clone https://github.com/HiddenDreamStudio/EchoesOfSlumber.git
cd EchoesOfSlumber/build
./premake5 gmake2
cd ..
make config=debug_x64
./bin/Debug/EchoesOfSlumber
```

### Rendering Backend

```bash
premake5 vs2022 --sdl_backend=opengl   # default
premake5 vs2022 --sdl_backend=vulkan
premake5 vs2022 --sdl_backend=d3d11    # Windows only
premake5 vs2022 --sdl_backend=d3d12    # Windows only
```

## Controls

| Key | Action |
|-----|--------|
| `WASD` | Move |
| `SPACE` | Jump |
| `ESC` | Pause |

### Debug Keys

| Key | Function |
|-----|----------|
| `H` | Help overlay |
| `F1` / `F2` | Jump to level 1 / 2 |
| `F5` / `F6` | Save / Load |
| `F8` | GUI bounds |
| `F9` | Colliders & pathfinding |
| `F10` | God mode |
| `F11` | Toggle FPS cap (30/60) |

## Milestones

| Milestone | Date | Focus |
|-----------|------|-------|
| **Concept Discovery** | 12 Mar 2026 | GDD, TDD, Production Plan, Moodboard |
| **Vertical Slice** | 16 Apr 2026 | Core gameplay loop, player, map, physics |
| **Alpha** | 21 May 2026 | Enemy AI, save/load, audio, combat |
| **Gold** | 14 Jun 2026 | Full levels, boss, GUI, optimization, polish |

## CI/CD

- **CodeQL** runs on every push/PR to `main` — automated C++ security analysis.
- **CODEOWNERS** enforces per-area review: engine code → dev-engineers, art assets → art-creators, docs → leads.

## License

[MIT](LICENSE) — Copyright (c) 2025 Hidden Dream Studio. All rights reserved.