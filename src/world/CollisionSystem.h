#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>

struct AABB {
    glm::vec3 min_, max_;

    bool contains(glm::vec3 p) const {
        return p.x >= min_.x && p.x <= max_.x &&
               p.y >= min_.y && p.y <= max_.y &&
               p.z >= min_.z && p.z <= max_.z;
    }

    bool intersectsSphere(glm::vec3 center, float r) const {
        float dx = std::max(min_.x - center.x, std::max(0.0f, center.x - max_.x));
        float dy = std::max(min_.y - center.y, std::max(0.0f, center.y - max_.y));
        float dz = std::max(min_.z - center.z, std::max(0.0f, center.z - max_.z));
        return (dx*dx + dy*dy + dz*dz) <= r*r;
    }

    // Closest point on AABB surface
    glm::vec3 closestPoint(glm::vec3 p) const {
        return {
            std::max(min_.x, std::min(p.x, max_.x)),
            std::max(min_.y, std::min(p.y, max_.y)),
            std::max(min_.z, std::min(p.z, max_.z))
        };
    }
};

class CollisionSystem {
public:
    std::vector<glm::vec3> treePositions;
    std::vector<float>     treeRadii;
    std::vector<AABB>      buildings;
    float                  mapRadius = 50.0f;

    void clear() {
        treePositions.clear();
        treeRadii.clear();
        buildings.clear();
    }

    void addTree(glm::vec3 pos, float radius) {
        treePositions.push_back(pos);
        treeRadii.push_back(radius);
    }

    void addBuilding(AABB box) {
        buildings.push_back(box);
    }

    // Resolve player sphere vs all obstacles; returns adjusted position
    glm::vec3 resolvePlayer(glm::vec3 oldPos, glm::vec3 newPos, float playerRadius = 0.5f) const {
        glm::vec3 result = newPos;

        // Map boundary (circular)
        glm::vec2 flat = {result.x, result.z};
        float dist = glm::length(flat);
        if (dist > mapRadius - playerRadius) {
            flat = glm::normalize(flat) * (mapRadius - playerRadius);
            result.x = flat.x;
            result.z = flat.y;
        }

        // Trees — sphere vs cylinder (parallel for many trees)
        int n = (int)treePositions.size();
        #pragma omp parallel for if(n > 50) schedule(static)
        for (int i = 0; i < n; i++) {
            glm::vec2 tp = {treePositions[i].x, treePositions[i].z};
            glm::vec2 pp = {result.x, result.z};
            float r = treeRadii[i] + playerRadius;
            glm::vec2 diff = pp - tp;
            float len = glm::length(diff);
            if (len < r && len > 0.001f) {
                #pragma omp critical
                {
                    glm::vec2 pushed = tp + glm::normalize(diff) * r;
                    result.x = pushed.x;
                    result.z = pushed.y;
                }
            }
        }

        // Buildings (AABB)
        for (auto& b : buildings) {
            if (b.intersectsSphere(result, playerRadius)) {
                glm::vec3 cp = b.closestPoint(result);
                glm::vec3 dir = result - cp;
                float len = glm::length(dir);
                if (len < playerRadius && len > 0.001f) {
                    result = cp + glm::normalize(dir) * playerRadius;
                }
            }
        }

        return result;
    }

    // Raycast — returns true + hitDist if any obstacle is intersected
    bool raycast(glm::vec3 origin, glm::vec3 dir, float maxDist, float& hitDist) const {
        hitDist = maxDist;
        bool hit = false;

        // Trees (cylinder approx as sphere on ground)
        for (int i = 0; i < (int)treePositions.size(); i++) {
            glm::vec3 oc = origin - treePositions[i];
            float a = dir.x*dir.x + dir.z*dir.z;
            float b = 2*(oc.x*dir.x + oc.z*dir.z);
            float c = oc.x*oc.x + oc.z*oc.z - treeRadii[i]*treeRadii[i];
            float disc = b*b - 4*a*c;
            if (disc < 0) continue;
            float t = (-b - sqrtf(disc)) / (2*a);
            if (t > 0.01f && t < hitDist) { hitDist = t; hit = true; }
        }

        // Buildings (AABB slab test)
        for (auto& bx : buildings) {
            float tmin = 0, tmax = maxDist;
            for (int a = 0; a < 3; a++) {
                float o  = (&origin.x)[a];
                float d  = (&dir.x)[a];
                float mn = (&bx.min_.x)[a];
                float mx = (&bx.max_.x)[a];
                if (fabsf(d) < 1e-6f) { if (o < mn || o > mx) { tmin = tmax + 1; break; } continue; }
                float t1 = (mn - o) / d, t2 = (mx - o) / d;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
            }
            if (tmin <= tmax && tmin > 0.01f && tmin < hitDist) { hitDist = tmin; hit = true; }
        }

        return hit;
    }

    bool insideMap(glm::vec3 pos) const {
        return glm::length(glm::vec2{pos.x, pos.z}) < mapRadius;
    }
};
