#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

class DayNight {
public:
    float timeOfDay = 0.25f;   
    float daySpeed  = 0.0002f;

    void update(float dt) {
        timeOfDay += daySpeed * dt * 60.0f;
        if (timeOfDay >= 1.0f) timeOfDay -= 1.0f;
    }
    
    void setSpeed(float s) { daySpeed = s; }

    glm::vec3 getSunDirection() const {
        float angle = timeOfDay * 2.0f * 3.14159265f;
        return glm::normalize(glm::vec3(cosf(angle), sinf(angle) * 0.8f + 0.2f, 0.3f));
    }

    static glm::vec3 lerpV(glm::vec3 a, glm::vec3 b, float t) {
        return a + (b - a) * std::max(0.0f, std::min(1.0f, t));
    }

    glm::vec3 getSunColor() const {
        float t = timeOfDay;

        glm::vec3 dawn  = {1.00f, 0.60f, 0.30f};
        glm::vec3 noon  = {1.00f, 0.95f, 0.80f};
        glm::vec3 dusk  = {1.00f, 0.40f, 0.10f};

        if      (t < 0.3f) return lerpV(dawn, dawn, t / 0.3f);
        else if (t < 0.5f) return lerpV(dawn, noon, (t - 0.3f) / 0.2f);
        else if (t < 0.7f) return lerpV(noon, noon, (t - 0.5f) / 0.2f);
        else               return lerpV(noon, dusk, (t - 0.7f) / 0.3f);
    }

    float getAmbientStrength() const {
        // Always daylight range (no night)
        float t = timeOfDay;
        float minVal = 0.25f;
        float maxVal = 0.50f;

        float factor = sinf(t * 3.14159265f); // smooth curve
        return minVal + (maxVal - minVal) * factor;
    }
float getNightFactor() const {
    return 0.0f; // always day
}
    float getFogDensity() const {
        return 0.006f; // constant (no night variation)
    }

    glm::vec3 getFogColor() const {
        return {0.14f, 0.20f, 0.10f}; // fixed day fog
    }

    glm::vec3 getSkyTopColor() const {
        return {0.28f, 0.52f, 0.68f}; // always day sky
    }

    glm::vec3 getSkyBottomColor() const {
        return {0.56f, 0.76f, 0.52f};
    }

};