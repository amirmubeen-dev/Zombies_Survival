#pragma once
#include "../core/Types.h"
#include "../entities/Zombie.h"
#include "../renderer/Camera.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cmath>

// Real physical bullets
struct Bullet {
    glm::vec3 pos;
    glm::vec3 vel;
    float     speed;
    float     damage;
    float     lifetime;   // max 3 seconds
    bool      alive;
    int       weaponType; // 0=Pistol, 1=Shotgun, 2=Rifle
    
    // Trail for rendering
    glm::vec3 trailPos[8];
    int       trailCount;
    
    void updateTrail() {
        for (int i = 7; i > 0; i--) { trailPos[i] = trailPos[i-1]; }
        trailPos[0] = pos;
        if (trailCount < 8) trailCount++;
    }
};

struct Weapon {
    std::string name;
    float       damage;
    float       fireRate;
    int         ammo;
    int         maxAmmo;
    int         reserveAmmo;
    float       range;
    float       reloadTime;

    float       currentCooldown = 0.0f;
    bool        reloading       = false;
    float       reloadTimer     = 0.0f;
    
    float       muzzleFlashTimer = 0.0f;

    virtual ~Weapon() = default;

    void update(float dt) {
        if (currentCooldown > 0) currentCooldown -= dt;
        if (muzzleFlashTimer > 0) muzzleFlashTimer -= dt;
        if (reloading) {
            reloadTimer -= dt;
            if (reloadTimer <= 0) {
                reloading = false;
                int needed = maxAmmo - ammo;
                int take = std::min(needed, reserveAmmo);
                ammo += take;
                reserveAmmo -= take;
            }
        }
    }

    bool canShoot() const { return ammo > 0 && currentCooldown <= 0 && !reloading; }

    void startReload() {
        if (ammo < maxAmmo && reserveAmmo > 0 && !reloading) {
            reloading = true;
            reloadTimer = reloadTime;
        }
    }

    // Spawn physical bullets instead of raycasting
    virtual void shoot(const Camera& cam, glm::vec3 spawnPos, std::vector<Bullet>& bullets, float& shakeAmount) = 0;

    // Viewmodel override
    virtual void drawViewModel(unsigned int shader, const struct Mesh& box, float recoil, float aspect) = 0;
};
