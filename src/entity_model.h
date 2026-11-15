#ifndef ENTITY_MODEL_H
#define ENTITY_MODEL_H

#include <vector>
#include <cmath>
#include "entity_types.h"
#include "creature_mesh.h"
#include "creature_texture.h"

// Entities are quiet, faceted, low-poly figures: recognisably human, proportioned wrong.
// No glowing eyes, teeth or horns - they should read as something that belongs in an empty office.
// All models stand on y = 0 and face +Z.

// E-01 Stalker - tall gaunt office worker, arms too long, hunched, head hanging to one side.
inline void buildStalkerModel(CreatureMesh& m) {
    m = CreatureMesh();
    m.strips = 4;
    m.vScale = 1.0f;
    const int SKIN = 0, SHIRT = 1, SLACKS = 2, DARK = 3;

    // Pelvis
    int root = m.beginPart(CJ_ROOT, -1, Vec3(0, 1.12f, 0));
    m.strip = SLACKS;
    CreatureRing pelvis[3] = {
        {Vec3(0, 1.00f, 0.00f), 0.150f, 0.100f},
        {Vec3(0, 1.14f, 0.00f), 0.165f, 0.105f},
        {Vec3(0, 1.36f, 0.01f), 0.135f, 0.090f}
    };
    m.tube(pelvis, 3, 8);

    // Hunched upper body
    int spine = m.beginPart(CJ_SPINE, root, Vec3(0, 1.34f, 0.01f));
    m.strip = SHIRT;
    CreatureRing chest[5] = {
        {Vec3(0, 1.32f, 0.01f), 0.137f, 0.092f},
        {Vec3(0, 1.52f, 0.04f), 0.160f, 0.105f},
        {Vec3(0, 1.72f, 0.09f), 0.190f, 0.115f},
        {Vec3(0, 1.85f, 0.13f), 0.200f, 0.100f},
        {Vec3(0, 1.93f, 0.16f), 0.085f, 0.070f}
    };
    m.tube(chest, 5, 8);
    // Loosened tie hanging plumb off the bent chest
    m.strip = DARK;
    CreatureRing tie[3] = {
        {Vec3(0, 1.89f, 0.232f), 0.022f, 0.010f},
        {Vec3(0, 1.62f, 0.240f), 0.032f, 0.008f},
        {Vec3(0, 1.46f, 0.243f), 0.006f, 0.004f}
    };
    m.tube(tie, 3, 4);
    // Neck, a little too long
    m.strip = SKIN;
    m.limb(Vec3(0, 1.91f, 0.155f), Vec3(0, 2.11f, 0.245f), 0.050f, 0.043f, 6);

    // Head - smooth blank oval with two shallow hollows
    m.beginPart(CJ_HEAD, spine, Vec3(0, 2.09f, 0.24f));
    m.strip = SKIN;
    m.ovoid(Vec3(0, 2.05f, 0.245f), Vec3(0, 2.36f, 0.295f), 0.098f, 0.112f, 8);
    // Small, too far apart, slightly uneven
    m.strip = DARK;
    m.ovoid(Vec3(-0.047f, 2.222f, 0.366f), Vec3(-0.047f, 2.242f, 0.370f), 0.011f, 0.010f, 5);
    m.ovoid(Vec3(0.049f, 2.214f, 0.364f), Vec3(0.049f, 2.236f, 0.368f), 0.012f, 0.010f, 5);

    // Arms reach down to the knees
    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 shoulder(fs * 0.215f, 1.80f, 0.115f);
        Vec3 elbow(fs * 0.270f, 1.29f, 0.070f);
        Vec3 wrist(fs * 0.265f, 0.82f, 0.170f);
        int arm = m.beginPart(s < 0 ? CJ_ARM_L : CJ_ARM_R, spine, shoulder);
        m.strip = SHIRT;
        m.limb(shoulder, elbow, 0.056f, 0.044f, 6);
        m.beginPart(s < 0 ? CJ_FOREARM_L : CJ_FOREARM_R, arm, elbow);
        m.strip = SKIN;
        m.limb(elbow, wrist, 0.040f, 0.029f, 6);
        // Flat palm and long fingers
        Vec3 knuckle(fs * 0.262f, 0.70f, 0.190f);
        CreatureRing palm[2] = { {wrist, 0.034f, 0.014f}, {knuckle, 0.040f, 0.012f} };
        m.tube(palm, 2, 4, true, true, Vec3(0, 0, 1));
        for (int f = 0; f < 4; f++) {
            float off = ((float)f - 1.5f) * 0.021f;
            float len = 0.20f - fabsf((float)f - 1.3f) * 0.022f;
            Vec3 base(knuckle.x, knuckle.y, knuckle.z + off);
            Vec3 tip(knuckle.x - fs * 0.006f, knuckle.y - len, knuckle.z + off * 1.5f + 0.015f);
            m.limb(base, tip, 0.0095f, 0.005f, 4);
        }
    }

    // Legs - the right knee sits slightly wrong
    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 hip(fs * 0.085f, 1.10f, 0.0f);
        Vec3 knee = s < 0 ? Vec3(-0.100f, 0.60f, 0.050f) : Vec3(0.070f, 0.59f, 0.075f);
        Vec3 ankle(fs * 0.095f, 0.085f, -0.020f);
        int leg = m.beginPart(s < 0 ? CJ_LEG_L : CJ_LEG_R, root, hip);
        m.strip = SLACKS;
        m.limb(hip, knee, 0.076f, 0.055f, 6);
        m.beginPart(s < 0 ? CJ_SHIN_L : CJ_SHIN_R, leg, knee);
        m.limb(knee, ankle, 0.052f, 0.038f, 6);
        m.strip = DARK;
        CreatureRing shoe[3] = {
            {Vec3(ankle.x, 0.045f, -0.085f), 0.040f, 0.040f},
            {Vec3(ankle.x, 0.042f, 0.050f), 0.047f, 0.038f},
            {Vec3(ankle.x, 0.030f, 0.185f), 0.040f, 0.022f}
        };
        m.tube(shoe, 3, 5);
    }
    m.finish();
}

