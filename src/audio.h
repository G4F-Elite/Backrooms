#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include "audio_dsp.h"

const int SAMP_RATE=44100, BUF_COUNT=3, BUF_LEN=1024;

struct SoundState {
    float humPhase=0;
    float humVol=0.15f;
    float footPhase=0;
    bool stepTrig=false;
    float stepPitch=1.0f;
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
    float uiMovePitch=1.0f;
    float uiAdjustPitch=1.0f;
    float moveIntensity=0.0f;
    float sprintIntensity=0.0f;
    float lowStamina=0.0f;
    float monsterProximity=0.0f;
    float monsterMenace=0.0f;
    bool deathMode=false;
};

extern SoundState sndState;
extern std::atomic<bool> audioRunning;
extern HANDLE hEvent;
inline float carpetStep(float t) {
    float thud = sinf(t * 55.0f) * expf(-t * 35.0f) * 1.2f;
    float rustle = sinf(t * 30.0f) * expf(-t * 50.0f) * 0.4f;
    return (thud + rustle) * 0.5f;
}

inline float clamp01Audio(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
inline void updateGameplayAudioState(float moveIntensity, float sprintIntensity, float staminaNorm, float monsterProximity, float monsterMenace) {
    sndState.moveIntensity = clamp01Audio(moveIntensity);
    sndState.sprintIntensity = clamp01Audio(sprintIntensity);
    sndState.lowStamina = 1.0f - clamp01Audio(staminaNorm);
    sndState.monsterProximity = clamp01Audio(monsterProximity);
    sndState.monsterMenace = clamp01Audio(monsterMenace);
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
    static float breathPhase = 0.0f;
    static float runPhase = 0.0f;
    static float runNoise = 0.0f;
    static float monsterPhase = 0.0f;
    static float deathPhase = 0.0f;
    static float deathImpact = -1.0f;
    static bool lastDeathMode = false;
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
        
        // Distant sounds - deep groans and creaks (slow, low-frequency)
        float distant = 0;
        if(rand()%80000 < 2) sndState.distantPhase = 0.8f;
        if(sndState.distantPhase > 0) {
            // Low creaking groan that fades slowly
            float dFreq = 35.0f + sinf(sndState.distantPhase * 2.0f) * 8.0f;
            distant = sinf(sndState.distantPhase * dFreq) * expf(-sndState.distantPhase * 2.5f) * 0.18f;
            distant += sinf(sndState.distantPhase * dFreq * 0.5f) * expf(-sndState.distantPhase * 1.8f) * 0.08f;
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
            float baseGap = 5.5f - envStress * 1.5f;
            if(baseGap < 3.5f) baseGap = 3.5f;
            float jitter = (float)(rand()%1000) / 1000.0f;
            nextSceneEvent = sceneClock + baseGap + jitter * 5.0f;

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
                buzzPitch = 180.0f + (rand()%120);
            }else if(eventType == 5 && ringTime < 0.0f){
                ringTime = 0.0f;
                ringPitch = 140.0f + (rand()%80);
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

        // Buzz - low electrical hum/crackle, not a high-pitched screech
        float buzzTick = 0.0f;
        if(buzzTime >= 0.0f){
            float t = buzzTime;
            float atk = (t < 0.02f) ? (t / 0.02f) : 1.0f;
            float env = atk * expf(-t * 4.5f);
            float f = buzzPitch + sinf(t * 8.0f) * 15.0f;
            // Low rumbling buzz with subtle overtone
            buzzTick = (sinf(twoPi * f * t) * 0.35f + sinf(twoPi * (f * 2.0f) * t) * 0.08f) * env;
            // Add filtered noise texture for electrical feel
            float nTarget = ((rand()%100) / 100.0f) * 2.0f - 1.0f;
            static float buzzNoise = 0.0f;
            buzzNoise = mixNoise(buzzNoise, nTarget, 0.015f);
            buzzTick += buzzNoise * 0.06f * env;
            buzzTime += dt;
            if(buzzTime > 0.8f) buzzTime = -1.0f;
        }

        // Knock - distant muffled thuds, like something heavy behind walls
        float knock = 0.0f;
        if(knockTime >= 0.0f){
            float tapStart = 0.0f;
            if(knockStage == 1) tapStart = 0.18f;
            else if(knockStage == 2) tapStart = 0.40f;
            float localT = knockTime - tapStart;
            if(localT >= 0.0f && localT < 0.25f){
                float atk = (localT < 0.003f) ? (localT / 0.003f) : 1.0f;
                float env = atk * expf(-localT * 18.0f);
                // Low frequency thud - like distant door slam
                float kf = 55.0f + 8.0f * knockStage;
                knock += sinf(twoPi * kf * localT) * 0.5f * env;
                // Body resonance
                knock += sinf(twoPi * (kf * 1.7f) * localT) * 0.15f * expf(-localT * 25.0f);
            }
            if(knockStage == 0 && knockTime > 0.18f) knockStage = 1;
            if(knockStage == 1 && knockTime > 0.40f) knockStage = 2;
            knockTime += dt;
            if(knockTime > 0.75f) knockTime = -1.0f;
        }

        // Rustle - low scratching/shuffling, not high-pitched flutter
        float rustle = 0.0f;
        if(rustleTime >= 0.0f){
            float t = rustleTime;
            float atk = (t < 0.06f) ? (t / 0.06f) : 1.0f;
            float env = atk * expf(-t * 3.0f);
            float targetNoise = ((rand()%100) / 100.0f) * 2.0f - 1.0f;
            rustleNoise = mixNoise(rustleNoise, targetNoise, 0.04f);
            // Low scratchy sound instead of high-pitched flutter
            float scrape = sinf(twoPi * (80.0f + 20.0f * sinf(t * 3.0f)) * t) * 0.15f;
            rustle = (rustleNoise * 0.25f + scrape) * env;
            rustleTime += dt;
            if(rustleTime > 1.5f) rustleTime = -1.0f;
        }

        // Ring - deep metallic resonance, not a piercing tone
        float ring = 0.0f;
        if(ringTime >= 0.0f){
            float t = ringTime;
            float atk = (t < 0.08f) ? (t / 0.08f) : 1.0f;
            float env = atk * expf(-t * 1.8f);
            float f = ringPitch + sinf(t * 1.5f) * 5.0f;
            // Deep metallic drone with beating
            ring = (sinf(twoPi * f * t) * 0.3f + sinf(twoPi * (f * 1.005f) * t) * 0.25f) * env;
            // Sub harmonic for depth
            ring += sinf(twoPi * (f * 0.5f) * t) * 0.1f * env;
            ringTime += dt;
            if(ringTime > 2.0f) ringTime = -1.0f;
        }
        
        // Footsteps - louder
        float step=0;
        if(sndState.stepTrig) {
            float pitch = sndState.stepPitch;
            if(pitch < 0.7f) pitch = 0.7f;
            if(pitch > 1.45f) pitch = 1.45f;
            step = carpetStep(sndState.footPhase * pitch);
            step += sinf(twoPi * (90.0f + pitch * 40.0f) * sndState.footPhase) * 0.05f * expf(-sndState.footPhase * 25.0f);
            sndState.footPhase += 1.0f / SAMP_RATE;
            if(sndState.footPhase > 0.2f) { sndState.stepTrig=false; sndState.footPhase=0; }
        }
        
        // Flashlight switch - mechanical click sound (low, tactile)
        float flashlightSwitch = 0;
        static float lastFlash = 0;
        if(sndState.flashlightOn != lastFlash) {
            safe.flashlightEnv = 1.0f;
            lastFlash = sndState.flashlightOn;
        }
        if(safe.flashlightEnv > 0.0001f) {
            // Low mechanical click - not a tone
            float clickPhase = (1.0f - safe.flashlightEnv);
            float swf = 160.0f + clickPhase * 60.0f;
            flashlightSwitch = sinf(twoPi * swf * globalPhase) * 0.18f * safe.flashlightEnv;
            // Add a sharp transient for the "click" feel
            flashlightSwitch += sinf(twoPi * 90.0f * globalPhase) * 0.08f * safe.flashlightEnv * safe.flashlightEnv;
            safe.flashlightEnv *= 0.94f;
        }
        float flashlightLoop = 0.0f;
        if(sndState.flashlightOn > 0.5f){
            float fz = 108.0f + sndState.lowStamina * 28.0f;
            flashlightLoop = (sinf(twoPi * fz * globalPhase) * 0.018f +
                              sinf(twoPi * fz * 2.0f * globalPhase) * 0.006f);
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
        // UI Move - soft muted thud with low tone (not a high-pitched beep)
        float uiMove = 0.0f;
        if(uiMoveTime >= 0.0f){
            float t = uiMoveTime;
            float attack = (t < 0.006f) ? (t / 0.006f) : 1.0f;
            float env = attack * expf(-t * 18.0f);
            float pitch = sndState.uiMovePitch;
            if(pitch < 0.8f) pitch = 0.8f;
            if(pitch > 1.2f) pitch = 1.2f;
            // Low, soft tone - like a muffled click/tap
            float f1 = 220.0f * pitch;
            float f2 = 330.0f * pitch;
            uiMove = (sinf(twoPi * f1 * t) * 0.35f + sinf(twoPi * f2 * t) * 0.12f) * env;
            // Add a soft thud texture
            uiMove += sinf(twoPi * 85.0f * pitch * t) * expf(-t * 30.0f) * 0.15f;
            uiMoveTime += dt;
            if(uiMoveTime > 0.12f) uiMoveTime = -1.0f;
        }
        float uiConfirm = 0.0f;
        // UI Adjust - very subtle low tick
        float uiAdjust = 0.0f;
        if(uiAdjustTime >= 0.0f){
            float t = uiAdjustTime;
            float attack = (t < 0.004f) ? (t / 0.004f) : 1.0f;
            float env = attack * expf(-t * 22.0f);
            float pitch = sndState.uiAdjustPitch;
            if(pitch < 0.85f) pitch = 0.85f;
            if(pitch > 1.15f) pitch = 1.15f;
            // Soft, muted tick - like a physical switch
            float f1 = 260.0f * pitch;
            uiAdjust = sinf(twoPi * f1 * t) * 0.28f * env;
            uiAdjust += sinf(twoPi * 120.0f * pitch * t) * expf(-t * 35.0f) * 0.12f;
            uiAdjustTime += dt;
            if(uiAdjustTime > 0.08f) uiAdjustTime = -1.0f;
        }
        // UI Confirm - gentle low descending tone (not a harsh beep)
        if(uiConfirmTime >= 0.0f){
            float t = uiConfirmTime;
            float attack = (t < 0.008f) ? (t / 0.008f) : 1.0f;
            float env = attack * expf(-t * 10.0f);
            float lerpT = t / 0.25f;
            if(lerpT > 1.0f) lerpT = 1.0f;
            // Descending warm tone
            float f1 = 340.0f + (240.0f - 340.0f) * lerpT;
            float f2 = f1 * 1.5f;
            uiConfirm = (sinf(twoPi * f1 * t) * 0.30f + sinf(twoPi * f2 * t) * 0.08f) * env;
            // Add sub bass thud
            uiConfirm += sinf(twoPi * 65.0f * t) * expf(-t * 15.0f) * 0.18f;
            uiConfirmTime += dt;
            if(uiConfirmTime > 0.25f) uiConfirmTime = -1.0f;
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
        
        float roomEventsAmb = pipeTone * (0.10f + envStress * 0.12f)
                            + ventRumble * 0.14f
                            + rustle * (0.08f + envInsanity * 0.08f)
                            + ring * (0.07f + envStress * 0.06f);
        float roomEventsSfx = knock * (0.08f + envStress * 0.14f)
                            + buzzTick * 0.07f;

        float move = clamp01Audio(sndState.moveIntensity);
        float sprint = clamp01Audio(sndState.sprintIntensity);
        float lowSt = clamp01Audio(sndState.lowStamina);
        float monsterProx = clamp01Audio(sndState.monsterProximity);
        float monsterMenace = clamp01Audio(sndState.monsterMenace);

        float runBed = 0.0f;
        if(move > 0.02f){
            float cadence = 5.4f + sprint * 4.6f;
            runPhase += dt * cadence;
            if(runPhase > 1.0f) runPhase -= 1.0f;
            float nTarget = ((float)(rand()%100) / 100.0f) * 2.0f - 1.0f;
            runNoise = mixNoise(runNoise, nTarget, 0.035f);
            float gait = sinf(twoPi * runPhase);
            float gait2 = sinf(twoPi * runPhase * 2.0f);
            runBed = (gait * 0.06f + gait2 * 0.03f + runNoise * 0.025f) * move * (0.55f + sprint * 0.65f);
        }

        float breath = 0.0f;
        float breathIntensity = lowSt * 0.82f + sprint * 0.35f + monsterProx * 0.22f;
        if(breathIntensity > 0.04f){
            breathPhase += dt * (0.34f + breathIntensity * 0.95f);
            if(breathPhase > 1.0f) breathPhase -= 1.0f;
            float cyc = sinf(twoPi * breathPhase);
            float airy = sinf(twoPi * breathPhase * 2.0f) * 0.4f;
            float inhale = (cyc > 0.0f) ? cyc : 0.0f;
            float exhale = (cyc < 0.0f) ? -cyc : 0.0f;
            breath = (inhale * 0.06f + exhale * 0.09f + airy * 0.025f) * breathIntensity;
        }

        float monsterTone = 0.0f;
        if(!sndState.deathMode && monsterProx > 0.03f){
            float freq = 34.0f + monsterMenace * 18.0f;
            monsterPhase += dt * (0.5f + monsterProx * 1.9f);
            if(monsterPhase > 1.0f) monsterPhase -= 1.0f;
            float wob = 1.0f + sinf(twoPi * monsterPhase) * 0.08f;
            float baseGrowl = sinf(twoPi * (freq * wob) * globalPhase) * 0.085f;
            float overGrowl = sinf(twoPi * (freq * 2.06f) * globalPhase) * 0.025f;
            float pulse = 0.55f + 0.45f * sinf(twoPi * monsterPhase * 0.5f);
            monsterTone = (baseGrowl + overGrowl) * monsterProx * (0.5f + monsterMenace * 0.7f) * pulse;
        }

        float deathTone = 0.0f;
        if(sndState.deathMode){
            deathPhase += dt * 0.11f;
            if(deathPhase > 1.0f) deathPhase -= 1.0f;
            float d0 = sinf(twoPi * deathPhase * 44.0f) * 0.11f;
            float d1 = sinf(twoPi * deathPhase * 22.0f) * 0.18f;
            deathTone = d0 + d1;
            if(!lastDeathMode) deathImpact = 0.0f;
        }
        if(deathImpact >= 0.0f){
            float imp = expf(-deathImpact * 3.4f);
            deathTone += sinf(twoPi * 130.0f * deathImpact) * imp * 0.35f;
            deathImpact += dt;
            if(deathImpact > 1.2f) deathImpact = -1.0f;
        }
        lastDeathMode = sndState.deathMode;

        float ambienceMix = hum*sndState.humVol + amb + distant + roomEventsAmb;
        float sfxMix = step + runBed + scare + flashlightSwitch + flashlightLoop + roomEventsSfx;
        float uiMix = (uiMove + uiAdjust + uiConfirm) * (0.45f + 0.55f * sndState.sfxVol);
        float voiceMix = insane + breath;
        float musicMix = sndState.deathMode ? deathTone : (creepy + monsterTone);
        if(sndState.deathMode){
            ambienceMix *= 0.15f;
            sfxMix *= 0.12f;
            voiceMix *= 0.08f;
        }
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
inline void triggerMenuNavigateSound() {
    static int noteStep = 0;
    const float scale[6] = {1.0f, 1.059f, 1.122f, 1.189f, 1.122f, 1.059f};
    sndState.uiMovePitch = scale[noteStep % 6];
    noteStep++;
    sndState.uiMoveTrig = true;
}
inline void triggerMenuAdjustSound() {
    static int noteStep = 0;
    const float scale[5] = {0.944f, 1.0f, 1.059f, 1.122f, 1.189f};
    sndState.uiAdjustPitch = scale[noteStep % 5];
    noteStep++;
    sndState.uiAdjustTrig = true;
}
inline void triggerMenuConfirmSound() { sndState.uiConfirmTrig = true; }

void audioThread();
