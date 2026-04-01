#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

const float MAP_SIZE = 300.0f;
const float PI_MAP   = 3.14159265f;

enum PropType {
    PROP_TREE = 0,
    PROP_DEAD_TREE = 1,
    PROP_RUIN = 2,
    PROP_CRATE = 3,
    PROP_CAR = 4,
    PROP_CAMPFIRE = 5
};

struct Prop {
    PropType  type;
    glm::vec3 pos;
    glm::vec3 size; // scale/extents depending on type
    float     rotation;
    int       style; // material/color variation
};

struct MapItem {
    int type; // 1=bandage, 2=medkit, 3=armor
    glm::vec3 pos;
    float timeAlive = 0.0f;
};

class Map {
public:
    std::vector<Prop> props;
    std::vector<MapItem> items;
    
    // PUBG Safe Zone
    glm::vec3 safeCenter;
    float     safeRadius = 300.0f;
    float     nextSafeRadius = 200.0f;
    float     zonePhaseTimer = 120.0f; // 2 minutes per shrink
    bool      zoneShrinking = false;

    void generate(int propCnt = 400, int itemCnt = 15) {
        props.clear();
        items.clear();
        safeCenter = {0,0,0};
        safeRadius = MAP_SIZE;
        nextSafeRadius = MAP_SIZE * 0.7f;
        zonePhaseTimer = 60.0f; // first shrink happens fast

        for (int i = 0; i < propCnt; i++) {
            Prop p;
            float angle  = ((float)rand() / RAND_MAX) * 2 * PI_MAP;
            float radius = 5.0f + ((float)rand() / RAND_MAX) * (MAP_SIZE - 20.0f);
            
            p.pos = {cosf(angle) * radius, getTerrainHeight(cosf(angle)*radius, sinf(angle)*radius), sinf(angle) * radius};
            p.rotation = ((float)rand() / RAND_MAX) * 360.0f;
            
            int rnd = rand() % 100;
            if (rnd < 40) {
                p.type = PROP_TREE;
                p.size = {0.2f + rand()%10*0.02f, 4.0f + rand()%6, 1.5f + rand()%2}; // radius, height, leaf
            } else if (rnd < 60) {
                p.type = PROP_DEAD_TREE;
                p.size = {0.25f, 3.0f + rand()%4, 0.0f};
            } else if (rnd < 75) {
                p.type = PROP_CRATE;
                p.size = {0.8f + rand()%5*0.1f, 0.8f + rand()%5*0.1f, 0.8f + rand()%5*0.1f};
            } else if (rnd < 90) {
                p.type = PROP_RUIN;
                p.size = {3.0f + rand()%3, 2.0f + rand()%2, 3.0f + rand()%3}; // half-extents
                p.style = rand() % 3;
            } else if (rnd < 98) {
                p.type = PROP_CAR;
                p.size = {1.2f, 0.8f, 2.8f}; // half extents
                p.style = rand() % 4; // rusty colors
            } else {
                p.type = PROP_CAMPFIRE;
                p.size = {0.4f, 0.2f, 0.4f};
            }
            props.push_back(p);
        }

        spawnItems(itemCnt);
    }

    void spawnItems(int count) {
        for (int i = 0; i < count; i++) {
            MapItem it;
            float angle  = ((float)rand() / RAND_MAX) * 2 * PI_MAP;
            float radius = 5.0f + ((float)rand() / RAND_MAX) * (safeRadius - 10.0f);
            it.pos = {cosf(angle) * radius, 0.0f, sinf(angle) * radius};
            it.pos.y = getTerrainHeight(it.pos.x, it.pos.z) + 0.2f;
            int r = rand() % 100;
            if (r < 40) it.type = 1;      // bandage
            else if (r < 75) it.type = 3; // armor
            else it.type = 2;             // medkit
            items.push_back(it);
        }
    }

    void update(float dt) {
        for (auto& it : items) it.timeAlive += dt;

        zonePhaseTimer -= dt;
        if (zonePhaseTimer <= 0) {
            if (!zoneShrinking) {
                zoneShrinking = true;
                zonePhaseTimer = 30.0f; // 30 seconds to shrink
            } else {
                zoneShrinking = false;
                safeRadius = nextSafeRadius;
                nextSafeRadius *= 0.6f;
                // pick new safe center inside current safeRadius
                float angle = ((float)rand() / RAND_MAX) * 2 * PI_MAP;
                float r = ((float)rand() / RAND_MAX) * (safeRadius - nextSafeRadius);
                safeCenter += glm::vec3(cosf(angle)*r, 0, sinf(angle)*r);
                zonePhaseTimer = 90.0f; // Wait 90s for next shrink
            }
        }
        if (zoneShrinking) {
            float t = 1.0f - (zonePhaseTimer / 30.0f); // 0 to 1
            safeRadius = std::max(10.0f, safeRadius - dt * (safeRadius - nextSafeRadius) / 30.0f);
        }
        
        // Medkit respawn logic (max 5 medkits total)
        int mkCount = 0;
        for (auto& it : items) { if (it.type == 2) mkCount++; }
        if (mkCount < 5 && (rand() % 600) == 0) { // roughly every 10s based on dt=0.016
            spawnItems(1);
            if (items.back().type != 2) items.back().type = 2; // force medkit
        }
    }

    float getTerrainHeight(float x, float z) const {
        // Subtle sine wave terrain variation
        return sinf(x * 0.05f) * 1.5f + cosf(z * 0.04f) * 1.2f;
    }
};
