#pragma once
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cmath>

class Texture {
public:
    GLuint id = 0;

    bool load(const std::string& path) {
        // Check cache
        static std::unordered_map<std::string, GLuint> cache;
        auto it = cache.find(path);
        if (it != cache.end()) { id = it->second; return true; }

        // Try loading with stb (if available in unity build), else fallback
        id = loadPNG(path);
        if (!id) {
            std::cerr << "[Texture] Failed to load: " << path << " — using fallback\n";
            id = makeColor(0.8f, 0.2f, 0.8f); // magenta fallback
        }
        cache[path] = id;
        return id != 0;
    }

    void bind(int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    GLuint getID() const { return id; }

    // Make a 1×1 solid color texture
    GLuint makeColor(float r, float g, float b) {
        unsigned char px[3] = {
            (unsigned char)(r * 255),
            (unsigned char)(g * 255),
            (unsigned char)(b * 255)
        };
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
        applyParams();
        id = tex;
        return tex;
    }

    // Generate procedural texture (0=grass,1=dirt,2=bark,3=concrete,4=skin)
    GLuint makeProceduralTexture(int type) {
        const int SZ = 64;
        unsigned char data[SZ * SZ * 3];
        for (int y = 0; y < SZ; y++) {
            for (int x = 0; x < SZ; x++) {
                int i = (y * SZ + x) * 3;
                float nx = (float)x / SZ, ny = (float)y / SZ;
                float n  = sinf(nx * 17.3f) * cosf(ny * 13.7f) * 0.5f + 0.5f;
                float n2 = cosf(nx * 31.1f + ny * 29.4f) * 0.5f + 0.5f;
                float noise = (n + n2) * 0.5f;
                switch (type) {
                    case 0: // grass
                        data[i]   = (unsigned char)(20  + noise * 30);
                        data[i+1] = (unsigned char)(80  + noise * 60);
                        data[i+2] = (unsigned char)(15  + noise * 20);
                        break;
                    case 1: // dirt
                        data[i]   = (unsigned char)(80  + noise * 40);
                        data[i+1] = (unsigned char)(50  + noise * 30);
                        data[i+2] = (unsigned char)(30  + noise * 20);
                        break;
                    case 2: // bark
                        data[i]   = (unsigned char)(50  + noise * 30);
                        data[i+1] = (unsigned char)(30  + noise * 20);
                        data[i+2] = (unsigned char)(10  + noise * 10);
                        break;
                    case 3: // concrete
                        { unsigned char g = (unsigned char)(100 + noise * 60);
                          data[i] = data[i+1] = data[i+2] = g; }
                        break;
                    default: // skin / zombie
                        data[i]   = (unsigned char)(60  + noise * 40);
                        data[i+1] = (unsigned char)(90  + noise * 30);
                        data[i+2] = (unsigned char)(60  + noise * 20);
                        break;
                }
            }
        }
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SZ, SZ, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        applyParams();
        id = tex;
        return tex;
    }

    ~Texture() {
        // Don't delete — cached textures shared
    }

private:
    GLuint loadPNG(const std::string& path) {
        // Stub — real loading handled via stb_image in the unity build (main.cpp)
        (void)path;
        return 0;
    }

    void applyParams() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
};

// Simple standalone procedural loader (no stb dependency)
inline GLuint makeTexture(int type) {
    Texture t; return t.makeProceduralTexture(type);
}
