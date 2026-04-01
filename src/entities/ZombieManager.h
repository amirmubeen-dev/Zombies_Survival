#pragma once
#include "Zombie.h"
#include "../renderer/Camera.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <omp.h>

const float PI_Z = 3.14159265f;
const int   MAX_ZOMBIES = 200;

class ZombieManager {
public:
    std::vector<Zombie> zombies;
    int ompThreads = 4;

    // Zone tracking (MPI simulation)
    static const int ZONE_COLS = 8, ZONE_ROWS = 8;
    int zoneActivity[ZONE_COLS * ZONE_ROWS] = {};

    void spawn(int wave) {
        if ((int)zombies.size() >= MAX_ZOMBIES) return;
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI_Z;
        float dist  = MAP_RADIUS * 0.85f;
        glm::vec3 p = {cosf(angle) * dist, 0.9f, sinf(angle) * dist};

        int r = rand() % 10;
        if (r < 6)      zombies.push_back(Zombie::createWalker(p));
        else if (r < 9) zombies.push_back(Zombie::createRunner(p));
        else            zombies.push_back(Zombie::createTank(p));
    }

    void spawnN(int n, int wave) {
        for (int i = 0; i < n; i++) spawn(wave);
    }

    // Returns total damage dealt to player this frame
    void updateAll(float dt, glm::vec3& playerPos, float& playerHealth, float& shakeAmount,
                   bool& playerAlive, int& gameState)
    {
        int n = (int)zombies.size();
        for (int i = 0; i < ZONE_COLS * ZONE_ROWS; i++) zoneActivity[i] = 0;

        #pragma omp parallel for num_threads(4) schedule(dynamic, 4)
        for (int i = 0; i < n; i++) {
            if (!zombies[i].alive) continue;

            glm::vec3 toPlayer = playerPos - zombies[i].pos;
            toPlayer.y = 0;
            float dist = glm::length(toPlayer);

            if (dist > 0.1f) {
                glm::vec3 dir = glm::normalize(toPlayer);
                zombies[i].vel.x = dir.x * zombies[i].speed;
                zombies[i].vel.z = dir.z * zombies[i].speed;
                zombies[i].angle = atan2f(dir.x, dir.z) * (180.0f / PI_Z);
            }

            zombies[i].pos.x += zombies[i].vel.x * dt;
            zombies[i].pos.z += zombies[i].vel.z * dt;
            zombies[i].pos.y  = 0.9f;

            zombies[i].walkAnim += dt * zombies[i].speed * 3.0f;

            if (zombies[i].flashTimer > 0) zombies[i].flashTimer -= dt;
            else zombies[i].flashing = false;

            if (zombies[i].attackCooldown > 0) zombies[i].attackCooldown -= dt;

            // Zone tracking
            int zx = (int)((zombies[i].pos.x + MAP_RADIUS) / (2 * MAP_RADIUS) * ZONE_COLS);
            int zz = (int)((zombies[i].pos.z + MAP_RADIUS) / (2 * MAP_RADIUS) * ZONE_ROWS);
            zx = std::max(0, std::min(zx, ZONE_COLS - 1));
            zz = std::max(0, std::min(zz, ZONE_ROWS - 1));
            #pragma omp atomic
            zoneActivity[zz * ZONE_COLS + zx]++;

            // Attack player
            if (dist < 1.5f && zombies[i].attackCooldown <= 0.0f) {
                #pragma omp critical
                {
                    playerHealth -= zombies[i].damage;
                    shakeAmount   = 0.3f;
                    zombies[i].attackCooldown = 1.0f;
                    if (playerHealth <= 0) {
                        playerAlive = false;
                        gameState   = 2;
                    }
                }
            }
        }
        ompThreads = omp_get_max_threads();

        // Remove fully dead
        zombies.erase(
            std::remove_if(zombies.begin(), zombies.end(),
                [](const Zombie& z){ return !z.alive && z.isDying && z.deathTimer > 3.0f; }),
            zombies.end());
    }

    int countAlive() const {
        int c = 0;
        for (auto& z : zombies) if (z.alive) c++;
        return c;
    }

    int getHotZones() const {
        int hot = 0;
        for (int i = 0; i < ZONE_COLS * ZONE_ROWS; i++)
            if (zoneActivity[i] > 2) hot++;
        return hot;
    }

    void clear() { zombies.clear(); }
};
