#pragma once
#include "Weapon.h"
#include "../core/Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Rifle : Weapon {
    Rifle() {
        name        = "RIFLE";
        damage      = 40.0f;
        fireRate    = 0.1f;
        ammo        = 999;
        maxAmmo     = 999;
        reserveAmmo = 9999;
        range       = 100.0f;
        reloadTime  = 2.2f;
    }

    void shoot(const Camera& cam, glm::vec3 spawnPos, std::vector<Bullet>& bullets, float& shakeAmount) override {
        if (!canShoot()) return;
        currentCooldown = fireRate;
        shakeAmount = 0.06f;
        muzzleFlashTimer = 0.05f;

        Bullet b;
        b.weaponType = 2; // Rifle
        b.pos = spawnPos;
        b.speed = 150.0f; // Very fast
        b.vel = cam.front * b.speed;
        b.damage = damage;
        b.lifetime = 3.0f;
        b.alive = true;
        b.trailCount = 0;
        bullets.push_back(b);
    }

    void drawViewModel(GLuint shader, const Mesh& box, float recoil, float aspect) override {
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 10.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 base = glm::translate(glm::mat4(1), {0.30f, -0.26f - recoil*0.10f, -0.75f});
        base = glm::rotate(base, glm::radians(-4.0f + recoil*15.0f), {1,0,0});

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,glm::value_ptr(proj));
        glUniform3f(glGetUniformLocation(shader,"lightPos"),0.5f,1.0f,0);
        glUniform3f(glGetUniformLocation(shader,"lightColor"),1,1,0.9f);
        glUniform1f(glGetUniformLocation(shader,"ambientStrength"),0.6f);
        glUniform3f(glGetUniformLocation(shader,"fogColor"),0,0,0);
        glUniform1f(glGetUniformLocation(shader,"alpha"),1.0f);
        glUniform1i(glGetUniformLocation(shader,"useTexture"),0);

        // Long body
        drawPart(shader, box, base, {0.055f,0.075f,0.65f}, {0.12f,0.12f,0.12f});
        // Barrel extension
        auto bar = glm::translate(base, {0,0.01f,-0.45f});
        drawPart(shader, box, bar, {0.025f,0.025f,0.40f}, {0.08f,0.08f,0.08f});
        // Scope box on top
        auto scope = glm::translate(base, {0,0.07f,-0.1f});
        drawPart(shader, box, scope, {0.03f,0.055f,0.18f}, {0.05f,0.05f,0.05f});
        // Scope glass (slightly blue)
        auto glass = glm::translate(base, {0,0.075f,-0.19f});
        drawPart(shader, box, glass, {0.025f,0.025f,0.025f}, {0.1f,0.2f,0.5f});
        // Handle
        auto hdl = glm::rotate(glm::translate(base,{0,-0.10f,0.15f}), glm::radians(15.0f),{1,0,0});
        drawPart(shader, box, hdl, {0.05f,0.14f,0.06f}, {0.2f,0.15f,0.1f});
        // Mag
        auto mag = glm::translate(base, {0,-0.08f,-0.05f});
        drawPart(shader, box, mag, {0.04f,0.10f,0.055f}, {0.15f,0.15f,0.15f});
    }

private:
    void drawPart(GLuint shader, const Mesh& box, glm::mat4 base, glm::vec3 scale, glm::vec3 col) {
        glm::mat4 m = glm::scale(base, scale);
        glUniformMatrix4fv(glGetUniformLocation(shader,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(shader,"objectColor"),1,glm::value_ptr(col));
        box.draw();
    }
};
