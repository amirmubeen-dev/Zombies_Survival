#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <omp.h>
#include <iostream>
#include <string>
#include <cmath>
#include <ctime>
#include <algorithm>

// Core
#include "core/Window.h"
#include "core/Mesh.h"
#include "core/Texture.h"
#include "core/Shader.h"

// Renderer
#include "renderer/Camera.h"
#include "renderer/Renderer.h"
#include "renderer/Skybox.h"
#include "renderer/HUD.h"
#include "renderer/ParticleSystem.h"

// Entities
#include "entities/Player.h"
#include "entities/Zombie.h"
#include "entities/ZombieManager.h"

// Weapons
#include "weapons/WeaponManager.h"

// World
#include "world/Map.h"
#include "world/DayNight.h"
#include "world/WaveManager.h"
#include "world/CollisionSystem.h"

// Parallel
#include "parallel/OpenMPManager.h"
#include "parallel/MPIZoneManager.h"

// Audio
#include "audio/AudioManager.h"

enum GameState { STATE_MENU = 0, STATE_PLAYING = 1, STATE_GAMEOVER = 2 };

class Game {
public:
    Window   window;
    static const int SCR_W = 1280, SCR_H = 720;
    float deltaTime = 0, lastFrame = 0;
    float gameTime = 0;

    Camera   camera;
    Player   player;
    ZombieManager zombieMgr;
    WeaponManager weaponMgr;
    Map          map;
    DayNight     dayNight;
    WaveManager  waveMgr;
    CollisionSystem collision;
    ParticleSystem  particles;
    OpenMPManager   ompMgr;
    MPIZoneManager  mpiMgr;

    Renderer  renderer;
    Skybox    skybox;
    HUD       hud;
    Mesh      meshBox, meshGround, meshTrunk;

    bool keys[1024] = {};
    bool mouseCaptured = false;
    float lastMouseX = SCR_W / 2.0f;
    float lastMouseY = SCR_H / 2.0f;
    bool  firstMouse = true;
    bool  isADS = false;

    GameState gameState = STATE_MENU;
    static Game* instance;

    bool init() {
        srand((unsigned)time(nullptr));
        instance = this;

        if (!window.init(SCR_W, SCR_H, "3D Zombie Survival | PUBG-Style | OpenMP"))
            return false;

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glfwSetKeyCallback(window.handle, keyCB);
        glfwSetMouseButtonCallback(window.handle, mouseBtnCB);
        glfwSetCursorPosCallback(window.handle, mouseCB);
        glfwSetScrollCallback(window.handle, scrollCB);

        renderer.shaderMain  = renderer.buildMainShader();
        renderer.shaderColor = renderer.buildColorShader();

        meshBox    = Mesh::buildBox(1, 1, 1);
        meshGround = Mesh::buildGround(MAP_SIZE*2, MAP_SIZE / 2.0f);
        meshTrunk  = Mesh::buildCylinder(1, 1, 12);
        renderer.meshBox    = &meshBox;
        renderer.meshGround = &meshGround;
        renderer.meshTrunk  = &meshTrunk;

        Texture tex;
        renderer.texGrass    = tex.makeProceduralTexture(0);
        renderer.texDirt     = tex.makeProceduralTexture(1);
        renderer.texBark     = tex.makeProceduralTexture(2);
        renderer.texConcrete = tex.makeProceduralTexture(3);
        renderer.texSkin     = tex.makeProceduralTexture(4);

        skybox.init({{"","","","","",""}});
        hud.init(SCR_W, SCR_H, renderer.shaderColor);

        ompMgr.init(4);
        mpiMgr.init(MAP_SIZE, 4);
        weaponMgr.init();

        dayNight.timeOfDay = 0.25f;
        dayNight.daySpeed  = 0.0005f;

        AudioManager::get().init();

        std::cout << "[Game] Initialized. Press ENTER to start.\n";
        return true;
    }

