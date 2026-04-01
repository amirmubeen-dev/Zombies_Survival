#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct KillMsg {
    std::string text;
    float timer = 4.0f;
};

class HUD {
public:
    int   scrW = 1280, scrH = 720;
    GLuint colorShader = 0;
    
    std::vector<KillMsg> killFeed;

    void init(int w, int h, GLuint colorSh) {
        scrW = w; scrH = h; colorShader = colorSh;
    }

    void beginHUD() {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(colorShader);
    }

    void endHUD() { glEnable(GL_DEPTH_TEST); }

    void addKillMsg(const std::string& msg) {
        killFeed.insert(killFeed.begin(), {msg, 4.0f});
        if (killFeed.size() > 5) killFeed.pop_back();
    }

    void update(float dt) {
        for (auto& k : killFeed) k.timer -= dt;
        killFeed.erase(std::remove_if(killFeed.begin(), killFeed.end(), 
            [](const KillMsg& k){ return k.timer <= 0; }), killFeed.end());
    }

    // PUBG Bottom-Left: Health & Armor
    void drawHealthArmor(float hp, float maxHp, float armor, float maxArmor, bool isDown, float bleedTimer) {
        float x = 20, y = scrH - 60, w = 220, h = 30;
        
        // Background
        rect(x-4, y-20, w+8, h+28, 0, 0, 0, 0.5f);
        
        // Armor bar (blue) above health
        float aRatio = maxArmor > 0 ? std::max(0.0f, std::min(1.0f, armor / maxArmor)) : 0;
        rect(x, y-14, w, 8, 0.1f, 0.1f, 0.2f, 0.8f);
        if (aRatio > 0) rect(x, y-14, w * aRatio, 8, 0.2f, 0.6f, 1.0f, 0.9f);
        
        // Health bar
        float hRatio = maxHp > 0 ? std::max(0.0f, std::min(1.0f, hp / maxHp)) : 0;
        float r = 0.9f, g = 0.9f, b = 0.9f;
        if (isDown) { r = 1.0f; g = 0.2f; b = 0.2f; hRatio = bleedTimer / 45.0f; }
        else if (hRatio < 0.3f) { r = 0.9f; g = 0.1f; b = 0.1f; }
        
        rect(x, y, w, h, 0.2f, 0.2f, 0.2f, 0.8f);
        rect(x, y, w * hRatio, h, r, g, b, 0.9f);
        
        // DBNO Pulse effect
        if (isDown) {
            float pulse = 0.5f + 0.5f * sinf(bleedTimer * 8.0f);
            rect(x, y, w, h, 1.0f, 0.0f, 0.0f, pulse * 0.4f);
        }
    }

    // PUBG Bottom-Right: Weapon & Ammo
    void drawWeaponInfo(int ammo, int reserve, const std::string& name, bool reloading, float relRatio) {
        float bw = 200, bh = 40;
        float bx = scrW - bw - 20, by = scrH - 60;
        
        rect(bx-4, by-4, bw+8, bh+8, 0, 0, 0, 0.5f);
        
        // Gun Name Box
        rect(bx, by, bw*0.4f, bh, 0.2f, 0.2f, 0.2f, 0.7f);
        
        // Ammo Box
        rect(bx + bw*0.42f, by, bw*0.58f, bh, 0.1f, 0.1f, 0.1f, 0.7f);
        
        if (reloading) {
            rect(bx, by - 12, bw, 6, 0.2f, 0.2f, 0.2f, 0.8f);
            rect(bx, by - 12, bw * relRatio, 6, 1.0f, 0.7f, 0.2f, 0.9f);
        }
    }

    // Dynamic Crosshair
    void drawCrosshair(float spread, bool isADS, bool targetZomb) {
        if (isADS) {
            // Red dot
            rect(scrW/2.0f - 2, scrH/2.0f - 2, 4, 4, 1.0f, 0.0f, 0.0f, 0.9f);
            return;
        }
        
        glm::mat4 id = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(colorShader, "mvp"), 1, GL_FALSE, glm::value_ptr(id));
        
