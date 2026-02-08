#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include "audio_dsp.h"

const int SAMP_RATE=44100;
const int BUF_COUNT=3;
const int BUF_LEN=8192;

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
    for(int i=0;i<len;i++) {
        globalPhase += 1.0f / SAMP_RATE;
        
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
            safe.uiMoveEnv = 1.0f;
            sndState.uiMoveTrig = false;
        }
        if(sndState.uiConfirmTrig){
            safe.uiConfirmEnv = 1.0f;
            sndState.uiConfirmTrig = false;
        }
        float uiMove = 0.0f;
        if(safe.uiMoveEnv > 0.0001f){
            uiMove = sinf(globalPhase * 1900.0f) * safe.uiMoveEnv * 0.26f;
            uiMove += sinf(globalPhase * 2300.0f) * safe.uiMoveEnv * 0.08f;
            safe.uiMoveEnv *= 0.981f;
        }
        float uiConfirm = 0.0f;
        if(safe.uiConfirmEnv > 0.0001f){
            uiConfirm = sinf(globalPhase * 1300.0f) * safe.uiConfirmEnv * 0.34f;
            uiConfirm += sinf(globalPhase * 820.0f) * safe.uiConfirmEnv * 0.14f;
            safe.uiConfirmEnv *= 0.978f;
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
        float sfxMix = step + scare + flashClick + uiMove + uiConfirm;
        float voiceMix = insane;
        float musicMix = creepy;
        float v =
            (ambienceMix * sndState.ambienceVol +
             sfxMix * sndState.sfxVol +
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
