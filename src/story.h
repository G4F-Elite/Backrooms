#pragma once
#include <vector>
#include <random>
#include "math.h"

struct Note { Vec3 pos; int id; bool collected, active; float bobPhase; };
enum GamePhase { PHASE_INTRO, PHASE_EXPLORATION, PHASE_SURVIVAL, PHASE_DESPERATION };
enum HallucinationType { HALLUC_NONE, HALLUC_FLICKER, HALLUC_SHADOW, HALLUC_STEPS, HALLUC_WHISPER };
struct Hallucination { Vec3 pos; HallucinationType type; float timer, duration; bool active; };

const char* NOTE_TITLES[] = {"Contract Brief","Signal Noise","Contact Log","Clock Drift","Crew Fragment","Hunter Pattern","Crawler Route","Shadow Zone","System Distort","Blocked Route","Archive Trace","Last Uplink"};
const char* NOTE_CONTENTS[] = {
    "Shift team entered Level 1.\nObjective: stabilize 3 nodes\nand hold the grid.",
    "Fluorescent hum maps to\nresonance drift. Use scanner\nonly when route is clear.",
    "Hostile silhouette tracked\nfrom maintenance branch.\nNoise attracts it fast.",
    "Clock drift confirmed.\nArchive timer is more stable\nthan local watches.",
    "Found old crew badge near\nservice vent. No body, no\ntransponder response.",
    "Hunter tracks movement\nbreaks. Close doors, then\nrotate routes.",
    "Crawler path loops around\nwet corridor. Use light burst\nif grab starts.",
    "Shadow zone expands at\nhigh ATTENTION. Keep team\nspread and pings short.",
    "Echo playback desync seen\nat critical threat levels.\nDo not spam replay.",
    "Parking lift blocked until\nbattery, fuses, and access\nline are confirmed.",
    "Archive traces imply prior\nshift attempts reached this\nphase and failed extraction.",
    "If this log remains: finish\nthe contract cycle, return to\nBreakroom, report upstream."
};
const char* INTRO_LINES[] = {"ARCHIVE UPLINK ONLINE.","SHIFT TEAM STATUS: DEPLOYING.","PRIMARY TASK: STABILIZE REALITY.","CONTRACT FEED RECEIVED.","LEVEL 1: YELLOW ROOMS.","LEVEL 2: PARKING.","USE RESONATOR TO TRACK NODES.","RECORD ECHO IF SOLO.","MANAGE ATTENTION AND LIGHT.","EXTRACT AFTER CONTRACT COMPLETE.","","VOID SHIFT v1.0.0"};
const int INTRO_LINE_COUNT = 12;

class StoryManager {
public:
    std::vector<Note> notes; std::vector<Hallucination> hallucinations;
    bool notesCollected[12] = {false}; int totalCollected = 0;
    bool introComplete = false, readingNote = false; int introLine = 0, currentNote = -1;
    float introTimer = 0, introLineTime = 2.5f, hallucinationTimer = 0, nextHallucination = 30.0f;
    GamePhase currentPhase = PHASE_INTRO;
    
    void init() { notes.clear(); hallucinations.clear(); for(int i=0;i<12;i++)notesCollected[i]=false; totalCollected=0;
        introComplete=false; introLine=0; introTimer=0; readingNote=false; currentNote=-1; currentPhase=PHASE_INTRO; hallucinationTimer=0; }
    void spawnNote(Vec3 pos, int id) { if(id>=12||notesCollected[id])return; Note n; n.pos=pos; n.id=id; n.collected=false; n.active=true; n.bobPhase=(float)(rand()%100)/10.0f; notes.push_back(n); }
    void update(float dt, float survivalTime, float sanity, std::mt19937& rng) {
        if(!introComplete) { introTimer+=dt; if(introTimer>=introLineTime) { introTimer=0; introLine++; if(introLine>=INTRO_LINE_COUNT)introComplete=true; }}
        if(survivalTime<60.0f)currentPhase=PHASE_INTRO; else if(survivalTime<180.0f)currentPhase=PHASE_EXPLORATION; else if(survivalTime<300.0f)currentPhase=PHASE_SURVIVAL; else currentPhase=PHASE_DESPERATION;
        for(auto&n:notes) { if(!n.active||n.collected)continue; n.bobPhase+=dt*2.0f; }
        if(sanity<50.0f) { hallucinationTimer+=dt; float spawnChance=(50.0f-sanity)/50.0f; nextHallucination=10.0f+20.0f*(1.0f-spawnChance);
            if(hallucinationTimer>=nextHallucination) { hallucinationTimer=0; if(rng()%100<(int)(spawnChance*60)) { Hallucination h; h.type=(HallucinationType)(1+rng()%4); h.timer=0; h.active=true; h.duration=0.5f+(rng()%100)/100.0f; hallucinations.push_back(h); }}}
        for(auto&h:hallucinations) { if(!h.active)continue; h.timer+=dt; if(h.timer>=h.duration)h.active=false; }
        hallucinations.erase(std::remove_if(hallucinations.begin(),hallucinations.end(),[](const Hallucination&h){return!h.active;}),hallucinations.end());
    }
    bool checkNotePickup(Vec3 pPos, float range=2.0f) { for(auto&n:notes) { if(!n.active||n.collected)continue; Vec3 d=n.pos-pPos; d.y=0;
        if(sqrtf(d.x*d.x+d.z*d.z)<range) { n.collected=true; notesCollected[n.id]=true; totalCollected++; currentNote=n.id; readingNote=true; return true; }} return false; }
    void closeNote() { readingNote=false; currentNote=-1; }
    GamePhase getPhase() const { return currentPhase; }
    bool isInIntro() const { return !introComplete; }
    bool hasHallucinations() const { return !hallucinations.empty(); }
    float getSpawnDelay() const { return currentPhase==PHASE_INTRO?999.0f:(currentPhase==PHASE_EXPLORATION?45.0f:(currentPhase==PHASE_SURVIVAL?30.0f:20.0f)); }
    int getMaxEntities() const { return currentPhase==PHASE_INTRO?0:(currentPhase==PHASE_EXPLORATION?1:(currentPhase==PHASE_SURVIVAL?2:3)); }
};
inline StoryManager storyMgr;
