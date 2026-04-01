#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct Keyframe {
    float      time;
    glm::vec3  position;
    glm::quat  rotation;
    glm::vec3  scale = {1, 1, 1};
};

struct BoneTrack {
    int                  boneIndex;
    std::vector<Keyframe> keyframes;

    glm::mat4 evaluate(float t) const {
        if (keyframes.empty()) return glm::mat4(1.0f);
        if (keyframes.size() == 1) return makeTransform(keyframes[0]);
        // Find surrounding keyframes
        int hi = 1;
        while (hi < (int)keyframes.size() - 1 && keyframes[hi].time < t) hi++;
        int lo = hi - 1;
        float span = keyframes[hi].time - keyframes[lo].time;
        float alpha = (span > 0.0001f) ? (t - keyframes[lo].time) / span : 0.0f;
        alpha = std::max(0.0f, std::min(1.0f, alpha));

        glm::vec3 pos   = glm::mix(keyframes[lo].position, keyframes[hi].position, alpha);
        glm::quat rot   = glm::slerp(keyframes[lo].rotation, keyframes[hi].rotation, alpha);
        glm::vec3 scale = glm::mix(keyframes[lo].scale, keyframes[hi].scale, alpha);

        glm::mat4 m = glm::mat4_cast(rot);
        m[3] = {pos.x, pos.y, pos.z, 1};
        return glm::scale(m, scale);
    }

private:
    static glm::mat4 makeTransform(const Keyframe& kf) {
        glm::mat4 m = glm::mat4_cast(kf.rotation);
        m[3] = {kf.position.x, kf.position.y, kf.position.z, 1};
        return glm::scale(m, kf.scale);
    }
};

struct Bone {
    std::string name;
    int         parentIndex = -1;
    glm::mat4   localTransform = glm::mat4(1.0f);
    glm::mat4   inverseBindPose = glm::mat4(1.0f);
};

struct SkeletalAnimation {
    std::string              name;
    float                    duration    = 1.0f;
    float                    currentTime = 0.0f;
    bool                     loop        = true;
    std::vector<BoneTrack>   tracks;

    void update(float dt) {
        currentTime += dt;
        if (loop && currentTime > duration) currentTime = fmodf(currentTime, duration);
        else if (!loop && currentTime > duration) currentTime = duration;
    }

    glm::mat4 getBoneTransform(int boneIndex, const glm::mat4& parentWorld) const {
        for (auto& track : tracks) {
            if (track.boneIndex == boneIndex)
                return parentWorld * track.evaluate(currentTime);
        }
        return parentWorld;
    }
};

struct Skeleton {
    std::vector<Bone>              bones;
    std::vector<SkeletalAnimation> animations;
    int                            currentAnim = 0;

    void playAnimation(const std::string& n, bool loop = true) {
        for (int i = 0; i < (int)animations.size(); i++) {
            if (animations[i].name == n) {
                currentAnim = i;
                animations[i].currentTime = 0;
                animations[i].loop = loop;
                return;
            }
        }
    }

    void update(float dt) {
        if (currentAnim < (int)animations.size())
            animations[currentAnim].update(dt);
    }

    // Simplified: return bone world transform (for box-based characters)
    glm::mat4 getFinalTransform(int boneIndex) const {
        if (currentAnim >= (int)animations.size()) return glm::mat4(1.0f);
        const SkeletalAnimation& anim = animations[currentAnim];
        glm::mat4 world = glm::mat4(1.0f);
        // Walk up bone chain
        std::vector<int> chain;
        int idx = boneIndex;
        while (idx >= 0) { chain.push_back(idx); idx = bones[idx].parentIndex; }
        std::reverse(chain.begin(), chain.end());
        for (int bi : chain) world = anim.getBoneTransform(bi, world);
        return world;
    }

    // --- Factory: build a simple zombie skeleton ---
    static Skeleton buildZombieSkeleton() {
        Skeleton sk;
        // Bone order: root, spine, head, lUpArm, lForeArm, rUpArm, rForeArm, lThigh, lShin, rThigh, rShin
        auto addBone = [&](const std::string& name, int parent) {
            Bone b; b.name = name; b.parentIndex = parent;
            sk.bones.push_back(b);
        };
        addBone("root",      -1);  // 0
        addBone("spine",      0);  // 1
        addBone("head",       1);  // 2
        addBone("lUpperArm",  1);  // 3
        addBone("lForeArm",   3);  // 4
        addBone("rUpperArm",  1);  // 5
        addBone("rForeArm",   5);  // 6
        addBone("lThigh",     0);  // 7
        addBone("lShin",      7);  // 8
        addBone("rThigh",     0);  // 9
        addBone("rShin",      9);  // 10

        sk.animations.push_back(buildWalkAnimation(sk));
        sk.animations.push_back(buildAttackAnimation(sk));
        sk.animations.push_back(buildDeathAnimation(sk));
        sk.currentAnim = 0;
        return sk;
    }

