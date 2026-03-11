# 🧟 Parallel Zombie Survival
### Semester Project — Parallel & Distributed Computing
**C++ | OpenMP | MPI | OpenGL (GPU)**

## 👤 Author
Semester Project — AMIR MUBEEN (FA23-BSE-032) & AHMAD HASSAN (FA23-BSE-018 ) 

---
/*
 *   PARALLEL ZOMBIE SURVIVAL - Semester Project

 * COMPILE (MSYS2 MinGW64 terminal):
 *   pacman -S mingw-w64-x86_64-freeglut 
 * -------------------------------------------------
 *   g++ main.cpp -o zombie_survival.exe -lfreeglut -lopengl32 -lglu32 -fopenmp -std=c++17
 * ./zombie_survival.exe  (run the game)
 * -------------------------------------------------

 */

## 📌 How to Compile & Run

### Requirements
- Windows 10/11
- MinGW GCC compiler (`g++`)
- **freeglut** library

### Step 1 — Install freeglut
1. Download from: https://www.transmissionzero.co.uk/software/freeglut-devel/
2. Extract and copy:
   - `include/GL/` → `C:/MinGW/include/GL/`
   - `lib/*.a` → `C:/MinGW/lib/`
   - `bin/freeglut.dll` → same folder as your `.exe`

### Step 2 — Compile
```bash
g++ main.cpp -o zombie_survival.exe -lfreeglut -lopengl32 -lglu32 -fopenmp -std=c++17
```

### Step 3 — Run
```bash
zombie_survival.exe
```

---

## 🎮 Controls
| Key | Action |
|-----|--------|
| W / A / S / D | Move player |
| Arrow Keys | Move player (alternative) |
| Left Click | Shoot toward mouse cursor |
| ENTER | Start / Restart game |
| ESC | Quit |

---

## ⚙️ Parallel Computing Concepts Used

### 1. OpenMP (CPU Multi-threading)
- `#pragma omp parallel for` used in `updateZombies_OpenMP()`
- 4 threads process zombie movement simultaneously
- Also used in `checkCollisions()` and `updateParticles_OpenMP()`
- `#pragma omp atomic` ensures thread-safe score updates

### 2. MPI (Simulated Domain Decomposition)
- Map divided into 63 zones (9×7 grid)
- `mpi_zone_analysis_simulate()` mimics MPI_Reduce across processes
- Hot zones (high zombie density) highlighted in red overlay
- In production MPI, each zone = separate MPI process

### 3. OpenGL — GPU Programming
- All rendering done on GPU via OpenGL
- GPU handles: circles, quads, particles, color blending
- `GL_BLEND` for transparency effects
- `GL_TRIANGLE_FAN` for smooth circle rendering
- Double buffering (`GLUT_DOUBLE`) for smooth GPU-driven animation

---

## 🧟 Zombie Types
| Type | Color | Speed | Health | Points |
|------|-------|-------|--------|--------|
| Normal | Green | Medium | 1 | 10 |
| Fast | Orange | Fast | 1 | 20 |
| Tank | Purple | Slow | 3+ | 30 |

---

## 📈 Wave System
- Score 100 × wave → next wave begins
- Each wave: faster spawning, tougher tanks
- Max 200 zombies on screen at once

---

