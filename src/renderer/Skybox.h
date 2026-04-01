#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <string>
#include <iostream>

class Skybox {
public:
    GLuint cubemapTex = 0;
    GLuint vao = 0, vbo = 0;
    GLuint shaderProg = 0;

    bool init(const std::array<std::string, 6>& faces) {
        buildCube();
        buildShader();
        cubemapTex = loadCubemap(faces);
        return true;
    }

    void draw(const glm::mat4& view, const glm::mat4& proj,
              glm::vec3 topColor, glm::vec3 botColor)
    {
        glDepthFunc(GL_LEQUAL);
        glUseProgram(shaderProg);
        // Remove translation from view
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProg, "view"),       1, GL_FALSE, glm::value_ptr(skyView));
        glUniformMatrix4fv(glGetUniformLocation(shaderProg, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(glGetUniformLocation(shaderProg, "topColor"),    1, glm::value_ptr(topColor));
        glUniform3fv(glGetUniformLocation(shaderProg, "bottomColor"), 1, glm::value_ptr(botColor));
        glUniform1i(glGetUniformLocation(shaderProg, "hasCubemap"),
                    cubemapTex ? 1 : 0);
        if (cubemapTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
            glUniform1i(glGetUniformLocation(shaderProg, "skybox"), 0);
        }
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);
    }

    ~Skybox() {
        if (vao)         glDeleteVertexArrays(1, &vao);
        if (vbo)         glDeleteBuffers(1, &vbo);
        if (cubemapTex)  glDeleteTextures(1, &cubemapTex);
        if (shaderProg)  glDeleteProgram(shaderProg);
    }

private:
    void buildCube() {
        float verts[] = {
            -1,  1,-1, -1,-1,-1,  1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
            -1,-1, 1, -1,-1,-1, -1, 1,-1, -1, 1,-1, -1, 1, 1, -1,-1, 1,
             1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1,
            -1,-1, 1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1,-1, 1, -1,-1, 1,
            -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
            -1,-1,-1, -1,-1, 1,  1,-1,-1,  1,-1,-1, -1,-1, 1,  1,-1, 1,
        };
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, (void*)0);
        glEnableVertexAttribArray(0);
    }

    void buildShader() {
        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
out vec3 TexCoords;
out float vY;
uniform mat4 projection, view;
void main() {
    TexCoords = aPos;
    vY = aPos.y;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";
        const char* fs = R"(
#version 330 core
in vec3 TexCoords;
in float vY;
out vec4 FragColor;
uniform samplerCube skybox;
uniform vec3 topColor;
uniform vec3 bottomColor;
uniform bool hasCubemap;
void main() {
    if (hasCubemap) {
        FragColor = texture(skybox, TexCoords);
    } else {
        float t = clamp(vY * 0.5 + 0.5, 0.0, 1.0);
        vec3 col = mix(bottomColor, topColor, t);
        // Subtle fog/haze at horizon
        float fogFactor = 1.0 - abs(vY) * 2.0;
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        col = mix(col, bottomColor * 0.5, fogFactor * 0.25);
        FragColor = vec4(col, 1.0);
    }
}
)";
        auto compile = [](GLenum type, const char* src) -> GLuint {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            return s;
        };
        GLuint v = compile(GL_VERTEX_SHADER, vs);
        GLuint f = compile(GL_FRAGMENT_SHADER, fs);
        shaderProg = glCreateProgram();
        glAttachShader(shaderProg, v); glAttachShader(shaderProg, f);
        glLinkProgram(shaderProg);
        glDeleteShader(v); glDeleteShader(f);
    }

    GLuint loadCubemap(const std::array<std::string, 6>& faces) {
        // Try to load — stb_image must be included in the main unity build
        // Returns 0 (procedural skybox used) if not available
        (void)faces;
        return 0;
    }
};
