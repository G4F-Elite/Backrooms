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
    bool uiAdjustTrig=false;
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
    static float uiAdjustTime = -1.0f;
    static float uiConfirmTime = -1.0f;
    static float pipeTime = -1.0f, pipePitch = 92.0f, pipeDrift = 0.0f;
    static float ventTime = -1.0f, ventPitch = 36.0f, ventNoise = 0.0f;
    static float buzzTime = -1.0f, buzzPitch = 1220.0f;
    static float knockTime = -1.0f;
    static int knockStage = 0;
    static float rustleTime = -1.0f, rustleNoise = 0.0f;
    static float ringTime = -1.0f, ringPitch = 410.0f;
    static float sceneClock = 0.0f;
    static float nextSceneEvent = 2.8f;
    static int lastSceneEvent = -1;
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

        // Soft procedural spot SFX (no hard peaks)
        float envDanger = sndState.dangerLevel;
        if(envDanger < 0.0f) envDanger = 0.0f;
        if(envDanger > 1.0f) envDanger = 1.0f;
        float envInsanity = 1.0f - sndState.sanityLevel;
        if(envInsanity < 0.0f) envInsanity = 0.0f;
        if(envInsanity > 1.0f) envInsanity = 1.0f;
        float envStress = envDanger * 0.65f + envInsanity * 0.35f;
        sceneClock += dt;
        if(sceneClock >= nextSceneEvent){
            float baseGap = 3.6f - envStress * 1.1f;
            if(baseGap < 2.1f) baseGap = 2.1f;
            float jitter = (float)(rand()%1000) / 1000.0f;
            nextSceneEvent = sceneClock + baseGap + jitter * 3.4f;

            int eventRoll = rand()%100;
            int eventType = 0;
            if(eventRoll < 24) eventType = 0;         // pipe
            else if(eventRoll < 45) eventType = 1;    // vent
            else if(eventRoll < 60) eventType = 2;    // rustle
            else if(eventRoll < 75) eventType = 3;    // knock
            else if(eventRoll < 89) eventType = 4;    // buzz
            else eventType = 5;                       // ring
            if(eventType == lastSceneEvent) eventType = (eventType + 1 + (rand()%2)) % 6;
            lastSceneEvent = eventType;

            if(eventType == 0 && pipeTime < 0.0f){
                pipeTime = 0.0f;
                pipePitch = 78.0f + (rand()%42);
                pipeDrift = ((rand()%100) / 100.0f) * 1.6f + 0.3f;
            }else if(eventType == 1 && ventTime < 0.0f){
                ventTime = 0.0f;
                ventPitch = 28.0f + (rand()%18);
            }else if(eventType == 2 && rustleTime < 0.0f){
                rustleTime = 0.0f;
            }else if(eventType == 3 && knockTime < 0.0f){
                knockTime = 0.0f;
                knockStage = 0;
            }else if(eventType == 4 && buzzTime < 0.0f){
                buzzTime = 0.0f;
                buzzPitch = 960.0f + (rand()%560);
            }else if(eventType == 5 && ringTime < 0.0f){
                ringTime = 0.0f;
                ringPitch = 360.0f + (rand()%160);
            }
        }

        float pipeTone = 0.0f;
        if(pipeTime >= 0.0f){
            float t = pipeTime;
            float atk = (t < 0.08f) ? (t / 0.08f) : 1.0f;
            float env = atk * expf(-t * 1.85f);
            float freq = pipePitch + sinf(t * 1.9f) * pipeDrift;
            pipeTone = (sinf(twoPi * freq * t) * 0.62f + sinf(twoPi * (freq * 1.97f) * t) * 0.21f) * env;
            pipeTime += dt;
            if(pipeTime > 1.9f) pipeTime = -1.0f;
        }

        float ventRumble = 0.0f;
        if(ventTime >= 0.0f){
            float t = ventTime;
            float atk = (t < 0.10f) ? (t / 0.10f) : 1.0f;
            float env = atk * expf(-t * 1.1f);
            float targetNoise = ((rand()%100) / 100.0f) * 2.0f - 1.0f;
            ventNoise = mixNoise(ventNoise, targetNoise, 0.02f);
            float base = sinf(twoPi * ventPitch * t) * 0.55f + sinf(twoPi * (ventPitch * 0.52f) * t) * 0.25f;
            ventRumble = (base + ventNoise * 0.28f) * env;
            ventTime += dt;
            if(ventTime > 2.2f) ventTime = -1.0f;
        }

        float buzzTick = 0.0f;
        if(buzzTime >= 0.0f){
            float t = buzzTime;
            float atk = (t < 0.004f) ? (t / 0.004f) : 1.0f;
            float env = atk * expf(-t * 9.5f);
            float f = buzzPitch + sinf(t * 42.0f) * 120.0f;
            buzzTick = (sinf(twoPi * f * t) * 0.55f + sinf(twoPi * (f * 1.5f) * t) * 0.16f) * env;
            buzzTime += dt;
            if(buzzTime > 0.35f) buzzTime = -1.0f;
        }

        float knock = 0.0f;
        if(knockTime >= 0.0f){
            float tapStart = 0.0f;
            if(knockStage == 1) tapStart = 0.13f;
            else if(knockStage == 2) tapStart = 0.29f;
            float localT = knockTime - tapStart;
            if(localT >= 0.0f && localT < 0.22f){
                float atk = (localT < 0.01f) ? (localT / 0.01f) : 1.0f;
                float env = atk * expf(-localT * 24.0f);
                float kf = 145.0f + 18.0f * knockStage;
                knock += (sinf(twoPi * kf * localT) * 0.85f + sinf(twoPi * (kf * 2.2f) * localT) * 0.2f) * env;
            }
            if(knockStage == 0 && knockTime > 0.13f) knockStage = 1;
            if(knockStage == 1 && knockTime > 0.29f) knockStage = 2;
            knockTime += dt;
            if(knockTime > 0.62f) knockTime = -1.0f;
        }

        float rustle = 0.0f;
        if(rustleTime >= 0.0f){
            float t = rustleTime;
            float atk = (t < 0.04f) ? (t / 0.04f) : 1.0f;
            float env = atk * expf(-t * 4.3f);
            float targetNoise = ((rand()%100) / 100.0f) * 2.0f - 1.0f;
            rustleNoise = mixNoise(rustleNoise, targetNoise, 0.08f);
            float flutter = sinf(twoPi * (540.0f + 40.0f * sinf(t * 5.0f)) * t) * 0.2f;
            rustle = (rustleNoise * 0.5f + flutter) * env;
            rustleTime += dt;
            if(rustleTime > 1.05f) rustleTime = -1.0f;
        }

        float ring = 0.0f;
        if(ringTime >= 0.0f){
            float t = ringTime;
            float atk = (t < 0.05f) ? (t / 0.05f) : 1.0f;
            float env = atk * expf(-t * 2.6f);
            float f = ringPitch + sinf(t * 2.2f) * 8.0f;
            ring = (sinf(twoPi * f * t) * 0.55f + sinf(twoPi * (f * 1.01f) * t) * 0.45f) * env;
            ringTime += dt;
            if(ringTime > 1.35f) ringTime = -1.0f;
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
        if(sndState.uiAdjustTrig){
            uiAdjustTime = 0.0f;
            sndState.uiAdjustTrig = false;
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
        float uiAdjust = 0.0f;
        if(uiAdjustTime >= 0.0f){
            float t = uiAdjustTime;
            float attack = (t < 0.008f) ? (t / 0.008f) : 1.0f;
            float env = attack * expf(-t * 13.5f);
            float f1 = 690.0f + sinf(t * 34.0f) * 14.0f;
            float f2 = 1030.0f;
            uiAdjust = (sinf(twoPi * f1 * t) * 0.62f + sinf(twoPi * f2 * t) * 0.16f) * env;
            uiAdjustTime += dt;
            if(uiAdjustTime > 0.15f) uiAdjustTime = -1.0f;
        }
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
            // Soft static texture (smoothed to avoid sharp clicks)
            float staticTarget = (float)(rand()%100-50)/100.0f * 0.06f * sndState.dangerLevel;
            safe.staticNoise = mixNoise(safe.staticNoise, staticTarget, 0.025f);
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
        
        float roomEventsAmb = pipeTone * (0.18f + envStress * 0.24f)
                            + ventRumble * 0.22f
                            + rustle * (0.12f + envInsanity * 0.12f)
                            + ring * (0.10f + envStress * 0.10f);
        float roomEventsSfx = knock * (0.10f + envStress * 0.26f)
                            + buzzTick * 0.11f;

        float ambienceMix = hum*sndState.humVol + amb + distant + roomEventsAmb;
        float sfxMix = step + scare + flashClick + roomEventsSfx;
        float uiMix = (uiMove + uiAdjust + uiConfirm) * (0.45f + 0.55f * sndState.sfxVol);
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
inline void triggerMenuAdjustSound() { sndState.uiAdjustTrig = true; }
inline void triggerMenuConfirmSound() { sndState.uiConfirmTrig = true; }

void audioThread();
