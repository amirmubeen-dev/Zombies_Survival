#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cstring>

// Simulated MPI domain decomposition — no MPI runtime required.
// Divides the map into a grid of zones, tracks hot zones where zombies cluster.
class MPIZoneManager {
public:
    static const int COLS = 8;
    static const int ROWS = 8;
    static const int TOTAL_ZONES = COLS * ROWS;

    int   activity[TOTAL_ZONES] = {};
    int   rankMap[TOTAL_ZONES]  = {}; // simulated MPI rank assignment
    float mapSize  = 50.0f;
    int   numRanks = 4; // simulated process count

    void init(float mapSz, int ranks = 4) {
        mapSize  = mapSz;
        numRanks = ranks;
        reset();
        // Assign zones to simulated ranks (round-robin)
        for (int i = 0; i < TOTAL_ZONES; i++) rankMap[i] = i % numRanks;
    }

    void reset() { memset(activity, 0, sizeof(activity)); }

    void recordZomble(float x, float z) {
        int zx = (int)((x + mapSize) / (2.0f * mapSize) * COLS);
        int zz = (int)((z + mapSize) / (2.0f * mapSize) * ROWS);
        zx = std::max(0, std::min(zx, COLS-1));
        zz = std::max(0, std::min(zz, ROWS-1));
        activity[zz * COLS + zx]++;
    }

    int getHotZones(int threshold = 2) const {
        int n = 0;
        for (int i = 0; i < TOTAL_ZONES; i++) if (activity[i] > threshold) n++;
        return n;
    }

    // Get center position of a zone
    glm::vec3 getZoneCenter(int zoneIdx) const {
        int zx = zoneIdx % COLS;
        int zz = zoneIdx / COLS;
        float cx = -mapSize + (zx + 0.5f) * (2.0f * mapSize / COLS);
        float cz = -mapSize + (zz + 0.5f) * (2.0f * mapSize / ROWS);
        return {cx, 0, cz};
    }

    float getZoneSize() const { return 2.0f * mapSize / COLS; }
    int   getRank(int zoneIdx) const { return rankMap[zoneIdx % TOTAL_ZONES]; }
};
