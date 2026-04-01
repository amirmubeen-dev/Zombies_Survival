# 🧟 3D Zombie Survival

A first-person zombie survival game built in C++ with OpenGL 3.3, OpenMP, and simulated MPI zones.

## Features

| System | Details |
|--------|---------|
| **Rendering** | OpenGL 3.3, GLFW, GLEW, Phong lighting, exponential fog, skybox gradient |
| **Weapons** | Pistol · Shotgun (8-pellet spread) · Rifle (scoped, 100m range) |
| **Zombies** | Walker · Runner · Tank; OpenMP parallel AI update |
| **World** | Jungle map, procedural trees, buildings, boundary walls |
| **Day/Night** | Full day cycle with dynamic sun color, fog density, ambient |
| **Particles** | Blood splatter, muzzle sparks, procedural |
| **Parallel** | OpenMP zombie AI + simulated MPI 8×8 zone grid |
| **Audio** | OpenAL (optional; graceful no-op fallback) |

## Controls

| Key | Action |
|-----|--------|
| `WASD` | Move |
| Mouse | Look |
| `LMB` | Shoot |
| `1 / 2 / 3` | Switch weapons (Pistol / Shotgun / Rifle) |
| Scroll | Cycle weapons |
| `R` | Reload |
| `ENTER` | Start / Restart |
| `ESC` | Release mouse / Quit |

## Build Instructions (MSYS2 MinGW64)

### Prerequisites

Open MSYS2 MinGW64 shell and install dependencies:

```bash
pacman -S mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-glfw \
          mingw-w64-x86_64-glew \
          mingw-w64-x86_64-glm \
          mingw-w64-x86_64-openal
```

### Build

```bash
cd /d/zombies/zombie3d
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
```

### Run

```bash
./ZombieSurvival3D.exe
```

## Project Structure

```
zombie3d/
├── src/
│   ├── main.cpp              ← Entry point
│   ├── Game.h                ← Core FSM + all subsystem integration
│   ├── core/
│   │   ├── Window.h          ← GLFW wrapper
│   │   ├── Shader.h          ← GLSL compiler
│   │   ├── Mesh.h            ← VAO/VBO + OBJ loader
│   │   └── Texture.h         ← Texture loader + procedural fallback
│   ├── renderer/
│   │   ├── Camera.h          ← First-person camera
│   │   ├── Renderer.h        ← 3D world draw pipeline
│   │   ├── Skybox.h          ← Cubemap + gradient fallback
│   │   ├── ParticleSystem.h  ← Blood / sparks / smoke
│   │   └── HUD.h             ← 2D bars, crosshair, vignette
│   ├── world/
│   │   ├── Map.h             ← Procedural map generation
│   │   ├── DayNight.h        ← Sun cycle controller
│   │   ├── WaveManager.h     ← Zombie wave spawner
│   │   └── CollisionSystem.h ← Sphere + AABB collision + raycast
│   ├── entities/
│   │   ├── Entity.h          ← Base entity
│   │   ├── Player.h          ← Player state + movement
│   │   ├── Zombie.h          ← Zombie types (walker/runner/tank)
│   │   ├── ZombieManager.h   ← OpenMP parallel AI
│   │   └── Animation.h       ← Skeletal animation system
│   ├── weapons/
│   │   ├── Weapon.h          ← Abstract base
│   │   ├── Pistol.h          ← 25dmg, 12 ammo
│   │   ├── Shotgun.h         ← 8 pellets, spread cone
│   │   ├── Rifle.h           ← 40dmg, scoped, 100m
│   │   └── WeaponManager.h   ← Weapon switcher
│   ├── audio/
│   │   └── AudioManager.h    ← OpenAL singleton (optional)
│   └── parallel/
│       ├── OpenMPManager.h   ← Thread pool helper
│       └── MPIZoneManager.h  ← Simulated domain decomposition
├── CMakeLists.txt
└── .vscode/
    ├── tasks.json
    └── launch.json
```

## Architecture Notes

- **Single translation unit build** — all `.h` files contain inline implementations; `main.cpp` is the only `.cpp` that matters. The CMake GLOB ensures any future `.cpp` additions are picked up.
- **No MPI runtime required** — the zone grid is simulated with atomic arrays (compatible with the original design).
- **Day/night** cycle speed configurable via `dayNight.setSpeed()` in `Game::init()`.
- **OpenAL** is optional — define `HAS_OPENAL` (auto-detected by CMake) to enable sound.
