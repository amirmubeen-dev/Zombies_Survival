#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    GLuint id = 0;

    Shader() = default;

    // Compile from inline source strings
    bool compileFromSource(const char* vertSrc, const char* fragSrc) {
        GLuint vert = compileStage(GL_VERTEX_SHADER, vertSrc);
        GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragSrc);
        if (!vert || !frag) return false;
        id = glCreateProgram();
        glAttachShader(id, vert);
        glAttachShader(id, frag);
        glLinkProgram(id);
        int ok; glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetProgramInfoLog(id, 512, nullptr, log);
            std::cerr << "[Shader] Link error: " << log << "\n";
            glDeleteProgram(id); id = 0;
        }
        glDeleteShader(vert); glDeleteShader(frag);
        return id != 0;
    }

    // Compile from files
    bool compileFromFiles(const std::string& vertPath, const std::string& fragPath) {
        std::string vs = readFile(vertPath);
        std::string fs = readFile(fragPath);
        if (vs.empty() || fs.empty()) return false;
        return compileFromSource(vs.c_str(), fs.c_str());
    }

    void use() const { glUseProgram(id); }

    // Uniform helpers
    void setInt(const char* name, int v)           const { glUniform1i(loc(name), v); }
    void setFloat(const char* name, float v)       const { glUniform1f(loc(name), v); }
    void setBool(const char* name, bool v)         const { glUniform1i(loc(name), (int)v); }
    void setVec2(const char* name, float x, float y)   const { glUniform2f(loc(name), x, y); }
    void setVec3(const char* name, float x, float y, float z) const { glUniform3f(loc(name), x, y, z); }
    void setVec3(const char* name, const glm::vec3& v) const { glUniform3fv(loc(name), 1, glm::value_ptr(v)); }
    void setVec4(const char* name, float r, float g, float b, float a) const { glUniform4f(loc(name), r, g, b, a); }
    void setMat4(const char* name, const glm::mat4& m) const { glUniformMatrix4fv(loc(name), 1, GL_FALSE, glm::value_ptr(m)); }

    ~Shader() { if (id) glDeleteProgram(id); }

private:
    GLint loc(const char* name) const { return glGetUniformLocation(id, name); }

    static GLuint compileStage(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "[Shader] Compile error (" << (type==GL_VERTEX_SHADER?"vert":"frag") << "): " << log << "\n";
            glDeleteShader(s); return 0;
        }
        return s;
    }

    static std::string readFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) { std::cerr << "[Shader] Cannot open: " << path << "\n"; return ""; }
        std::stringstream ss; ss << f.rdbuf();
        return ss.str();
    }
};
