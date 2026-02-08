#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include "audio_dsp.h"

const int SAMP_RATE=44100;
const int BUF_COUNT=3;
// Smaller audio blocks reduce menu SFX input latency.
const int BUF_LEN=1024;

struct SoundState {
    float humPhase=0;
    float humVol=0.15f;
    float footPhase=0;
    bool stepTrig=false;
    float ambPhase=0;
    float masterVol=0.7f;
    float dangerLevel=0;
    float sanityLevel=1.0f;
    float creepyPhase=0;
    float staticPhase=0;
    float whisperPhase=0;
    float flashlightOn=0;
    float distantPhase=0;
    float heartPhase=0;
    float scareTimer=0;
    float scareVol=0;
    float musicVol=0.55f;
    float ambienceVol=0.75f;
    float sfxVol=0.7f;
    float voiceVol=0.65f;
    bool uiMoveTrig=false;
    bool uiConfirmTrig=false;
};

extern SoundState sndState;
extern std::atomic<bool> audioRunning;
extern HANDLE hEvent;

inline float carpetStep(float t) {
    float thud = sinf(t * 55.0f) * expf(-t * 35.0f) * 1.2f;
    float rustle = sinf(t * 30.0f) * expf(-t * 50.0f) * 0.4f;
    return (thud + rustle) * 0.5f;
}

