#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

class DayNight {
public:
    float timeOfDay = 0.25f;   // 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk, 1=midnight
    float daySpeed  = 0.0002f; // One cycle ~5000s by default; speed up via setter

    void update(float dt) {
        timeOfDay += daySpeed * dt * 60.0f;
        if (timeOfDay >= 1.0f) timeOfDay -= 1.0f;
    }

    void setSpeed(float s) { daySpeed = s; }

    // Sun direction (directional light) — follows solar arc
    glm::vec3 getSunDirection() const {
        float angle = timeOfDay * 2.0f * 3.14159265f;
        return glm::normalize(glm::vec3(cosf(angle), sinf(angle) * 0.8f + 0.2f, 0.3f));
    }

    // Lerp helper
    static glm::vec3 lerpV(glm::vec3 a, glm::vec3 b, float t) {
        return a + (b - a) * std::max(0.0f, std::min(1.0f, t));
    }

    glm::vec3 getSunColor() const {
        float t = timeOfDay;
        // Night: 0.0
        glm::vec3 night = {0.10f, 0.10f, 0.25f};
        glm::vec3 dawn  = {1.00f, 0.60f, 0.30f};
        glm::vec3 noon  = {1.00f, 0.95f, 0.80f};
        glm::vec3 dusk  = {1.00f, 0.40f, 0.10f};

        if      (t < 0.2f) return lerpV(night, night, t / 0.2f);
        else if (t < 0.3f) return lerpV(night, dawn,  (t-0.2f)/0.1f);
        else if (t < 0.5f) return lerpV(dawn,  noon,  (t-0.3f)/0.2f);
        else if (t < 0.7f) return lerpV(noon,  noon,  (t-0.5f)/0.2f);
        else if (t < 0.8f) return lerpV(noon,  dusk,  (t-0.7f)/0.1f);
        else if (t < 0.9f) return lerpV(dusk,  night, (t-0.8f)/0.1f);
        else               return night;
    }

    float getAmbientStrength() const {
        // 0.05 at night, 0.5 at noon
        float t = timeOfDay;
        float night = 0.05f, day = 0.50f;
        if (t < 0.25f || t > 0.85f) return night;
        float mid = (t < 0.55f) ? (t - 0.25f) / 0.3f : 1.0f - (t - 0.55f) / 0.3f;
        return night + (day - night) * std::max(0.0f, std::min(1.0f, mid));
    }

    float getFogDensity() const {
        // Denser at night
        return 0.008f + (1.0f - getNightFactor()) * 0.005f;
    }

    glm::vec3 getFogColor() const {
        float nf = getNightFactor();
        glm::vec3 dayFog  = {0.14f, 0.20f, 0.10f};
        glm::vec3 nightFog = {0.03f, 0.04f, 0.06f};
        return lerpV(dayFog, nightFog, nf);
    }

    glm::vec3 getSkyTopColor() const {
        float nf = getNightFactor();
        glm::vec3 dayTop   = {0.28f, 0.52f, 0.68f};
        glm::vec3 nightTop = {0.02f, 0.03f, 0.08f};
        return lerpV(dayTop, nightTop, nf);
    }

    glm::vec3 getSkyBottomColor() const {
        float nf = getNightFactor();
        glm::vec3 dayBot   = {0.56f, 0.76f, 0.52f};
        glm::vec3 nightBot = {0.05f, 0.08f, 0.05f};
        return lerpV(dayBot, nightBot, nf);
    }

    // 0=day, 1=night
    float getNightFactor() const {
        float t = timeOfDay;
        if (t < 0.2f)  return 1.0f - t / 0.2f;
        if (t < 0.5f)  return 0.0f;
        if (t < 0.8f)  return 0.0f;
        if (t < 0.9f)  return (t - 0.8f) / 0.1f;
        return 1.0f;
    }

    bool isNight() const { return getNightFactor() > 0.5f; }
};