// E-02 Crawler - a person folded down onto all fours, joints healed above the back.
inline void buildCrawlerModel(CreatureMesh& m) {
    m = CreatureMesh();
    m.strips = 4;
    m.vScale = 1.0f;
    const int SKIN = 0, GRIME = 1, RAG = 2, DARK = 3;

    int root = m.beginPart(CJ_ROOT, -1, Vec3(0, 0.30f, -0.10f));
    m.strip = SKIN;
    CreatureRing body[6] = {
        {Vec3(0, 0.300f, -0.52f), 0.100f, 0.065f},
        {Vec3(0, 0.330f, -0.38f), 0.150f, 0.090f},
        {Vec3(0, 0.250f, -0.12f), 0.125f, 0.070f},
        {Vec3(0, 0.275f, 0.12f), 0.160f, 0.100f},
        {Vec3(0, 0.300f, 0.28f), 0.170f, 0.085f},
        {Vec3(0, 0.290f, 0.37f), 0.070f, 0.055f}
    };
    m.tube(body, 6, 8);
    // What is left of the clothes
    m.strip = RAG;
    CreatureRing rag[3] = {
        {Vec3(0, 0.305f, -0.535f), 0.112f, 0.076f},
        {Vec3(0, 0.335f, -0.380f), 0.162f, 0.100f},
        {Vec3(0, 0.295f, -0.270f), 0.132f, 0.084f}
    };
    m.tube(rag, 3, 8);

    // Head hangs from a neck that turned too far: the face looks up and sideways
    m.beginPart(CJ_HEAD, root, Vec3(0, 0.29f, 0.37f));
    m.strip = SKIN;
    m.limb(Vec3(0, 0.29f, 0.36f), Vec3(0.01f, 0.255f, 0.52f), 0.042f, 0.036f, 6);
    Vec3 hb(0.010f, 0.240f, 0.490f), ht(0.035f, 0.205f, 0.770f);
    m.ovoid(hb, ht, 0.090f, 0.105f, 8);
    m.strip = DARK;
    m.ovoid(Vec3(0.066f, 0.300f, 0.655f), Vec3(0.068f, 0.299f, 0.677f), 0.011f, 0.010f, 5);
    m.ovoid(Vec3(0.112f, 0.232f, 0.660f), Vec3(0.114f, 0.230f, 0.682f), 0.010f, 0.012f, 5);

    // Arms: the right forearm broke and set crooked
    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 shoulder(fs * 0.160f, 0.300f, 0.260f);
        m.beginPart(s < 0 ? CJ_ARM_L : CJ_ARM_R, root, shoulder);
        m.strip = SKIN;
        Vec3 wrist;
        if (s < 0) {
            Vec3 elbow(-0.400f, 0.450f, 0.215f);
            wrist = Vec3(-0.450f, 0.045f, 0.390f);
            m.limb3(shoulder, elbow, wrist, 0.052f, 0.046f, 0.032f, 6);
        } else {
            Vec3 elbow(0.370f, 0.500f, 0.290f);
            Vec3 kink(0.520f, 0.290f, 0.320f);
            wrist = Vec3(0.430f, 0.045f, 0.450f);
            CreatureRing arm[4] = { {shoulder, 0.052f, 0.052f}, {elbow, 0.046f, 0.046f}, {kink, 0.040f, 0.040f}, {wrist, 0.032f, 0.032f} };
            m.tube(arm, 4, 6);
        }
        m.strip = GRIME;
        Vec3 knuckle(wrist.x + fs * 0.03f, 0.022f, wrist.z + 0.13f);
        CreatureRing palm[2] = { {wrist, 0.036f, 0.016f}, {knuckle, 0.046f, 0.012f} };
        m.tube(palm, 2, 4);
        for (int f = 0; f < 4; f++) {
            float off = ((float)f - 1.5f) * 0.026f;
            Vec3 base(knuckle.x + off, knuckle.y, knuckle.z);
            Vec3 tip(knuckle.x + off * 2.2f + fs * 0.01f, 0.010f, knuckle.z + 0.15f - fabsf(off) * 1.2f);
            m.limb(base, tip, 0.010f, 0.005f, 4);
        }
    }

    // Legs: knees ride above the back
    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 hip(fs * 0.110f, 0.310f, -0.440f);
        Vec3 knee = s < 0 ? Vec3(-0.420f, 0.500f, -0.300f) : Vec3(0.380f, 0.540f, -0.400f);
        Vec3 ankle = s < 0 ? Vec3(-0.460f, 0.050f, -0.560f) : Vec3(0.490f, 0.050f, -0.640f);
        m.beginPart(s < 0 ? CJ_LEG_L : CJ_LEG_R, root, hip);
        m.strip = SKIN;
        m.limb3(hip, knee, ankle, 0.072f, 0.056f, 0.036f, 6);
        m.strip = GRIME;
        Vec3 toes(ankle.x + fs * 0.07f, 0.022f, ankle.z - 0.20f);
        CreatureRing foot[2] = { {ankle, 0.036f, 0.024f}, {toes, 0.044f, 0.014f} };
        m.tube(foot, 2, 5);
    }
    m.finish();
}

