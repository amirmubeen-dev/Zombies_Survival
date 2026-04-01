#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../core/Mesh.h"
#include "../renderer/Camera.h"
#include "../renderer/ParticleSystem.h"
#include "../entities/Zombie.h"
#include "../world/Map.h"
#include "../world/DayNight.h"
#include "../weapons/WeaponManager.h"
#include <vector>
#include <cmath>

// Shader uniform locations are cached per-draw via glGetUniformLocation

class Renderer {
public:
    GLuint shaderMain  = 0;
    GLuint shaderColor = 0;

    // Meshes (owned by Game, referenced here)
    Mesh* meshBox    = nullptr;
    Mesh* meshGround = nullptr;
    Mesh* meshTrunk  = nullptr;

    // Textures
    GLuint texGrass = 0, texDirt = 0, texBark  = 0;
    GLuint texSkin  = 0, texConcrete = 0;

    // Shadow map (2048×2048 depth texture)
    GLuint shadowFBO = 0, shadowDepthTex = 0;
    static const int SHADOW_RES = 2048;
    glm::mat4 lightSpaceMatrix;

    void initShadowMap() {
        glGenFramebuffers(1, &shadowFBO);
        glGenTextures(1, &shadowDepthTex);
        glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_RES, SHADOW_RES,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = {1,1,1,1};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Set all standard world uniforms
    void setWorldUniforms(const Camera& cam, int scrW, int scrH,
                          const DayNight& dn, float shakeAmount) const
    {
        glm::vec3 shake = {0,0,0};
        if (shakeAmount > 0.01f) {
            shake.x = ((float)rand()/RAND_MAX - 0.5f) * shakeAmount * 0.3f;
            shake.y = ((float)rand()/RAND_MAX - 0.5f) * shakeAmount * 0.15f;
        }
        Camera shakyCam = cam;
        shakyCam.pos += shake;

        glm::mat4 view = shakyCam.getView();
        glm::mat4 proj = shakyCam.getProj((float)scrW / scrH);

        glUseProgram(shaderMain);
        glUniformMatrix4fv(glGetUniformLocation(shaderMain,"view"),      1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderMain,"projection"),1,GL_FALSE,glm::value_ptr(proj));

        // Day/night lighting
        glUniform3fv(glGetUniformLocation(shaderMain,"lightPos"),   1,glm::value_ptr(dn.getSunDirection()*50.0f));
        glUniform3fv(glGetUniformLocation(shaderMain,"lightColor"), 1,glm::value_ptr(dn.getSunColor()));
        glUniform1f(glGetUniformLocation(shaderMain,"ambientStrength"), dn.getAmbientStrength());
        glUniform3fv(glGetUniformLocation(shaderMain,"fogColor"),   1,glm::value_ptr(dn.getFogColor()));
        glUniform1f(glGetUniformLocation(shaderMain,"fogDensity"),  dn.getFogDensity());
        glUniform1f(glGetUniformLocation(shaderMain,"nightFactor"), dn.getNightFactor());
        glUniform1f(glGetUniformLocation(shaderMain,"alpha"),       1.0f);
        glUniform1i(glGetUniformLocation(shaderMain,"useTexture"),  0);
    }

