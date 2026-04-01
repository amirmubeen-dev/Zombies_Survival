# MIXAMO + ASSIMP MODEL INTEGRATION GUIDE
# 3D Zombie Survival Game — Real Animated Characters

================================================================
STEP 1: INSTALL ASSIMP (MSYS2 MinGW64 Admin terminal)
================================================================

pacman -S mingw-w64-x86_64-assimp

Add to compile command:
g++ main.cpp -o game.exe -I. -I../vendor \
  -lglfw3 -lglew32 -lopengl32 -lglu32 -lopenal -lassimp \
  -fopenmp -std=c++17

================================================================
STEP 2: MIXAMO DOWNLOAD SETTINGS
================================================================

Website: https://www.mixamo.com
Login: Free Adobe account

ZOMBIE CHARACTER:
1. Characters tab → search "Zombie" → pick any
2. Animations tab → download these ONE BY ONE:
   - "Zombie Walk"     → Format: FBX, Skin: With Skin, 30fps
   - "Zombie Run"      → Format: FBX, Skin: With Skin, 30fps  
   - "Zombie Attack"   → Format: FBX, Skin: With Skin, 30fps
   - "Zombie Death"    → Format: FBX, Skin: With Skin, 30fps

PLAYER CHARACTER:
1. Characters tab → search "Soldier" or use "Y Bot"
2. Download these:
   - "Rifle Walk"      → Format: FBX, Skin: With Skin, 30fps
   - "Rifle Run"       → Format: FBX, Skin: With Skin, 30fps
   - "Rifle Idle"      → Format: FBX, Skin: With Skin, 30fps

PUT FILES HERE:
d:\zombies\zombie3d\assets\models\
├── zombie_walk.fbx
├── zombie_run.fbx
├── zombie_attack.fbx
├── zombie_death.fbx
├── player_walk.fbx
├── player_run.fbx
└── player_idle.fbx

================================================================
STEP 3: AI PROMPT — Paste this to Claude/ChatGPT
================================================================