        float s = 0.01f + spread * 0.015f; // gap
        float l = 0.015f; // length
        float t = 0.003f; // thickness
        
        float r = targetZomb ? 1.0f : 1.0f;
        float g2= targetZomb ? 0.0f : 1.0f;
        float b = targetZomb ? 0.0f : 1.0f;
        glUniform4f(glGetUniformLocation(colorShader, "color"), r, g2, b, 0.8f);
        
        float verts[] = {
            -s-l, -t, 0,   -s, -t, 0,   -s, t, 0,   -s-l, t, 0, // Left
             s, -t, 0,      s+l, -t, 0,  s+l, t, 0,  s, t, 0,   // Right
            -t, -s-l, 0,    t, -s-l, 0,  t, -s, 0,  -t, -s, 0,  // Top
            -t, s, 0,       t, s, 0,     t, s+l, 0, -t, s+l, 0  // Bottom
        };
        unsigned int idx[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 8,9,10,8,10,11, 12,13,14,12,14,15};
        
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, (void*)0); glEnableVertexAttribArray(0);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
        glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo);
    }
    
    // Top-Right Minimap (150x150)
    void drawMinimap(glm::vec2 pPos, glm::vec2 pDir, const std::vector<glm::vec2>& zombies) {
        float size = 150.0f;
        float x = scrW - size - 20, y = 20;
        
        // Map Background
        rect(x-2, y-2, size+4, size+4, 0.05f, 0.08f, 0.05f, 0.8f);
        
        // Player (Center)
        float cx = x + size/2;
        float cy = y + size/2;
        float pSize = 4.0f;
        rect(cx - pSize/2, cy - pSize/2, pSize, pSize, 1.0f, 1.0f, 1.0f, 1.0f);
        
        // View cone logic
        float coneLen = 15.0f;
        rect(cx + pDir.x*coneLen - 1, cy + pDir.y*coneLen - 1, 3, 3, 1.0f, 1.0f, 0.0f, 0.6f);

        // Zombies (Relative to player, scale factor e.g., 1 map unit = 2 pixels)
        float scale = 2.0f;
        for (const auto& z : zombies) {
            float dx = (z.x - pPos.x) * scale;
            float dy = (z.y - pPos.y) * scale; // z in world is map y
            if (abs(dx) < size/2 && abs(dy) < size/2) {
                rect(cx + dx - 1.5f, cy + dy - 1.5f, 3.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.9f);
            }
        }
    }

    void drawKillFeed() {
        float x = scrW - 250;
        float y = 190; // Below minimap
        for (size_t i = 0; i < killFeed.size(); i++) {
            float alpha = std::min(killFeed[i].timer, 1.0f);
            rect(x, y + i * 25, 230, 20, 0.1f, 0.1f, 0.1f, alpha * 0.6f);
            // Red hit marker proxy
            rect(x + 5, y + i * 25 + 5, 10, 10, 1.0f, 0.2f, 0.0f, alpha * 0.9f);
        }
    }

    void drawTopBar(int wave, int score, int aliveZombies, float safeRadius, float nextSafeRadius, bool zoneShrinking) {
        float x = scrW * 0.15f;
        float w = scrW * 0.7f;
        float y = 10;
        float h = 22;

        // base top-left panel
        rect(x - 4, y - 4, w + 8, h + 32, 0, 0, 0, 0.55f);
        rect(x, y, w, h + 24, 0.1f, 0.1f, 0.15f, 0.85f);

        // wave/score/targets bars
        float barY = y + 6;
        float gap = 8;
        float barW = (w - gap * 2) / 3.0f;

        float waveProg = std::min(1.0f, (float)wave / 20.0f);
        float aliveProg = std::min(1.0f, (float)aliveZombies / 100.0f);
        float scoreProg = std::min(1.0f, (float)score / 2000.0f);

        rect(x + 2, barY, barW, 8, 0.08f, 0.08f, 0.1f, 0.6f);
        rect(x + 2, barY, barW * waveProg, 8, 0.8f, 0.2f, 0.2f, 0.9f);
        rect(x + 2 + barW + gap, barY, barW, 8, 0.08f, 0.08f, 0.1f, 0.6f);
        rect(x + 2 + barW + gap, barY, barW * scoreProg, 8, 0.2f, 0.8f, 0.2f, 0.9f);
        rect(x + 2 + (barW + gap) * 2, barY, barW, 8, 0.08f, 0.08f, 0.1f, 0.6f);
        rect(x + 2 + (barW + gap) * 2, barY, barW * aliveProg, 8, 0.8f, 0.6f, 0.1f, 0.9f);

        // zone status bar
        float zonePerc = (nextSafeRadius > 0) ? std::min(1.0f, safeRadius / (nextSafeRadius*1.5f)) : 0.0f;
        float zoneColorR = zoneShrinking ? 1.0f : 0.3f;
        float zoneColorG = zoneShrinking ? 0.2f : 0.8f;
        rect(x + 2, barY + 14, w - 4, 6, 0.05f, 0.05f, 0.05f, 0.6f);
        rect(x + 2, barY + 14, (w - 4) * zonePerc, 6, zoneColorR, zoneColorG, 0.1f, 0.85f);

        // small dot markers for UI feel
        for (int i = 0; i < 5; i++) {
            float px = x + 6 + i * ((w - 20) / 4);
            rect(px, barY + 24, 4, 4, 0.9f, 0.9f, 0.9f, 0.5f);
        }
    }
    
    // Directional Damage Vignette
    void drawDamageVignette(float intensity) {
        if (intensity <= 0.01f) return;
        float alpha = std::min(intensity * 0.6f, 0.6f);
        float th = scrH * 0.15f;
        float tw = scrW * 0.15f;
        rect(0, 0, (float)scrW, th, 1.0f, 0, 0, alpha);               
        rect(0, scrH - th, (float)scrW, th, 1.0f, 0, 0, alpha);       
        rect(0, 0, tw, (float)scrH, 1.0f, 0, 0, alpha);                
        rect(scrW - tw, 0, tw, (float)scrH, 1.0f, 0, 0, alpha);        
    }

    void drawParallelStats(int ompThreads, int hotZones, int totalZones, int entities) {
        float y = scrH - 18.0f;
        rect(0, y, (float)scrW, 18.0f, 0, 0, 0, 0.8f);
        
        float eRatio = std::min((float)entities / 200.0f, 1.0f);
        float zRatio = totalZones > 0 ? (float)hotZones / totalZones : 0.0f;
        float oRatio = 4.0f / 8.0f; // Mock 4 threads
        
        rect(10, y + 4, 80 * oRatio, 10, 0, 0.7f, 0.2f, 0.8f);
        rect(100, y + 4, 80 * zRatio, 10, 0.9f, 0.4f, 0.1f, 0.8f);
        rect(190, y + 4, 80 * eRatio, 10, 0.3f, 0.4f, 0.9f, 0.8f);
    }

private:
    void rect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
        if (w <= 0 || h <= 0) return;
        glm::mat4 ortho = glm::ortho(0.0f, (float)scrW, (float)scrH, 0.0f);
        float verts[] = { x,y,0, x+w,y,0, x+w,y+h,0, x,y+h,0 };
        unsigned int idx[] = {0,1,2,0,2,3};
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, (void*)0); glEnableVertexAttribArray(0);
        glUniformMatrix4fv(glGetUniformLocation(colorShader, "mvp"), 1, GL_FALSE, glm::value_ptr(ortho));
        glUniform4f(glGetUniformLocation(colorShader, "color"), r, g, b, a);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo);
    }
};
