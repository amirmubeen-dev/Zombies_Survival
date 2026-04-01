#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <cstdlib>

class Camera {
public:
    glm::vec3 pos;
    glm::vec3 targetPos;
    glm::vec3 front, up, right;
    glm::vec3 worldUp;
    float     yaw, pitch;
    float     fov;
    float     targetFov;
    float     shakeIntensity;

    Camera() : pos(0,0,0), targetPos(0,0,0), front(0,0,-1), worldUp(0,1,0),
               yaw(-90.0f), pitch(0.0f), fov(70.0f), targetFov(70.0f), shakeIntensity(0.0f) {
        updateDirection();
    }

    void updateDirection() {
        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 f;
        f.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
        f.y = sinf(glm::radians(pitch));
        f.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
        front = glm::normalize(f);

        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }

    void applyMouseLook(float dx, float dy, float sensitivity = 0.1f) {
        yaw   += dx * sensitivity;
        pitch += dy * sensitivity;
        updateDirection();
    }

    glm::mat4 getView() const {
        glm::vec3 actualPos = pos;
        if (shakeIntensity > 0.001f) {
            float rx = ((float)rand()/RAND_MAX - 0.5f) * shakeIntensity;
            float ry = ((float)rand()/RAND_MAX - 0.5f) * shakeIntensity;
            float rz = ((float)rand()/RAND_MAX - 0.5f) * shakeIntensity;
            actualPos += glm::vec3(rx,ry,rz);
        }
        return glm::lookAt(actualPos, actualPos + front, up);
    }

    glm::mat4 getProj(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 500.0f);
    }

    void update(float dt) {
        // Smooth lerping
        pos = glm::mix(pos, targetPos, 1.0f - exp2f(-10.0f * dt));
        fov = glm::mix(fov, targetFov, 1.0f - exp2f(-15.0f * dt));
        if (shakeIntensity > 0) {
            shakeIntensity *= exp2f(-10.0f * dt);
        }
    }
    
    // Gives the horizontal plane forward direction
    glm::vec3 getFlatFront() const {
        return glm::normalize(glm::vec3(front.x, 0, front.z));
    }
    glm::vec3 getRight() const { return right; }
};
