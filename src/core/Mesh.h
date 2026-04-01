#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <sstream>
#include <fstream>
#include <array>

// Vertex layout: location 0 = vec3 pos, 1 = vec3 normal, 2 = vec2 uv
struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    int    indexCount = 0;

    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }

    void free() {
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
        if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
        indexCount = 0;
    }

private:
    void upload(const std::vector<float>& verts, const std::vector<unsigned int>& idx) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
        // pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // uv
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        indexCount = (int)idx.size();
    }

public:
    static Mesh buildBox(float w, float h, float d) {
        float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;
        std::vector<float> v = {
            // Front +Z
            -hw,-hh, hd, 0,0,1, 0,0,
             hw,-hh, hd, 0,0,1, 1,0,
             hw, hh, hd, 0,0,1, 1,1,
            -hw, hh, hd, 0,0,1, 0,1,
            // Back -Z
             hw,-hh,-hd, 0,0,-1, 0,0,
            -hw,-hh,-hd, 0,0,-1, 1,0,
            -hw, hh,-hd, 0,0,-1, 1,1,
             hw, hh,-hd, 0,0,-1, 0,1,
            // Left -X
            -hw,-hh,-hd,-1,0,0, 0,0,
            -hw,-hh, hd,-1,0,0, 1,0,
            -hw, hh, hd,-1,0,0, 1,1,
            -hw, hh,-hd,-1,0,0, 0,1,
            // Right +X
             hw,-hh, hd, 1,0,0, 0,0,
             hw,-hh,-hd, 1,0,0, 1,0,
             hw, hh,-hd, 1,0,0, 1,1,
             hw, hh, hd, 1,0,0, 0,1,
            // Top +Y
            -hw, hh, hd, 0,1,0, 0,0,
             hw, hh, hd, 0,1,0, 1,0,
             hw, hh,-hd, 0,1,0, 1,1,
            -hw, hh,-hd, 0,1,0, 0,1,
            // Bottom -Y
            -hw,-hh,-hd, 0,-1,0, 0,0,
             hw,-hh,-hd, 0,-1,0, 1,0,
             hw,-hh, hd, 0,-1,0, 1,1,
            -hw,-hh, hd, 0,-1,0, 0,1,
        };
        std::vector<unsigned int> idx;
        for (int f = 0; f < 6; f++) {
            unsigned b = f * 4;
            idx.insert(idx.end(), {b, b+1, b+2, b, b+2, b+3});
        }
        Mesh m; m.upload(v, idx); return m;
    }

    static Mesh buildGround(float size, float texRepeat) {
        std::vector<float> v = {
            -size, 0, -size, 0,1,0, 0,0,
             size, 0, -size, 0,1,0, texRepeat,0,
             size, 0,  size, 0,1,0, texRepeat,texRepeat,
            -size, 0,  size, 0,1,0, 0,texRepeat,
        };
        std::vector<unsigned int> idx = {0,1,2,0,2,3};
        Mesh m; m.upload(v, idx); return m;
    }

    static Mesh buildCylinder(float radius, float height, int segs = 16) {
        const float PI = 3.14159265f;
        std::vector<float> verts;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= segs; i++) {
            float a = i * 2.0f * PI / segs;
            float c = cosf(a), s = sinf(a);
            verts.insert(verts.end(), {c*radius, 0,      s*radius, c,0,s, (float)i/segs, 0});
            verts.insert(verts.end(), {c*radius, height, s*radius, c,0,s, (float)i/segs, 1});
        }
        for (int i = 0; i < segs; i++) {
            unsigned b = i * 2;
            idx.insert(idx.end(), {b, b+2, b+1, b+1, b+2, b+3});
        }
        Mesh m; m.upload(verts, idx); return m;
    }

    // Load OBJ using simple built-in parser (no external lib required)
    bool loadOBJ(const std::string& path) {
        std::vector<glm::vec3> positions, normals;
        std::vector<glm::vec2> uvs;
        std::vector<float>        verts;
        std::vector<unsigned int> idx;
        std::ifstream file(path);
        if (!file.is_open()) { std::cerr << "[Mesh] Cannot open OBJ: " << path << "\n"; return false; }
        std::string line;
        while (std::getline(file, line)) {
            if (line.size() < 2) continue;
            std::istringstream ss(line);
            std::string tok; ss >> tok;
            if (tok == "v")  { float x,y,z; ss>>x>>y>>z; positions.push_back({x,y,z}); }
            else if (tok == "vn") { float x,y,z; ss>>x>>y>>z; normals.push_back({x,y,z}); }
            else if (tok == "vt") { float u,v;     ss>>u>>v;     uvs.push_back({u,v}); }
            else if (tok == "f") {
                // Parse face — support v v v and v/vt/vn v/vt/vn v/vt/vn
                std::vector<std::array<int,3>> face;
                std::string fv;
                while (ss >> fv) {
                    std::array<int,3> fi = {0,0,0};
                    // parse p/t/n or p//n or p/t or p
                    int s1 = fv.find('/');
                    fi[0] = std::stoi(fv.substr(0, s1));
                    if (s1 != (int)std::string::npos) {
                        int s2 = fv.find('/', s1+1);
                        if (s2 != (int)std::string::npos) {
                            if (s2 > s1+1) fi[1] = std::stoi(fv.substr(s1+1, s2-s1-1));
                            fi[2] = std::stoi(fv.substr(s2+1));
                        } else { fi[1] = std::stoi(fv.substr(s1+1)); }
                    }
                    face.push_back(fi);
                }
                // Triangulate polygon (fan)
                for (int i = 1; i+1 < (int)face.size(); i++) {
                    for (int k : {0, i, i+1}) {
                        auto& f = face[k];
                        int pi = f[0]-1, ti = f[1]-1, ni = f[2]-1;
                        glm::vec3 p = (pi>=0 && pi<(int)positions.size()) ? positions[pi] : glm::vec3(0);
                        glm::vec2 t = (ti>=0 && ti<(int)uvs.size())       ? uvs[ti]        : glm::vec2(0);
                        glm::vec3 n = (ni>=0 && ni<(int)normals.size())   ? normals[ni]    : glm::vec3(0,1,0);
                        verts.insert(verts.end(), {p.x,p.y,p.z, n.x,n.y,n.z, t.x,t.y});
                        idx.push_back((unsigned)idx.size());
                    }
                }
            }
        }
        if (verts.empty()) { std::cerr << "[Mesh] OBJ empty: " << path << "\n"; return false; }
        upload(verts, idx);
        return true;
    }
};

// End of Mesh.h
