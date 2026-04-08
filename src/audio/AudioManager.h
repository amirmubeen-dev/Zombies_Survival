#pragma once

#ifdef HAS_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <cmath>
#include <map>
#include <string>

// Fallback no-op macros for systems without OpenAL
#ifndef HAS_OPENAL
#define ALuint unsigned int
#define AL_NONE 0
#endif

class AudioManager {
private:
#ifdef HAS_OPENAL
    ALCdevice*  device  = nullptr;
    ALCcontext* context = nullptr;
#endif

    std::map<std::string, ALuint> buffers;
    std::vector<ALuint> sources;

    AudioManager() {}

public:
    static AudioManager& get() {
        static AudioManager instance;
        return instance;
    }

    AudioManager(const AudioManager&) = delete;
    void operator=(const AudioManager&) = delete;

    void init() {
#ifdef HAS_OPENAL
        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "[Audio] Failed to open OpenAL device. Audio disabled.\n";
            return;
        }
        context = alcCreateContext(device, nullptr);
        alcMakeContextCurrent(context);

        // Preallocate 32 source channels
        sources.resize(32);
        alGenSources(32, sources.data());
        
        // Generate procedural sounds
        generateProceduralSounds();
#else
        std::cerr << "[Audio] Compiled without OpenAL. Audio disabled.\n";
#endif
    }

    void setListenerPos(glm::vec3 pos, glm::vec3 front, glm::vec3 up) {
#ifdef HAS_OPENAL
        if (!device) return;
        alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
        float ori[6] = {front.x, front.y, front.z, up.x, up.y, up.z};
        alListenerfv(AL_ORIENTATION, ori);
#endif
    }

    void playSound(const std::string& name, glm::vec3 pos = {0,0,0}, float pitch = 1.0f, float gain = 1.0f) {
#ifdef HAS_OPENAL
        if (!device || buffers.find(name) == buffers.end()) return;

        // Find idle source
        ALuint src = 0;
        for (ALuint s : sources) {
            ALint state;
            alGetSourcei(s, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING) { src = s; break; }
        }
        if (src == 0) return; // All channels full

        alSourcei(src, AL_BUFFER, buffers[name]);
        alSource3f(src, AL_POSITION, pos.x, pos.y, pos.z);
        alSourcef(src, AL_PITCH, pitch);
        alSourcef(src, AL_GAIN, gain);
        // Distance model: 
        alSourcef(src, AL_REFERENCE_DISTANCE, 5.0f);
        alSourcef(src, AL_MAX_DISTANCE, 100.0f);
        alSourcef(src, AL_ROLLOFF_FACTOR, 1.5f);
        
        alSourcePlay(src);
#endif
    }

    void cleanup() {
#ifdef HAS_OPENAL
        if (!device) return;
        for (ALuint s : sources) alDeleteSources(1, &s);
        for (auto& pair : buffers) alDeleteBuffers(1, &pair.second);
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
#endif
    }

private:
    void generateProceduralSounds() {
#ifdef HAS_OPENAL
        // 1. Pistol (thoda sharper aur realistic crack)
buffers["pistol"] = createNoiseBurst(0.12f, 0.005f, 1.3f);

// 2. Shotgun (heavy boom feel)
buffers["shotgun"] = createNoiseBurst(0.45f, 0.02f, 0.5f);

// 3. Rifle (sharp + loud crack)
buffers["rifle"] = createNoiseBurst(0.18f, 0.003f, 1.5f);

// 4. Zombie Growl (zyada creepy + deep variation)
buffers["zombie_growl"] = createSawSweep(1.0f, 90.0f, 40.0f);

// 5. Footstep (soft + realistic thump)
buffers["footstep"] = createNoiseBurst(0.06f, 0.008f, 0.25f);

// 6. Ambient Jungle (zyada smooth aur natural)
buffers["ambience"] = createAmbientLoop(8.0f);

// 7. Death (existing functions se hi banaya 🔥)
#endif
    }

#ifdef HAS_OPENAL
    ALuint createNoiseBurst(float duration, float attack, float filterMult = 1.0f) {
        int sampleRate = 44100;
        int samples = (int)(duration * sampleRate);
        std::vector<short> data(samples);
        for (int i = 0; i < samples; i++) {
            float t = (float)i / sampleRate;
            float env = 1.0f;
            if (t < attack) env = t / attack;
            else env = expf(-5.0f * (t - attack) / (duration - attack));
            
            float noise = ((float)rand()/RAND_MAX * 2.0f - 1.0f);
            data[i] = (short)(noise * env * filterMult * 32767.0f);
        }
        ALuint buf; alGenBuffers(1, &buf);
        alBufferData(buf, AL_FORMAT_MONO16, data.data(), (ALsizei)(data.size() * sizeof(short)), sampleRate);
        return buf;
    }

    ALuint createSawSweep(float duration, float startFreq, float endFreq) {
        int sampleRate = 44100;
        int samples = (int)(duration * sampleRate);
        std::vector<short> data(samples);
        float phase = 0.0f;
        for (int i = 0; i < samples; i++) {
            float t = (float)i / sampleRate;
            float freq = startFreq + (endFreq - startFreq) * (t / duration);
            phase += freq / sampleRate;
            if (phase > 1.0f) phase -= 1.0f;
            
            float val = (phase * 2.0f - 1.0f);
            float env = sinf(t / duration * 3.14159f); // Smooth bowing env
            data[i] = (short)(val * env * 16383.0f);
        }
        ALuint buf; alGenBuffers(1, &buf);
        alBufferData(buf, AL_FORMAT_MONO16, data.data(), (ALsizei)(data.size() * sizeof(short)), sampleRate);
        return buf;
    }

    ALuint createAmbientLoop(float duration) {
        int sampleRate = 44100;
        int samples = (int)(duration * sampleRate);
        std::vector<short> data(samples);
        for (int i = 0; i < samples; i++) {
            float noise = ((float)rand()/RAND_MAX * 2.0f - 1.0f);
            // Heavy low-pass proxy
            static float last = 0;
            last = last * 0.9f + noise * 0.1f;
            data[i] = (short)(last * 8000.0f);
        }
        ALuint buf; alGenBuffers(1, &buf);
        alBufferData(buf, AL_FORMAT_MONO16, data.data(), (ALsizei)(data.size() * sizeof(short)), sampleRate);
        return buf;
    }
#endif
};