// E-03 Shadow - a person-shaped absence. No detail at all, limbs taper into nothing.
inline void buildShadowModel(CreatureMesh& m) {
    m = CreatureMesh();
    m.strips = 1;
    m.vScale = 1.0f;

    int root = m.beginPart(CJ_ROOT, -1, Vec3(0, 0.0f, 0));
    CreatureRing torso[5] = {
        {Vec3(0, 1.00f, 0.00f), 0.130f, 0.085f},
        {Vec3(0, 1.30f, 0.00f), 0.105f, 0.072f},
        {Vec3(0, 1.62f, 0.02f), 0.155f, 0.090f},
        {Vec3(0, 1.82f, 0.03f), 0.185f, 0.082f},
        {Vec3(0, 1.92f, 0.04f), 0.060f, 0.050f}
    };
    m.tube(torso, 5, 8);
    m.limb(Vec3(0, 1.90f, 0.04f), Vec3(0, 2.02f, 0.05f), 0.040f, 0.036f, 6);
    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        CreatureRing leg[3] = {
            {Vec3(fs * 0.070f, 1.08f, 0.00f), 0.072f, 0.072f},
            {Vec3(fs * 0.078f, 0.56f, 0.02f), 0.046f, 0.046f},
            {Vec3(fs * 0.055f, 0.00f, 0.00f), 0.010f, 0.010f}
        };
        m.tube(leg, 3, 6);
    }

    m.beginPart(CJ_HEAD, root, Vec3(0, 2.00f, 0.05f));
    m.ovoid(Vec3(0, 1.97f, 0.05f), Vec3(0, 2.27f, 0.07f), 0.086f, 0.100f, 8);

    for (int s = -1; s <= 1; s += 2) {
        float fs = (float)s;
        Vec3 shoulder(fs * 0.200f, 1.80f, 0.03f);
        m.beginPart(s < 0 ? CJ_ARM_L : CJ_ARM_R, root, shoulder);
        CreatureRing arm[3] = {
            {shoulder, 0.046f, 0.046f},
            {Vec3(fs * 0.250f, 1.28f, 0.02f), 0.034f, 0.034f},
            {Vec3(fs * 0.250f, 0.60f, 0.05f), 0.007f, 0.007f}
        };
        m.tube(arm, 3, 6);
    }
    m.finish();
}