    static SkeletalAnimation buildWalkAnimation(const Skeleton&) {
        SkeletalAnimation anim;
        anim.name = "walk"; anim.duration = 1.2f; anim.loop = true;
        const float PI = 3.14159265f;
        // Arm swing
        {   BoneTrack t; t.boneIndex = 3; // lUpperArm
            t.keyframes = {
                {0.0f, {-0.35f, 0.5f, 0}, glm::angleAxis(0.5f, glm::vec3(1,0,0)), {1,1,1}},
                {0.6f, {-0.35f, 0.5f, 0}, glm::angleAxis(-0.5f,glm::vec3(1,0,0)), {1,1,1}},
                {1.2f, {-0.35f, 0.5f, 0}, glm::angleAxis(0.5f, glm::vec3(1,0,0)), {1,1,1}},
            }; anim.tracks.push_back(t);
        }
        {   BoneTrack t; t.boneIndex = 5; // rUpperArm
            t.keyframes = {
                {0.0f, {0.35f, 0.5f, 0}, glm::angleAxis(-0.5f,glm::vec3(1,0,0)), {1,1,1}},
                {0.6f, {0.35f, 0.5f, 0}, glm::angleAxis(0.5f, glm::vec3(1,0,0)), {1,1,1}},
                {1.2f, {0.35f, 0.5f, 0}, glm::angleAxis(-0.5f,glm::vec3(1,0,0)), {1,1,1}},
            }; anim.tracks.push_back(t);
        }
        // Leg swing
        {   BoneTrack t; t.boneIndex = 7; // lThigh
            t.keyframes = {
                {0.0f, {-0.18f,-0.7f,0}, glm::angleAxis(-0.5f,glm::vec3(1,0,0)), {1,1,1}},
                {0.6f, {-0.18f,-0.7f,0}, glm::angleAxis(0.5f, glm::vec3(1,0,0)), {1,1,1}},
                {1.2f, {-0.18f,-0.7f,0}, glm::angleAxis(-0.5f,glm::vec3(1,0,0)), {1,1,1}},
            }; anim.tracks.push_back(t);
        }
        {   BoneTrack t; t.boneIndex = 9; // rThigh
            t.keyframes = {
                {0.0f, {0.18f,-0.7f,0}, glm::angleAxis(0.5f, glm::vec3(1,0,0)), {1,1,1}},
                {0.6f, {0.18f,-0.7f,0}, glm::angleAxis(-0.5f,glm::vec3(1,0,0)), {1,1,1}},
                {1.2f, {0.18f,-0.7f,0}, glm::angleAxis(0.5f, glm::vec3(1,0,0)), {1,1,1}},
            }; anim.tracks.push_back(t);
        }
        return anim;
    }

    static SkeletalAnimation buildAttackAnimation(const Skeleton&) {
        SkeletalAnimation anim;
        anim.name = "attack"; anim.duration = 0.6f; anim.loop = false;
        {   BoneTrack t; t.boneIndex = 5; // rUpperArm (swipe)
            t.keyframes = {
                {0.0f, {0.35f,0.5f,0}, glm::angleAxis(-1.2f,glm::vec3(1,0,0)), {1,1,1}},
                {0.2f, {0.35f,0.5f,0}, glm::angleAxis( 0.8f,glm::vec3(1,0,0)), {1,1,1}},
                {0.6f, {0.35f,0.5f,0}, glm::angleAxis(-1.2f,glm::vec3(1,0,0)), {1,1,1}},
            }; anim.tracks.push_back(t);
        }
        return anim;
    }

    static SkeletalAnimation buildDeathAnimation(const Skeleton&) {
        SkeletalAnimation anim;
        anim.name = "death"; anim.duration = 1.0f; anim.loop = false;
        {   BoneTrack t; t.boneIndex = 1; // spine — fall forward
            t.keyframes = {
                {0.0f, {0,0.5f,0}, glm::angleAxis(0.0f,  glm::vec3(1,0,0)), {1,1,1}},
                {0.8f, {0,-0.3f,0}, glm::angleAxis(1.5f, glm::vec3(1,0,0)), {1,1,1}},
                {1.0f, {0,-0.5f,0}, glm::angleAxis(1.57f,glm::vec3(1,0,0)), {1,1,1}},
            }; anim.tracks.push_back(t);
        }
        return anim;
    }
};
