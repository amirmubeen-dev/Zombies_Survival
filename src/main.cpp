// ============================================================
// 3D Zombie Survival — Entry Point
// ============================================================
// Libraries: OpenGL 3.3 + GLFW + GLEW + GLM + OpenMP
// Compiler: MinGW64 GCC (MSYS2) / MSVC
// ============================================================

#include "Game.h"

int main() {
    std::cout << "=== 3D Zombie Survival ===\n";
    std::cout << "[OpenMP] Max threads: " << omp_get_max_threads() << "\n";

    Game game;
    if (!game.init()) {
        std::cerr << "[Fatal] Game init failed\n";
        return -1;
    }

    game.run();
    return 0;
}