inline void buildEntityModel(EntityType type, CreatureMesh& m) {
    if (type == ENTITY_CRAWLER) buildCrawlerModel(m);
    else if (type == ENTITY_SHADOW) buildShadowModel(m);
    else buildStalkerModel(m);
}

// Muted, slightly sick office palette. Strip order matches the builders above.
inline void genEntityTextureByType(EntityType type, unsigned char* data, int w, int h) {
    if (type == ENTITY_STALKER) {
        const CreatureStripStyle st[4] = {
            {168.0f, 160.0f, 146.0f, 10.0f, CREATURE_PAT_SKIN},
            {146.0f, 148.0f, 142.0f, 9.0f, CREATURE_PAT_SHIRT},
            {44.0f, 43.0f, 46.0f, 6.0f, CREATURE_PAT_WEAVE},
            {17.0f, 15.0f, 15.0f, 4.0f, CREATURE_PAT_PLAIN}
        };
        genCreatureTexture(st, 4, data, w, h, 11u);
    } else if (type == ENTITY_CRAWLER) {
        const CreatureStripStyle st[4] = {
            {142.0f, 134.0f, 122.0f, 12.0f, CREATURE_PAT_SKIN},
            {74.0f, 66.0f, 58.0f, 12.0f, CREATURE_PAT_SKIN},
            {58.0f, 55.0f, 50.0f, 9.0f, CREATURE_PAT_WEAVE},
            {15.0f, 13.0f, 13.0f, 4.0f, CREATURE_PAT_PLAIN}
        };
        genCreatureTexture(st, 4, data, w, h, 23u);
    } else {
        const CreatureStripStyle st[1] = { {5.0f, 5.0f, 6.0f, 2.0f, CREATURE_PAT_PLAIN} };
        genCreatureTexture(st, 1, data, w, h, 37u);
    }
}

inline void genEntityTexture(unsigned char* data, int w, int h) {
    genEntityTextureByType(ENTITY_STALKER, data, w, h);
}

#endif
