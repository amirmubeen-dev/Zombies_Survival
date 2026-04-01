#pragma once
#include "Weapon.h"
#include "../core/Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>

struct Shotgun : Weapon {
    static const int PELLETS = 8;
    static constexpr float SPREAD = 0.10f; // radians

    Shotgun() {
        name        = "SHOTGUN";
        damage      = 15.0f; // per pellet
        fireRate    = 0.9f;
        ammo        = 999;
        maxAmmo     = 999;
        reserveAmmo = 9999;
        range       = 20.0f;
        reloadTime  = 2.0f;
    }

    void shoot(const Camera& cam, glm::vec3 spawnPos, std::vector<Bullet>& bullets, float& shakeAmount) override {
        if (!canShoot()) return;
        currentCooldown = fireRate;
        shakeAmount = 0.25f;
        muzzleFlashTimer = 0.08f;

        for (int p = 0; p < PELLETS; p++) {
            float rx = ((float)rand()/RAND_MAX - 0.5f) * 2.0f * SPREAD;
            float ry = ((float)rand()/RAND_MAX - 0.5f) * 2.0f * SPREAD;
            glm::vec3 dir = glm::normalize(cam.front + cam.getRight()*rx + cam.up*ry);

            Bullet b;
            b.weaponType = 1;
            b.pos = spawnPos;
            b.speed = 60.0f;
            b.vel = dir * b.speed;
            b.damage = damage;
            b.lifetime = 1.0f;
            b.alive = true;
            b.trailCount = 0;
            bullets.push_back(b);
        }
    }

    void drawViewModel(GLuint shader, const Mesh& box, float recoil, float aspect) override {
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 10.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 base = glm::translate(glm::mat4(1), {0.32f, -0.30f - recoil*0.2f, -0.65f});
        base = glm::rotate(base, glm::radians(-5.0f + recoil*25.0f), {1,0,0});

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,glm::value_ptr(proj));
        glUniform3f(glGetUniformLocation(shader,"lightPos"),0.5f,1.0f,0);
        glUniform3f(glGetUniformLocation(shader,"lightColor"),1,1,0.9f);
        glUniform1f(glGetUniformLocation(shader,"ambientStrength"),0.6f);
        glUniform3f(glGetUniformLocation(shader,"fogColor"),0,0,0);
        glUniform1f(glGetUniformLocation(shader,"alpha"),1.0f);
        glUniform1i(glGetUniformLocation(shader,"useTexture"),0);

        // Receiver (wide)
        drawPart(shader, box, base, {0.10f,0.09f,0.45f}, {0.18f,0.14f,0.10f});
        // Long barrel
        auto bar = glm::translate(base, {0,0.02f,-0.35f});
        drawPart(shader, box, bar, {0.045f,0.045f,0.38f}, {0.12f,0.12f,0.12f});
        // Under barrel
        auto under = glm::translate(base, {0,-0.03f,-0.25f});
        drawPart(shader, box, under, {0.035f,0.035f,0.3f}, {0.1f,0.1f,0.1f});
        // Handle
        auto hdl = glm::rotate(glm::translate(base,{0,-0.12f,0.12f}), glm::radians(18.0f),{1,0,0});
        drawPart(shader, box, hdl, {0.07f,0.16f,0.07f}, {0.2f,0.15f,0.1f});
    }

private:
    void drawPart(GLuint shader, const Mesh& box, glm::mat4 base, glm::vec3 scale, glm::vec3 col) {
        glm::mat4 m = glm::scale(base, scale);
        glUniformMatrix4fv(glGetUniformLocation(shader,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(shader,"objectColor"),1,glm::value_ptr(col));
        box.draw();
    }
};
