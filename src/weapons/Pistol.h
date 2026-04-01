#pragma once
#include "Weapon.h"
#include "../core/Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Pistol : Weapon {
    Pistol() {
        name        = "PISTOL";
        damage      = 25.0f;
        fireRate    = 0.3f;
        ammo        = 999;
        maxAmmo     = 999;
        reserveAmmo = 9999;
        range       = 50.0f;
        reloadTime  = 1.2f;
    }

    void shoot(const Camera& cam, glm::vec3 spawnPos, std::vector<Bullet>& bullets, float& shakeAmount) override {
        if (!canShoot()) return;
        currentCooldown = fireRate;
        shakeAmount = 0.05f;
        muzzleFlashTimer = 0.05f;

        Bullet b;
        b.weaponType = 0;
        b.pos = spawnPos;
        b.speed = 80.0f;
        b.vel = cam.front * b.speed;
        b.damage = damage;
        b.lifetime = 3.0f;
        b.alive = true;
        b.trailCount = 0;
        bullets.push_back(b);
    }

    void drawViewModel(GLuint shader, const Mesh& box, float recoil, float aspect) override {
        glm::mat4 proj  = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 10.0f);
        glm::mat4 view  = glm::mat4(1.0f);
        glm::mat4 base  = glm::translate(glm::mat4(1), {0.35f, -0.28f - recoil*0.15f, -0.7f});
        base = glm::rotate(base, glm::radians(-5.0f + recoil*20.0f), {1,0,0});

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,glm::value_ptr(proj));
        glUniform3f(glGetUniformLocation(shader,"lightPos"),0.5f,1.0f,0);
        glUniform3f(glGetUniformLocation(shader,"lightColor"),1,1,0.9f);
        glUniform1f(glGetUniformLocation(shader,"ambientStrength"),0.6f);
        glUniform3f(glGetUniformLocation(shader,"fogColor"),0,0,0);
        glUniform1f(glGetUniformLocation(shader,"alpha"),1.0f);
        glUniform1i(glGetUniformLocation(shader,"useTexture"),0);

        // Body
        drawPart(shader, box, base, {0.06f,0.08f,0.45f}, {0.15f,0.15f,0.15f});
        // Barrel
        auto bar = glm::translate(base, {0,0.02f,-0.3f});
        drawPart(shader, box, bar, {0.03f,0.03f,0.3f}, {0.1f,0.1f,0.1f});
        // Handle
        auto hdl = glm::rotate(glm::translate(base,{0,-0.1f,0.1f}), glm::radians(15.0f),{1,0,0});
        drawPart(shader, box, hdl, {0.055f,0.14f,0.06f}, {0.2f,0.15f,0.1f});
    }

private:
    void drawPart(GLuint shader, const Mesh& box, glm::mat4 base, glm::vec3 scale, glm::vec3 col) {
        glm::mat4 m = glm::scale(base, scale);
        glUniformMatrix4fv(glGetUniformLocation(shader,"model"),1,GL_FALSE,glm::value_ptr(m));
        glUniform3fv(glGetUniformLocation(shader,"objectColor"),1,glm::value_ptr(col));
        box.draw();
    }
};