    void drawMesh(const Mesh& mesh, glm::mat4 model, glm::vec3 color,
                  GLuint tex = 0, float alpha = 1.0f, bool flash = false) const
    {
        glm::vec3 col = flash ? glm::vec3(1,1,1) : color;
        glm::mat4 id = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderMain,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderMain,"objectColor"),1,glm::value_ptr(col));
        glUniform1f(glGetUniformLocation(shaderMain,"alpha"), alpha);

        if (tex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(glGetUniformLocation(shaderMain,"tex"), 0);
            glUniform1i(glGetUniformLocation(shaderMain,"useTexture"), 1);
        } else {
            glUniform1i(glGetUniformLocation(shaderMain,"useTexture"), 0);
        }
        mesh.draw();
    }

    void drawGround(const Map& map) {
        glm::mat4 m = glm::mat4(1.0f);
        drawMesh(*meshGround, m, {0.3f,0.5f,0.2f}, texGrass);
    }

    void drawProps(const Map& map) {
        for (const auto& p : map.props) {
            glm::mat4 m = glm::translate(glm::mat4(1), p.pos);
            m = glm::rotate(m, glm::radians(p.rotation), {0,1,0});

            if (p.type == PROP_TREE) {
                // Trunk
                drawMesh(*meshTrunk, glm::scale(m, {p.size.x * 5, p.size.y, p.size.x * 5}), {0.4f,0.25f,0.1f}, texBark);
                // Leaves (3 layers)
                for (int layer = 0; layer < 3; layer++) {
                    float ls = p.size.z * (1.1f - layer * 0.25f);
                    glm::mat4 leafM = glm::translate(m, glm::vec3(0, p.size.y + layer * ls * 0.6f, 0));
                    drawMesh(*meshBox, glm::scale(leafM, {ls, ls * 0.5f, ls}), {0.1f + layer*0.03f, 0.35f + layer*0.05f, 0.05f}, texGrass);
                }
            } else if (p.type == PROP_DEAD_TREE) {
                drawMesh(*meshTrunk, glm::scale(m, {p.size.x * 5, p.size.y, p.size.x * 5}), {0.2f,0.2f,0.2f}, texBark);
            } else if (p.type == PROP_CRATE) {
                m = glm::translate(m, {0, p.size.y, 0});
                drawMesh(*meshBox, glm::scale(m, p.size), {0.5f,0.3f,0.1f});
            } else if (p.type == PROP_RUIN) {
                m = glm::translate(m, {0, p.size.y, 0});
                drawMesh(*meshBox, glm::scale(m, p.size), {0.45f, 0.42f, 0.38f}, texConcrete);
            } else if (p.type == PROP_CAR) {
                m = glm::translate(m, {0, p.size.y, 0});
                drawMesh(*meshBox, glm::scale(m, p.size), {0.3f, 0.2f, 0.15f}); // Rusty
            } else if (p.type == PROP_CAMPFIRE) {
                m = glm::translate(m, {0, p.size.y, 0});
                drawMesh(*meshBox, glm::scale(m, p.size), {0.2f, 0.2f, 0.2f});
            }
        }
    }

    void drawItems(const Map& map, float time) {
        for (const auto& it : map.items) {
            float hover = sinf(time * 3.0f + it.pos.x) * 0.1f;
            glm::mat4 m = glm::translate(glm::mat4(1), it.pos + glm::vec3(0, 0.3f + hover, 0));
            m = glm::rotate(m, time * 2.0f, {0,1,0});
            
            if (it.type == 1) { // Bandage
                drawMesh(*meshBox, glm::scale(m, {0.15f, 0.1f, 0.15f}), {0.9f, 0.9f, 0.9f});
            } else if (it.type == 2) { // Medkit (Glowing Green)
                drawMesh(*meshBox, glm::scale(m, {0.3f, 0.2f, 0.3f}), {0.2f, 1.0f, 0.2f}, 0, 1.0f, true);
            } else if (it.type == 3) { // Armor (Blue vest)
                drawMesh(*meshBox, glm::scale(m, {0.25f, 0.3f, 0.1f}), {0.1f, 0.3f, 0.9f});
            }
        }
    }

    void drawSafeZone(const Map& map) {
        // Draw 36 segments of a thin box for the circle
        const float PI = 3.14159265f;
        int segments = 36;
        for (int i = 0; i < segments; i++) {
            float ang = i * 2 * PI / segments;
            glm::vec3 pos = map.safeCenter + glm::vec3(cosf(ang) * map.safeRadius, 1.0f, sinf(ang) * map.safeRadius);
            glm::mat4 m = glm::translate(glm::mat4(1), pos);
            m = glm::rotate(m, -ang, {0,1,0});
            m = glm::scale(m, {0.5f, 5.0f, (map.safeRadius*2*PI)/segments * 0.5f});
            drawMesh(*meshBox, m, {1.0f, 1.0f, 1.0f}, 0, 0.3f, true); // White, translucent, emissive tube
        }
    }

    void drawZombie(const Zombie& z, float time) {
        if (!z.alive && z.isDying && z.deathTimer >= 2.8f) return; // fully faded

        glm::vec3 bc = z.getBaseColor();
        bool flash   = z.flashing;
        float scale  = z.getScale();
        float alpha  = 1.0f;

        // Death fall animation
        float dropRotX = 0.0f;
        if (z.isDying) {
            dropRotX = std::min(90.0f, z.deathTimer * 112.5f); // 90 deg in 0.8s
            if (z.deathTimer > 0.8f) {
                alpha = std::max(0.0f, 1.0f - (z.deathTimer - 0.8f) * 0.5f); // fade over 2s
            }
        }

        auto T = glm::translate(glm::mat4(1), z.pos + glm::vec3(0, 0.9f*scale, 0));
        T = glm::rotate(T, glm::radians(z.angle), {0,1,0});
        if (dropRotX > 0) T = glm::rotate(T, glm::radians(dropRotX), {1,0,0});
        T = glm::scale(T, {scale,scale,scale});

        // Torso (hunched forward)
        auto Torso = glm::rotate(T, glm::radians(15.0f), {1,0,0});
        drawMesh(*meshBox, glm::scale(Torso, {0.4f,0.7f,0.25f}), bc * 0.8f, texSkin, alpha, flash);

        // Head
        auto H = glm::translate(Torso, {0, 0.85f, 0.1f});
        if (!z.isDying) {
            float headBob = sinf(time * z.speed * 2.0f) * 0.05f;
            H = glm::translate(H, {0, headBob, 0});
        }
        drawMesh(*meshBox, glm::scale(H, {0.3f,0.3f,0.3f}), bc, texSkin, alpha, flash);
        
        // Glowing Eyes (only if not completely dead)
        if (!z.isDying || z.deathTimer < 1.0f) {
            auto EyeL = glm::translate(H, {-0.12f, 0.05f, 0.31f});
            auto EyeR = glm::translate(H, { 0.12f, 0.05f, 0.31f});
            drawMesh(*meshBox, glm::scale(EyeL, {0.04f,0.04f,0.02f}), {1.0f,0,0}, 0, alpha, true); // Flash=true makes it ignore lighting
            drawMesh(*meshBox, glm::scale(EyeR, {0.04f,0.04f,0.02f}), {1.0f,0,0}, 0, alpha, true);
        }

        // Arms (outstretched)
        float armSwing = z.isDying ? 0 : sinf(z.walkAnim) * 0.2f;
        auto LA = glm::rotate(glm::translate(Torso, {-0.5f, 0.4f, 0.3f}), glm::radians(80.0f) + armSwing, {1,0,0});
        drawMesh(*meshBox, glm::scale(LA, {0.12f,0.6f,0.12f}), bc, texSkin, alpha, flash);
        
        auto RA = glm::rotate(glm::translate(Torso, { 0.5f, 0.4f, 0.3f}), glm::radians(80.0f) - armSwing, {1,0,0});
        drawMesh(*meshBox, glm::scale(RA, {0.12f,0.6f,0.12f}), bc, texSkin, alpha, flash);

        // Legs
        float legSwing = z.isDying ? 0 : sinf(z.walkAnim) * 0.6f;
        auto LL = glm::rotate(glm::translate(T, {-0.2f, -0.6f, 0}), -legSwing, {1,0,0});
        drawMesh(*meshBox, glm::scale(LL, {0.18f,0.6f,0.18f}), bc * 0.6f, texSkin, alpha, flash);
        
        // Zombie drag: one leg injured
        auto RL = glm::rotate(glm::translate(T, { 0.2f, -0.6f, -0.1f}), legSwing * 0.5f, {1,0,0});
        drawMesh(*meshBox, glm::scale(RL, {0.18f,0.6f,0.18f}), bc * 0.6f, texSkin, alpha, flash);
    }
    
    void drawBullets(const std::vector<Bullet>& bullets) {
        for (const auto& b : bullets) {
            glm::mat4 M = glm::translate(glm::mat4(1), b.pos);
            glm::vec3 col = (b.weaponType == 1) ? glm::vec3(1.0f, 0.2f, 0.0f) : glm::vec3(1.0f, 0.9f, 0.2f);
            
            if (b.weaponType == 2) { // Rifle capsule
                glm::mat4 rot = glm::mat4(1.0f);
                if (glm::length(b.vel) > 0.1f) {
                    glm::vec3 f = glm::normalize(b.vel);
                    glm::vec3 up = glm::vec3(0,1,0); 
                    if (abs(f.y) > 0.99f) up = glm::vec3(1,0,0);
                    glm::vec3 r = glm::normalize(glm::cross(up, f));
                    glm::vec3 u = glm::cross(f, r);
                    rot = glm::mat4(glm::vec4(r,0), glm::vec4(u,0), glm::vec4(f,0), glm::vec4(0,0,0,1));
                }
                M = M * rot * glm::scale(glm::mat4(1), {0.03f, 0.03f, 0.15f});
            } else { // Sphere proxy
                float r = (b.weaponType == 1) ? 0.02f : 0.04f;
                M = glm::scale(M, {r,r,r});
            }
            // Draw bullet true unlit
            drawMesh(*meshBox, M, col, 0, 1.0f, true);
        }
    }

    void drawParticles(const ParticleSystem& ps) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (auto& p : ps.particles) {
            float t = p.life / p.maxLife;
            glm::mat4 m = glm::scale(glm::translate(glm::mat4(1), p.pos), {p.size, p.size, p.size});
            drawMesh(*meshBox, m, p.color, 0, t * 0.9f);
        }
        glDisable(GL_BLEND);
    }

    GLuint buildColorShader() {
        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 mvp;
uniform vec4 color;
out vec4 vColor;
void main() { gl_Position = mvp * vec4(aPos,1.0); vColor = color; }
)";
        const char* fs = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
)";
        auto compile = [](GLenum type, const char* src) {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            return s;
        };
        GLuint v = compile(GL_VERTEX_SHADER, vs);
        GLuint f = compile(GL_FRAGMENT_SHADER, fs);
        GLuint p = glCreateProgram();
        glAttachShader(p,v); glAttachShader(p,f);
        glLinkProgram(p);
        glDeleteShader(v); glDeleteShader(f);
        return p;
    }

    GLuint buildMainShader() {
        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float Dist;
uniform mat4 model, view, projection;
void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos  = worldPos.xyz;
    Normal   = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    vec4 viewPos = view * worldPos;
    Dist = length(viewPos.xyz);
    gl_Position = projection * viewPos;
}
)";
        const char* fs = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float Dist;