```
You are an expert C++ OpenGL game developer.

I have a 3D zombie survival game using:
- OpenGL 3.3, GLFW, GLEW, GLM, OpenMP
- MSYS2 MinGW64, C++17
- Assimp library (already installed via pacman)
- Mixamo FBX files for zombie and player characters

Project location: d:\zombies\zombie3d\
Assets location:  d:\zombies\zombie3d\assets\models\

[PASTE YOUR CURRENT src/core/Mesh.h HERE]
[PASTE YOUR CURRENT src/entities/Zombie.h HERE]
[PASTE YOUR CURRENT src/entities/Player.h HERE]
[PASTE YOUR CURRENT src/renderer/Renderer.h HERE]

================================================================
CREATE FILE 1: src/core/ModelLoader.h
================================================================

A complete Assimp-based animated 3D model loader.

STRUCTS NEEDED:

struct BoneInfo {
    glm::mat4 offsetMatrix;   // bone space to mesh space
    int       id;
};

struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    int       boneIDs[4];     // up to 4 bones per vertex
    float     weights[4];     // corresponding weights
};

struct AnimKeyframe {
    double         time;
    glm::vec3      position;
    glm::quat      rotation;
    glm::vec3      scale;
};

struct AnimBoneChannel {
    std::string              boneName;
    std::vector<AnimKeyframe> posKeys;
    std::vector<AnimKeyframe> rotKeys;
    std::vector<AnimKeyframe> scaleKeys;
};

struct Animation {
    std::string                    name;
    double                         duration;      // in ticks
    double                         ticksPerSecond;
    std::vector<AnimBoneChannel>   channels;
};

struct SkinnedMesh {
    GLuint                         vao, vbo, ebo;
    int                            indexCount;
    GLuint                         texture;       // diffuse texture
};

class AnimatedModel {
public:
    // Load FBX file with Assimp
    bool load(const std::string& path);

    // Update animation (call every frame)
    // animIndex: which animation to play
    // timeSeconds: current animation time (loop automatically)
    void update(int animIndex, float timeSeconds);

    // Draw with skinning shader
    // shader must have uniform mat4 boneTransforms[100]
    void draw(GLuint shader);

    // Get bone transform matrices (pass to shader)
    // Returns vector of up to 100 bone matrices
    std::vector<glm::mat4> getBoneTransforms();

    // Animation info
    int   getAnimationCount();
    float getAnimationDuration(int index); // in seconds

    // Bone count (for HUD debug)
    int getBoneCount();

private:
    std::vector<SkinnedMesh>  meshes;
    std::vector<Animation>    animations;
    std::map<std::string, BoneInfo> boneMap;
    std::vector<glm::mat4>    boneTransforms;
    int                        boneCount = 0;

    // Assimp scene (keep alive while model loaded)
    const aiScene* scene = nullptr;
    Assimp::Importer importer;

    // Node hierarchy traversal
    void processNode(aiNode* node, const aiScene* scene);
    SkinnedMesh processMesh(aiMesh* mesh, const aiScene* scene);

    // Animation calculation
    glm::mat4 calcBoneTransform(int animIndex, float timeSeconds,
                                 const std::string& boneName,
                                 glm::mat4 parentTransform);
    void calcAllBoneTransforms(int animIndex, float timeSeconds);

    // Interpolation helpers
    glm::vec3 interpolatePosition(float time, const AnimBoneChannel& ch);
    glm::quat interpolateRotation(float time, const AnimBoneChannel& ch);
    glm::vec3 interpolateScale(float time, const AnimBoneChannel& ch);

    // Texture loading
    GLuint loadMaterialTexture(aiMaterial* mat, const std::string& dir);

    glm::mat4 aiToGlm(const aiMatrix4x4& m);
    glm::vec3 aiToGlm(const aiVector3D& v);
    glm::quat aiToGlm(const aiQuaternion& q);
};

IMPLEMENTATION REQUIREMENTS:
- Full working implementation, not skeleton
- Handle missing textures gracefully (use white 1x1 texture)
- Handle models with no animations (static draw still works)
- Max 100 bones (standard limit)
- Include: assimp/Importer.hpp, assimp/scene.h, assimp/postprocess.h

================================================================
CREATE FILE 2: src/core/SkinningShader.h
================================================================

Inline GLSL shader strings for skinned mesh rendering.

VERTEX SHADER must support:
- layout(location=0) in vec3 aPos
- layout(location=1) in vec3 aNormal  
- layout(location=2) in vec2 aTexCoord
- layout(location=3) in ivec4 aBoneIDs
- layout(location=4) in vec4  aWeights
- uniform mat4 boneTransforms[100]
- uniform mat4 model, view, projection
- Skinning math:
  mat4 skinMatrix = aWeights.x * boneTransforms[aBoneIDs.x]
                  + aWeights.y * boneTransforms[aBoneIDs.y]
                  + aWeights.z * boneTransforms[aBoneIDs.z]
                  + aWeights.w * boneTransforms[aBoneIDs.w];
  vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
- Output: FragPos, Normal, TexCoord, FogFactor (same as world shader)

FRAGMENT SHADER:
- Same Phong lighting as existing world.frag
- sampler2D diffuseTexture
- Fog (exponential)
- Output: FragColor

================================================================
UPDATE FILE 3: src/entities/Zombie.h
================================================================

Replace box-based zombie drawing with AnimatedModel.

Changes needed:
- Add: #include "../core/ModelLoader.h"
- Add to Zombie struct:
    int   animIndex;          // 0=walk, 1=run, 2=attack, 3=death
    float animTime;           // seconds elapsed in current animation
    bool  isDying;
    float deathTimer;

- Static shared models (load once, reuse for all zombies):
    static AnimatedModel* walkerModel;
    static AnimatedModel* runnerModel;  
    static AnimatedModel* tankModel;

- Static init function (call once at game start):
    static bool loadModels() {
        walkerModel = new AnimatedModel();
        walkerModel->load("assets/models/zombie_walk.fbx");
        // etc.
    }

- Update draw function:
    void draw(GLuint skinningShader) {
        // Choose model based on type
        AnimatedModel* mdl = (type==0) ? walkerModel :
                             (type==1) ? runnerModel : tankModel;
        
        // Set animation based on state
        animIndex = isDying ? 3 : (type==1 ? 1 : 0);
        
        // Update animation time
        animTime += deltaTime;
        mdl->update(animIndex, animTime);
        
        // Build model matrix (position + rotation + scale)
        glm::mat4 model = glm::translate(glm::mat4(1), pos);
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0,1,0));
        if (type==2) model = glm::scale(model, glm::vec3(1.8f));
        
        // Pass bone transforms to shader
        auto bones = mdl->getBoneTransforms();
        for (int i=0; i<bones.size(); i++) {
            std::string u = "boneTransforms[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(skinningShader, u.c_str()),
                               1, GL_FALSE, glm::value_ptr(bones[i]));
        }
        glUniformMatrix4fv(glGetUniformLocation(skinningShader,"model"),
                           1, GL_FALSE, glm::value_ptr(model));
        mdl->draw(skinningShader);
    }

================================================================
UPDATE FILE 4: src/entities/Player.h  
================================================================

Add player AnimatedModel:
- Load: "assets/models/player_walk.fbx" (contains walk+run+idle)
- Animation indices: 0=idle, 1=walk, 2=run
- Draw player model in third-person mode
- In first-person mode: draw only arms (clip to near plane)

================================================================
UPDATE FILE 5: src/Game.h
================================================================

- Add: GLuint skinningShader (compiled from SkinningShader.h)
- In init(): call Zombie::loadModels(), Player::loadModel()
- In render(): use skinningShader for zombies and player
- Keep existing world shader for environment (trees, ground, buildings)

================================================================
REQUIREMENTS:
================================================================
- All files complete, not partial
- Compile with: g++ main.cpp -o game.exe -I. -I../vendor -lglfw3 -lglew32 -lopengl32 -lglu32 -lopenal -lassimp -fopenmp -std=c++17
- Handle file-not-found gracefully: if FBX missing, fall back to box model
- No crashes if animation index out of range
- deltaTime must be passed to Zombie::update(float dt)
- Models scaled to match existing game units (1 unit ≈ 1 meter)
  Mixamo models are usually huge — scale down by 0.01

Return files in this order:
1. src/core/ModelLoader.h      (complete Assimp loader)
2. src/core/SkinningShader.h   (inline GLSL strings)
3. src/entities/Zombie.h       (updated with real models)
4. src/entities/Player.h       (updated with real model)
5. src/Game.h                  (updated integration)
```