    void startNewGame() {
        player.reset();
        camera.pos = player.pos + glm::vec3(0, 1.8f, 0) - camera.front * 2.5f;
        camera.yaw = -90.0f; camera.pitch = -15.0f;
        camera.updateDirection();

        zombieMgr.clear();
        particles.clear();
        weaponMgr.init();
        map.generate(400, 30); // 300x300 Map with props + medkits
        
        // Initial setup for safe zone
        player.pos = map.safeCenter;
        
        collision.clear();
        collision.mapRadius = MAP_SIZE;
        // Simplified collision for now
        
        waveMgr = WaveManager();
        waveMgr.startFirstWave();
        gameTime = 0.0f;
        
        AudioManager::get().playSound("ambience", player.pos, 1.0f, 0.4f); // background
        
        hud.killFeed.clear();

        mouseCaptured = true;
        firstMouse    = true;
        window.captureMouse(true);
        gameState = STATE_PLAYING;
    }

    void update() {
        float now = (float)glfwGetTime();
        deltaTime = std::min(now - lastFrame, 0.05f);
        lastFrame = now;

        if (gameState != STATE_PLAYING) return;
        gameTime += deltaTime;

        // DBNO & Healing Update
        player.update(deltaTime);
        if (!player.alive) {
            gameState = STATE_GAMEOVER;
            AudioManager::get().playSound("zombie_growl", player.pos);
            mouseCaptured = false;
            window.captureMouse(false);
            return;
        }

        // --- PUBG Movement ---
        glm::vec3 flatFront = camera.getFlatFront();
        glm::vec3 right     = camera.getRight();
        glm::vec3 moveDir   = {0,0,0};
        
        if (!player.isDown && !player.isHealing) {
            if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) moveDir += flatFront;
            if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) moveDir -= flatFront;
            if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) moveDir -= right;
            if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) moveDir += right;
        } else if (player.isDown) { // Crawl
            if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) moveDir += flatFront;
        }
        
        player.isSprinting = keys[GLFW_KEY_LEFT_SHIFT] && !player.isDown && !player.isHealing;
        player.isCrouching = keys[GLFW_KEY_LEFT_CONTROL] && !player.isDown && !player.isSprinting;

        // Momentum + Physics
        float targetSpeed = 0.0f;
        if (glm::length(moveDir) > 0.01f) {
            moveDir = glm::normalize(moveDir);
            targetSpeed = PLAYER_SPEED;
            if (player.isSprinting) targetSpeed *= 2.0f;
            if (player.isCrouching) targetSpeed *= 0.5f;
            if (player.isDown) targetSpeed *= 0.3f;
        }
        
        player.vel.x = glm::mix(player.vel.x, moveDir.x * targetSpeed, deltaTime * 8.0f);
        player.vel.z = glm::mix(player.vel.z, moveDir.z * targetSpeed, deltaTime * 8.0f);
        
        // Gravity & Jump
        if (!player.onGround) player.vel.y += GRAVITY * deltaTime;
        player.pos += player.vel * deltaTime;
        
        float floorY = map.getTerrainHeight(player.pos.x, player.pos.z);
        if (player.pos.y <= floorY) {
            player.pos.y = floorY;
            player.vel.y = 0;
            player.onGround = true;
        }
        
        // Footstep sounds
        static float footstepTimer = 0;
        if (player.onGround && targetSpeed > 0) {
            footstepTimer -= deltaTime;
            if (footstepTimer <= 0) {
                AudioManager::get().playSound("footstep", player.pos, player.isSprinting ? 1.2f : 0.8f, 0.5f);
                footstepTimer = player.isSprinting ? 0.3f : 0.5f;
            }
        }

        // Safe Zone Damage
        if (glm::distance(glm::vec2(player.pos.x, player.pos.z), glm::vec2(map.safeCenter.x, map.safeCenter.z)) > map.safeRadius) {
            static float zoneDmgTimer = 0;
            zoneDmgTimer += deltaTime;
            if (zoneDmgTimer >= 1.0f) {
                player.takeDamage(1.0f); // 1 DPS
                zoneDmgTimer = 0;
            }
        }

        // Item Pickups
        if (keys[GLFW_KEY_E] && !player.isDown && !player.isHealing) {
            for (size_t i = 0; i < map.items.size(); i++) {
                if (glm::distance(player.pos, map.items[i].pos) < 2.5f) {
                    if (map.items[i].type == 1) player.bandages++;
                    else if (map.items[i].type == 2) player.medkits++;
                    else if (map.items[i].type == 3) player.armor = player.maxArmor;
                    map.items.erase(map.items.begin() + i);
                    break;
                }
            }
        }
        
        // Use heals
        if (keys[GLFW_KEY_Q]) player.startHealing(1); // Bandage
        if (keys[GLFW_KEY_H]) player.startHealing(2); // Medkit

        // Auto-shoot loop
        if (keys[GLFW_KEY_F] && !player.isDown && !player.isHealing) shoot();

        // --- PUBG Third-Person Camera ---
        float camHeight = player.isDown ? 0.3f : (player.isCrouching ? 1.2f : 1.8f);
        glm::vec3 headPos = player.pos + glm::vec3(0, camHeight, 0);
        
        float dist = isADS ? 0.8f : 2.5f;
        camera.targetFov = isADS ? 45.0f : 70.0f;
        camera.targetPos = headPos - camera.front * dist;
        
        // Simple camera collision (pull forward if below terrain)
        if (camera.targetPos.y < map.getTerrainHeight(camera.targetPos.x, camera.targetPos.z) + 0.2f) {
            camera.targetPos.y = map.getTerrainHeight(camera.targetPos.x, camera.targetPos.z) + 0.2f;
        }
        
        camera.update(deltaTime);

        // Subsystems
        map.update(deltaTime);
        particles.update(deltaTime);
        weaponMgr.updateBullets(deltaTime, zombieMgr.zombies, particles, map.safeRadius * 1.5f);
        dayNight.update(deltaTime);
        hud.update(deltaTime);

        // Zombie AI Update
        zombieMgr.updateAll(deltaTime, player.pos, player.health, camera.shakeIntensity, player.alive, (int&)gameState);
        
        // Check for deaths and play sounds + kill feed
        static int lastAlive = 0;
        int nowAlive = 0;
        for (auto& z : zombieMgr.zombies) {
            if (!z.alive && !z.isDying) { // Just died frame
                z.isDying = true;
                z.deathTimer = 0.0f; // Start death animation
                AudioManager::get().playSound("zombie_growl", z.pos, 0.6f, 1.5f); // death gurgle
                
                std::string typeNames[] = {"WALKER", "RUNNER", "TANK"};
                hud.addKillMsg("+10 pts  " + typeNames[z.type] + " KILLED");
                waveMgr.zombiesKilled++;
            }
            if (!z.isDying) nowAlive++;
            else {
                z.updateAnimation(deltaTime, false);
            }
        }
        lastAlive = nowAlive;
        player.score = waveMgr.zombiesKilled * 10;
        
        waveMgr.update(deltaTime, zombieMgr);
        
        AudioManager::get().setListenerPos(camera.pos, camera.front, camera.up);
        mpiMgr.reset(); // simulation visual
    }

    void shoot() {
        if (!weaponMgr.getActive().canShoot()) {
            if (weaponMgr.getActive().ammo <= 0) AudioManager::get().playSound("empty", player.pos, 1.0f, 0.2f);
            return;
        }
        
        glm::vec3 muzzle = camera.pos + camera.front * 1.2f + camera.getRight() * 0.3f;
        particles.spawnSparks(muzzle, camera.front, 12);
        
        weaponMgr.shoot(camera, muzzle, camera.shakeIntensity);
        player.gunRecoil = 0.3f;
        
        std::string sfx = (weaponMgr.currentWeapon==0?"pistol":(weaponMgr.currentWeapon==1?"shotgun":"rifle"));
        AudioManager::get().playSound(sfx, player.pos, 1.0f + ((rand()%20)-10)*0.01f);
    }

    void render() {
        if (gameState == STATE_MENU || gameState == STATE_GAMEOVER) {
            renderMenu();
            return;
        }

        glClearColor(dayNight.getSkyBottomColor().r, dayNight.getSkyBottomColor().g, dayNight.getSkyBottomColor().b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getView();
        glm::mat4 proj = camera.getProj((float)SCR_W / SCR_H);
        skybox.draw(view, proj, dayNight.getSkyTopColor(), dayNight.getSkyBottomColor());

        renderer.setWorldUniforms(camera, SCR_W, SCR_H, dayNight, camera.shakeIntensity);
        glUseProgram(renderer.shaderMain);

        renderer.drawGround(map);
        renderer.drawProps(map);
        renderer.drawItems(map, gameTime);
        renderer.drawSafeZone(map);

        for (auto& z : zombieMgr.zombies) renderer.drawZombie(z, gameTime);
        renderer.drawBullets(weaponMgr.activeBullets);
        renderer.drawParticles(particles);

        if (!player.isDown && !player.isHealing) {
            glClear(GL_DEPTH_BUFFER_BIT);
             weaponMgr.drawViewModel(renderer.shaderMain, meshBox, player.gunRecoil, (float)SCR_W / SCR_H);
        }

        // PUBG HUD
        hud.beginHUD();
        hud.drawHealthArmor(player.health, player.maxHealth, player.armor, player.maxArmor, player.isDown, player.bleedOutTimer);
        hud.drawWeaponInfo(weaponMgr.getAmmo(), weaponMgr.getReserve(), weaponMgr.getName(), 
                           weaponMgr.isReloading(), weaponMgr.getReloadT());
        
        if (!player.isDown) hud.drawCrosshair(glm::length(player.vel), isADS, false);
        
        // Minimap
        std::vector<glm::vec2> zPos;
        for (const auto& z : zombieMgr.zombies) {
            if (z.alive && !z.isDying && glm::distance(z.pos, player.pos) < 50.0f) 
                zPos.push_back({z.pos.x, z.pos.z});
        }
        hud.drawMinimap({player.pos.x, player.pos.z}, {camera.getFlatFront().x, camera.getFlatFront().z}, zPos);
        
        hud.drawTopBar(waveMgr.currentWave, player.score, (int)zombieMgr.zombies.size());
        hud.drawKillFeed();
        
        if (player.lastDamageTime < 0.2f && !player.isDown) {
            hud.drawDamageVignette(1.0f - (player.lastDamageTime / 0.2f));
        }

        hud.drawParallelStats(ompMgr.activeThreads, mpiMgr.getHotZones(), mpiMgr.TOTAL_ZONES, (int)zombieMgr.zombies.size());
        hud.endHUD();
    }

    void renderMenu() {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND);
        
        glUseProgram(renderer.shaderColor);
        glm::mat4 ortho = glm::ortho(0.0f, (float)SCR_W, (float)SCR_H, 0.0f);
        auto rect2d = [&](float x, float y, float w, float h, float r, float g2, float b, float a = 1.0f) {
            if (w <= 0 || h <= 0) return;
            float v[] = {x,y,0, x+w,y,0, x+w,y+h,0, x,y+h,0};
            unsigned idx[] = {0u,1u,2u,0u,2u,3u};
            GLuint va,vb,eb; glGenVertexArrays(1,&va); glGenBuffers(1,&vb); glGenBuffers(1,&eb);
            glBindVertexArray(va); glBindBuffer(GL_ARRAY_BUFFER,vb); glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,eb); glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,(void*)0); glEnableVertexAttribArray(0);
            glUniformMatrix4fv(glGetUniformLocation(renderer.shaderColor,"mvp"),1,GL_FALSE,glm::value_ptr(ortho));
            glUniform4f(glGetUniformLocation(renderer.shaderColor,"color"),r,g2,b,a);
            glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
            glDeleteVertexArrays(1,&va); glDeleteBuffers(1,&vb); glDeleteBuffers(1,&eb);
        };

        rect2d(0, 0, SCR_W, SCR_H, 0.05f, 0.05f, 0.05f, 1.0f);
        float cy = SCR_H * 0.3f;
        rect2d(SCR_W/2.0f-360, cy-50, 720, 90, 0.9f, 0.6f, 0.0f, 0.95f); // PUBG yellow
        rect2d(SCR_W/2.0f-358, cy-48, 716, 86, 0.1f, 0.1f, 0.1f, 0.98f);
        rect2d(SCR_W/2.0f-358, cy+44, 716, 4, 1.0f, 1.0f, 1.0f, 0.8f);
        
        float pulse = 0.55f + 0.45f * sinf((float)glfwGetTime() * 2.5f);
        rect2d(SCR_W/2.0f-200, cy+250, 400, 56, 1.0f, 0.6f, 0.0f, pulse * 0.9f);
        
        if (gameState == STATE_GAMEOVER) {
            rect2d(SCR_W/2.0f-200, cy+180, 400, 50, 0.8f, 0.1f, 0.1f, 0.85f);
        }
        glEnable(GL_DEPTH_TEST);
    }

    void run() {
        while (!window.shouldClose()) {
            window.pollEvents();
            update();
            render();
            window.swapBuffers();
        }
    }

    // Input callbacks
    static void keyCB(GLFWwindow*, int key, int, int action, int) {
        if (!instance) return;
        Game& g = *instance;
        if (action == GLFW_PRESS)   g.keys[key] = true;
        if (action == GLFW_RELEASE) g.keys[key] = false;

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            if (g.gameState == STATE_PLAYING) {
                g.mouseCaptured = false;
                g.window.captureMouse(false);
            } else glfwSetWindowShouldClose(g.window.handle, true);
        }
        if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
            if (g.gameState == STATE_MENU || g.gameState == STATE_GAMEOVER) g.startNewGame();
        }
        if (action == GLFW_PRESS && !g.player.isDown) {
            if (key == GLFW_KEY_SPACE && g.player.onGround) {
                g.player.vel.y = 8.0f; // Jump
                g.player.onGround = false;
            }
            if (key == GLFW_KEY_1) g.weaponMgr.switchWeapon(0);
            if (key == GLFW_KEY_2) g.weaponMgr.switchWeapon(1);
            if (key == GLFW_KEY_3) g.weaponMgr.switchWeapon(2);
            if (key == GLFW_KEY_R) g.weaponMgr.reload();
        }
    }

    static void mouseBtnCB(GLFWwindow*, int button, int action, int) {
        if (!instance) return;
        Game& g = *instance;
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            if (g.gameState == STATE_PLAYING && !g.player.isDown && !g.player.isHealing) g.shoot();
            else if (g.gameState != STATE_PLAYING) {
                g.mouseCaptured = true; g.firstMouse = true; g.window.captureMouse(true);
            }
        }
        // ADS 
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) g.isADS = true;
            else if (action == GLFW_RELEASE) g.isADS = false;
        }
    }

    static void mouseCB(GLFWwindow*, double xpos, double ypos) {
        if (!instance || !instance->mouseCaptured) return;
        Game& g = *instance;
        if (g.firstMouse) { g.lastMouseX = (float)xpos; g.lastMouseY = (float)ypos; g.firstMouse = false; }
        float dx = (float)xpos - g.lastMouseX;
        float dy = g.lastMouseY - (float)ypos;
        g.lastMouseX = (float)xpos; g.lastMouseY = (float)ypos;
        g.camera.applyMouseLook(dx, dy);
    }

    static void scrollCB(GLFWwindow*, double, double yoff) {
        if (instance && instance->gameState == STATE_PLAYING && !instance->player.isDown)
            instance->weaponMgr.scrollWeapon((int)yoff);
    }
};

Game* Game::instance = nullptr;