out vec4 FragColor;
uniform vec3  objectColor;
uniform vec3  lightPos;
uniform vec3  lightColor;
uniform float ambientStrength;
uniform bool  useTexture;
uniform sampler2D tex;
uniform vec3  fogColor;
uniform float fogDensity;
uniform float alpha;
uniform float nightFactor;
void main() {
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 ambient  = ambientStrength * lightColor;
    vec3 diffuse  = diff * lightColor;
    vec3 base     = useTexture ? texture(tex, TexCoord).rgb : objectColor;
    vec3 result   = (ambient + diffuse) * base;
    // Night emissive (zombie eyes glow)
    result += nightFactor * 0.05;
    // Exponential fog
    float fog = 1.0 - exp(-fogDensity * Dist);
    result = mix(result, fogColor, clamp(fog, 0.0, 1.0));
    FragColor = vec4(result, alpha);
}
)";
        auto compile = [](GLenum type, const char* src) {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
                std::cerr << "Shader compile error: " << log << "\n";
            }
            return s;
        };
        GLuint v = compile(GL_VERTEX_SHADER, vs);
        GLuint f = compile(GL_FRAGMENT_SHADER, fs);
        GLuint p = glCreateProgram();
        glAttachShader(p,v); glAttachShader(p,f);
        glLinkProgram(p);
        glDeleteShader(v); glDeleteShader(f);
        return p;
    }
};
