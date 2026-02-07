#pragma once
#include <vector>
#include <random>
#include "math.h"

struct Note { Vec3 pos; int id; bool collected, active; float bobPhase; };
enum GamePhase { PHASE_INTRO, PHASE_EXPLORATION, PHASE_SURVIVAL, PHASE_DESPERATION };
enum HallucinationType { HALLUC_NONE, HALLUC_FLICKER, HALLUC_SHADOW, HALLUC_STEPS, HALLUC_WHISPER };
struct Hallucination { Vec3 pos; HallucinationType type; float timer, duration; bool active; };

const char* NOTE_TITLES[] = {"First Entry","The Hum","Sighting","Time","They Were People","The Stalker","The Crawler","The Shadow","Transformation","No Escape","The Truth","Final Entry"};
const char* NOTE_CONTENTS[] = {
    "Day 1 (I think?)\n\nI don't know how I got here.\nYellow walls everywhere.\nI need to find a way out.",
    "The buzzing never stops.\n60Hz. My head hurts.\nI can't remember...",
    "I SAW SOMETHING.\nTall. Wrong. When I looked\nit was gone.\nAm I losing my mind?",
    "Day 7? My watch stopped.\nThe walls moved.\nI KNOW they moved.",
    "I found a wallet.\nMichael Torres. Accountant.\nWhere is Michael now?",
    "It follows when I'm not\nlooking. But when I turn\naround - nothing.",
    "FAST. SO FAST.\nDon't run. It hears you.\nDon't run don't run",
    "You can't see it if you\nlook. Trust your peripheral.\nIt was someone once.",
    "I saw one... change.\nTheir body... bent. Wrong.\nWe become THEM.",
    "There is no exit.\nThe Backrooms go on forever.\nSurviving IS the point.",
    "This place is made of\nforgotten spaces. We're\ninside memory. OUR memory.",
    "Don't give up. Stay human.\nAs long as you can.\n\n...what was my name?"
};
const char* INTRO_LINES[] = {"It was just another late night...","I walked to get coffee.","The light was flickering.","I blinked...","...the hallway was longer.","The door wasn't there.","When I turned around,","neither was my office.","Just... yellow.","Endless yellow.","","LEVEL 0 - THE LOBBY"};
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