inline void fillAudio(short* buf, int len) {
    static float globalPhase = 0;
    static AudioSafetyState safe;
    static float uiMoveTime = -1.0f;
    static float uiConfirmTime = -1.0f;
    const float dt = 1.0f / (float)SAMP_RATE;
    const float twoPi = 6.283185307f;
    for(int i=0;i<len;i++) {
        globalPhase += dt;
        
        // Fluorescent hum - louder base
        float hum = sinf(sndState.humPhase)*0.25f;
        hum += sinf(sndState.humPhase*2.0f)*0.15f;
        hum += sinf(sndState.humPhase*3.0f)*0.08f;
        float humTargetNoise = (float)(rand()%100-50)/100.0f * 0.02f;
        safe.humNoise = mixNoise(safe.humNoise, humTargetNoise, 0.03f);
        hum += safe.humNoise;
        sndState.humPhase += 0.0085f;
        if(sndState.humPhase>6.28318f) sndState.humPhase-=6.28318f;
        
        // Ambient low drone - always present
        float amb = sinf(sndState.ambPhase)*0.04f;
        amb += sinf(sndState.ambPhase*0.7f)*0.02f;
        sndState.ambPhase += 0.00025f;
        if(sndState.ambPhase>6.28318f) sndState.ambPhase-=6.28318f;
        
        // Distant sounds - random creaks and groans
        float distant = 0;
        if(rand()%50000 < 3) sndState.distantPhase = 0.5f;
        if(sndState.distantPhase > 0) {
            distant = sinf(sndState.distantPhase * 80.0f) * expf(-sndState.distantPhase * 4.0f) * 0.3f;
            sndState.distantPhase -= 1.0f / SAMP_RATE;
        }
        
        // Footsteps - louder
        float step=0;
        if(sndState.stepTrig) {
            step = carpetStep(sndState.footPhase);
            sndState.footPhase += 1.0f / SAMP_RATE;
            if(sndState.footPhase > 0.2f) { sndState.stepTrig=false; sndState.footPhase=0; }
        }
        
        // Flashlight click
        float flashClick = 0;
        static float lastFlash = 0;
        if(sndState.flashlightOn != lastFlash) {
            safe.clickEnv = 1.0f;
            lastFlash = sndState.flashlightOn;
        }
        if(safe.clickEnv > 0.0001f) {
            flashClick = safe.clickEnv * 0.25f;
            safe.clickEnv *= 0.992f;
        }

        // Menu UI sounds
        if(sndState.uiMoveTrig){
            uiMoveTime = 0.0f;
            sndState.uiMoveTrig = false;
        }
        if(sndState.uiConfirmTrig){
            uiConfirmTime = 0.0f;
            sndState.uiConfirmTrig = false;
        }
        float uiMove = 0.0f;
        if(uiMoveTime >= 0.0f){
            float t = uiMoveTime;
            float attack = (t < 0.012f) ? (t / 0.012f) : 1.0f;
            float env = attack * expf(-t * 12.0f);
            float wob = 1.0f + 0.015f * sinf(t * 30.0f);
            float f1 = 590.0f * wob;
            float f2 = 880.0f;
            uiMove = (sinf(twoPi * f1 * t) * 0.55f + sinf(twoPi * f2 * t) * 0.25f) * env;
            uiMoveTime += dt;
            if(uiMoveTime > 0.22f) uiMoveTime = -1.0f;
        }
        float uiConfirm = 0.0f;
        if(uiConfirmTime >= 0.0f){
            float t = uiConfirmTime;
            float attack = (t < 0.010f) ? (t / 0.010f) : 1.0f;
            float env = attack * expf(-t * 8.2f);
            float lerpT = t / 0.32f;
            if(lerpT > 1.0f) lerpT = 1.0f;
            float f1 = 760.0f + (520.0f - 760.0f) * lerpT;
            float f2 = f1 * 1.5f;
            uiConfirm = (sinf(twoPi * f1 * t) * 0.75f + sinf(twoPi * f2 * t) * 0.22f) * env;
            uiConfirmTime += dt;
            if(uiConfirmTime > 0.32f) uiConfirmTime = -1.0f;
        }
        
        // Danger sounds - progressive
        float creepy = 0;
        if(sndState.dangerLevel > 0.05f) {
            // Low rumble
            creepy += sinf(sndState.creepyPhase * 0.4f) * 0.2f * sndState.dangerLevel;
            // Dissonant tone
            creepy += sinf(sndState.creepyPhase * 2.1f) * 0.1f * sndState.dangerLevel;
            // Static/noise
            float staticTarget = (float)(rand()%100-50)/100.0f * 0.15f * sndState.dangerLevel;
            safe.staticNoise = mixNoise(safe.staticNoise, staticTarget, 0.06f);
            creepy += safe.staticNoise;
            
            // Heartbeat at high danger
            if(sndState.dangerLevel > 0.5f) {
                sndState.heartPhase += 0.00015f * (1.0f + sndState.dangerLevel);
                if(sndState.heartPhase > 6.28f) sndState.heartPhase -= 6.28f;
                float hb = sinf(sndState.heartPhase);
                if(hb > 0.8f) creepy += 0.35f * sndState.dangerLevel;
            }
            
            sndState.creepyPhase += 0.002f;
            if(sndState.creepyPhase > 6.28318f) sndState.creepyPhase -= 6.28318f;
        }
        
        // Jump scare sound
        float scare = 0;
        if(sndState.scareVol > 0) {
            scare = sinf(sndState.scareTimer * 200.0f) * sndState.scareVol;
            scare += safe.staticNoise * sndState.scareVol * 0.35f;
            sndState.scareTimer += 1.0f / SAMP_RATE;
            sndState.scareVol *= 0.9998f;
            if(sndState.scareVol < 0.01f) sndState.scareVol = 0;
        }
        
        // Insanity sounds
        float insane = 0;
        float insanity = 1.0f - sndState.sanityLevel;
        if(insanity > 0.2f) {
            float whisper = sinf(sndState.whisperPhase * 15.0f) * sinf(sndState.whisperPhase * 0.5f);
            float whisperTarget = (float)(rand()%100)/100.0f * 0.12f * insanity;
            safe.whisperNoise = mixNoise(safe.whisperNoise, whisperTarget, 0.05f);
            whisper *= safe.whisperNoise;
            insane += whisper;
            insane += sinf(sndState.whisperPhase * 9.5f) * 0.05f * insanity;
            if(rand()%800 < (int)(insanity * 12)) {
                safe.insaneBurst = ((float)(rand()%100-50)/100.0f) * 0.18f;
            }
            safe.insaneBurst *= 0.993f;
            insane += safe.insaneBurst;
            sndState.whisperPhase += 0.0012f + insanity * 0.0015f;
            if(sndState.whisperPhase > 6.28318f) sndState.whisperPhase -= 6.28318f;
        }
        
        float ambienceMix = hum*sndState.humVol + amb + distant;
        float sfxMix = step + scare + flashClick;
        float uiMix = (uiMove + uiConfirm) * (0.45f + 0.55f * sndState.sfxVol);
        float voiceMix = insane;
        float musicMix = creepy;
        float v =
            (ambienceMix * sndState.ambienceVol +
             sfxMix * sndState.sfxVol +
             uiMix +
             voiceMix * sndState.voiceVol +
             musicMix * sndState.musicVol) * sndState.masterVol;
        v = applyLimiter(v, safe.limiterEnv);
        v = softClip(v);
        if(v>1.0f) v=1.0f; if(v<-1.0f) v=-1.0f;
        buf[i]=(short)(v*32767);
    }
}

inline void triggerScare() { sndState.scareVol = 0.8f; sndState.scareTimer = 0; }
inline void triggerMenuNavigateSound() { sndState.uiMoveTrig = true; }
inline void triggerMenuConfirmSound() { sndState.uiConfirmTrig = true; }

void audioThread();