================================================================
STEP 4: AFTER AI GIVES FILES — Compile karo
================================================================

MSYS2 terminal mein:

cd /d/zombies/zombie3d/src
g++ main.cpp -o ../build/game.exe \
  -I. -I../vendor \
  -lglfw3 -lglew32 -lopengl32 -lglu32 -lopenal -lassimp \
  -fopenmp -std=c++17

================================================================
STEP 5: COMMON ERRORS + FIXES
================================================================

ERROR: "assimp/Importer.hpp not found"
FIX:   pacman -S mingw-w64-x86_64-assimp

ERROR: "model too big / invisible"
FIX:   Mixamo models are 100x too big
       Add: model = glm::scale(model, glm::vec3(0.01f));

ERROR: "animation not playing / T-pose"
FIX:   Check animIndex is valid (< getAnimationCount())
       Make sure animTime increases each frame

ERROR: "texture missing / all white"
FIX:   Normal — white fallback texture is working correctly
       For real textures: export FBX with "Embed Textures" in Mixamo

ERROR: "linker error -lassimp"
FIX:   Add full path: -LC:/msys64/mingw64/lib -lassimp

================================================================
QUICK REFERENCE — Animation Indices (Mixamo FBX)
================================================================

zombie_walk.fbx   → animIndex 0 (walking)
zombie_run.fbx    → animIndex 0 (running — separate file)
zombie_attack.fbx → animIndex 0 (attacking)
zombie_death.fbx  → animIndex 0 (dying)

Note: Each Mixamo FBX has ONE animation (index 0)
Load separate FBX files for each animation, not one combined file
