#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

class Window {
public:
    GLFWwindow* handle = nullptr;
    int width, height;

    bool init(int w, int h, const std::string& title) {
        width = w; height = h;
        if (!glfwInit()) { std::cerr << "[Window] glfwInit failed\n"; return false; }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        handle = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
        if (!handle) { std::cerr << "[Window] glfwCreateWindow failed\n"; glfwTerminate(); return false; }
        glfwMakeContextCurrent(handle);
        glfwSwapInterval(1); // vsync
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) { std::cerr << "[Window] glewInit failed\n"; return false; }
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return true;
    }

    bool shouldClose() const { return glfwWindowShouldClose(handle); }
    void swapBuffers()        { glfwSwapBuffers(handle); }
    void pollEvents()         { glfwPollEvents(); }
    void setTitle(const std::string& t) { glfwSetWindowTitle(handle, t.c_str()); }

    void captureMouse(bool capture) {
        glfwSetInputMode(handle, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    float getAspect() const { return (float)width / (float)height; }

    ~Window() { if (handle) glfwDestroyWindow(handle); glfwTerminate(); }
};
