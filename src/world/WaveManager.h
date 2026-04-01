#pragma once
#include "../entities/ZombieManager.h"
#include <cmath>
#include <algorithm>

struct WaveConfig {
    int numZombies;
    int numRunners;
    int numTanks;
    float spawnRate; // seconds between spawns
};

class WaveManager {
public:
    int   currentWave    = 1;
    float timeBetweenWaves = 15.0f;
    float waveTimer       = 5.0f;   // time until first wave
    int   zombiesThisWave = 0;
    int   zombiesSpawned  = 0;
    int   zombiesKilled   = 0;
    float spawnTimer      = 0.0f;
    bool  waveActive      = false;

    // Wave announcement (for HUD fade)
    float waveAnnounceTimer = 0.0f;

    WaveConfig getWaveConfig(int wave) const {
        WaveConfig c;
        c.numZombies = std::min(10 + wave * 5, 60);
        c.numRunners = wave > 2 ? std::min(wave * 2, 15) : 0;
        c.numTanks   = wave > 3 ? std::min(wave - 3, 8)  : 0;
        c.spawnRate  = std::max(0.4f, 2.5f - wave * 0.15f);
        return c;
    }

    void startNextWave() {
        currentWave++;
        WaveConfig cfg = getWaveConfig(currentWave);
        zombiesThisWave = cfg.numZombies;
        zombiesSpawned  = 0;
        zombiesKilled   = 0;
        spawnTimer      = 0.0f;
        waveActive      = true;
        waveAnnounceTimer = 3.0f;
    }

    void startFirstWave() {
        WaveConfig cfg = getWaveConfig(currentWave);
        zombiesThisWave = cfg.numZombies;
        zombiesSpawned  = 0;
        zombiesKilled   = 0;
        spawnTimer      = 0.0f;
        waveActive      = true;
        waveAnnounceTimer = 3.0f;
    }

    bool isWaveComplete() const {
        return waveActive && zombiesSpawned >= zombiesThisWave && zombiesKilled >= zombiesThisWave;
    }

    void onZombieKilled() { zombiesKilled++; }

    void update(float dt, ZombieManager& zm) {
        if (waveAnnounceTimer > 0) waveAnnounceTimer -= dt;

        if (!waveActive) {
            waveTimer -= dt;
            if (waveTimer <= 0) {
                waveTimer = timeBetweenWaves;
                startNextWave();
            }
            return;
        }

        // Spawn zombies periodically
        if (zombiesSpawned < zombiesThisWave) {
            spawnTimer -= dt;
            WaveConfig cfg = getWaveConfig(currentWave);
            if (spawnTimer <= 0) {
                spawnTimer = cfg.spawnRate;
                zm.spawn(currentWave);
                zombiesSpawned++;
            }
        }

        // Check wave completion
        if (isWaveComplete()) {
            waveActive = false;
            waveTimer  = timeBetweenWaves;
        }
    }

    float getWaveProgress() const {
        if (zombiesThisWave <= 0) return 1.0f;
        return (float)zombiesKilled / zombiesThisWave;
    }
};
