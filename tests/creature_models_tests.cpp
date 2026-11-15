#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "../src/entity_model.h"
#include "../src/creature_anim.h"

const float EPS = 1e-3f;

void assertWellFormed(const CreatureMesh& m, float maxHeight) {
    assert(!m.v.empty());
    assert(m.v.size() % (CREATURE_STRIDE * 3) == 0);
    for (size_t i = 0; i < m.v.size(); i += CREATURE_STRIDE) {
        for (int k = 0; k < CREATURE_STRIDE; k++) assert(std::isfinite(m.v[i + k]));
        float y = m.v[i + 1];
        assert(y > -0.02f && y < maxHeight);
        float u = m.v[i + 3];
        assert(u >= 0.0f && u <= 1.0f);
        float nx = m.v[i + 5], ny = m.v[i + 6], nz = m.v[i + 7];
        assert(fabsf(sqrtf(nx * nx + ny * ny + nz * nz) - 1.0f) < EPS);
    }
}

// Every vertex belongs to exactly one part, and parents come before children.
void assertPartsCoverMesh(const CreatureMesh& m) {
    assert(!m.parts.empty());
    assert((int)m.parts.size() <= CREATURE_MAX_PARTS);
    int next = 0;
    for (size_t i = 0; i < m.parts.size(); i++) {
        const CreaturePart& p = m.parts[i];
        assert(p.first == next);
        assert(p.count > 0 && p.count % 3 == 0);
        assert(p.parent < (int)i);
        assert(p.joint >= 0 && p.joint < CJ_COUNT);
        next += p.count;
    }
    assert(m.parts[0].parent == -1);
    assert(next == m.vertexCount());
}

// Each strip keeps its UVs inside its own column so materials never bleed.
void assertUvInsideStrips(const CreatureMesh& m) {
    for (size_t i = 0; i < m.v.size(); i += CREATURE_STRIDE) {
        float scaled = m.v[i + 3] * (float)m.strips;
        float frac = scaled - floorf(scaled);
        assert(frac > 0.05f && frac < 0.95f);
    }
}

void testEntityModels() {
    const EntityType types[3] = { ENTITY_STALKER, ENTITY_CRAWLER, ENTITY_SHADOW };
    const float heights[3] = { 2.6f, 0.9f, 2.5f };
    for (int i = 0; i < 3; i++) {
        CreatureMesh m;
        buildEntityModel(types[i], m);
        assertWellFormed(m, heights[i]);
        assertPartsCoverMesh(m);
        assertUvInsideStrips(m);
    }
}

void testFacesPointOutward() {
    // A closed convex tube: every face normal must point away from the axis.
    CreatureMesh m;
    m.beginPart(CJ_ROOT, -1, Vec3(0, 0, 0));
    m.limb(Vec3(0, 0, 0), Vec3(0, 1, 0), 0.2f, 0.1f, 6);
    m.finish();
    for (size_t i = 0; i < m.v.size(); i += CREATURE_STRIDE * 3) {
        Vec3 a(m.v[i], m.v[i + 1], m.v[i + 2]);
        Vec3 b(m.v[i + 8], m.v[i + 9], m.v[i + 10]);
        Vec3 c(m.v[i + 16], m.v[i + 17], m.v[i + 18]);
        Vec3 n(m.v[i + 5], m.v[i + 6], m.v[i + 7]);
        Vec3 center = (a + b + c) * (1.0f / 3.0f);
        Vec3 fromAxis(center.x, center.y - 0.5f, center.z);
        assert(n.dot(fromAxis) > 0.0f);
        // Counter-clockwise winding agrees with the stored normal (back-face culling is on).
        assert((b - a).cross(c - a).dot(n) > 0.0f);
    }
}

void testTextureFilled() {
    std::vector<unsigned char> d(64 * 64 * 4, 0);
    genEntityTextureByType(ENTITY_STALKER, d.data(), 64, 64);
    for (size_t i = 3; i < d.size(); i += 4) assert(d[i] == 255);
    // The shadow stays a void even under a flashlight.
    genEntityTextureByType(ENTITY_SHADOW, d.data(), 64, 64);
    for (size_t i = 0; i < d.size(); i += 4) assert(d[i] < 16 && d[i + 1] < 16 && d[i + 2] < 16);
}

void testWorldMatrixFacesYaw() {
    float yaw = 0.7f;
    Mat4 w = creatureWorldMatrix(Vec3(3, 0, -2), yaw);
    Vec3 f = w.transformDir(Vec3(0, 0, 1));
    assert(fabsf(f.x - sinf(yaw)) < EPS && fabsf(f.z - cosf(yaw)) < EPS);
    Vec3 p = w.transformPoint(Vec3(0, 0, 0));
    assert(fabsf(p.x - 3.0f) < EPS && fabsf(p.z + 2.0f) < EPS);
}

void testRestPoseIsIdentityAndJointsPivot() {
    CreatureMesh m;
    buildStalkerModel(m);
    Mat4 mats[CREATURE_MAX_PARTS];
    CreaturePose rest;
    creaturePartMatrices(m.parts, rest, Mat4(), mats);
    for (size_t i = 0; i < m.parts.size(); i++) {
        for (int k = 0; k < 16; k++) assert(fabsf(mats[i].m[k] - Mat4().m[k]) < EPS);
    }
    // Rotating a joint keeps its pivot in place and carries the child along.
    CreaturePose bent;
    bent.rot[CJ_LEG_L].x = -0.6f;
    creaturePartMatrices(m.parts, bent, Mat4(), mats);
    int thigh = -1, shin = -1;
    for (size_t i = 0; i < m.parts.size(); i++) {
        if (m.parts[i].joint == CJ_LEG_L) thigh = (int)i;
        if (m.parts[i].joint == CJ_SHIN_L) shin = (int)i;
    }
    assert(thigh >= 0 && shin >= 0 && m.parts[shin].parent == thigh);
    Vec3 hip = m.parts[thigh].pivot;
    Vec3 hipAfter = mats[thigh].transformPoint(hip);
    assert((hipAfter - hip).len() < EPS);
    Vec3 knee = m.parts[shin].pivot;
    Vec3 kneeAfter = mats[shin].transformPoint(knee);
    assert(kneeAfter.z > knee.z + 0.1f); // negative X rotation swings the leg forward
    assert(fabsf((kneeAfter - hip).len() - (knee - hip).len()) < EPS);
}

void testGaitFollowsTravel() {
    float yaw = 1.1f;
    Vec3 a(4, 0, 9);
    Vec3 step(sinf(yaw) * 0.76f, 0, cosf(yaw) * 0.76f);
    float d = creatureGaitPhase(a + step, yaw, 0.76f) - creatureGaitPhase(a, yaw, 0.76f);
    assert(fabsf(d - 3.14159265f) < 0.01f);
    // Standing still never animates the legs.
    CreaturePose p0 = stalkerPose(1.3f, 0.0f, ENT_STALKING);
    CreaturePose p1 = stalkerPose(1.3f, 5.0f, ENT_STALKING);
    assert(p0.rot[CJ_LEG_L].x == p1.rot[CJ_LEG_L].x);
    assert(p0.rot[CJ_SHIN_R].x == p1.rot[CJ_SHIN_R].x);
}

void testPosesStayFinite() {
    const EntityState states[6] = { ENT_IDLE, ENT_ROAMING, ENT_STALKING, ENT_CHASING, ENT_FLEEING, ENT_ATTACKING };
    for (int s = 0; s < 6; s++) {
        for (int i = 0; i < 40; i++) {
            float g = (float)i * 0.37f - 5.0f, t = (float)i * 1.9f;
            CreaturePose poses[4] = {
                stalkerPose(g, t, states[s]), crawlerPose(g, t, states[s]), shadowPose(t, states[s]),
                playerPose(g, (float)(i % 3) * 0.5f, (float)(i % 2), (float)(i % 5) * 0.6f - 1.2f, (i % 2) == 0, t)
            };
            for (int p = 0; p < 4; p++) {
                for (int j = 0; j < CJ_COUNT; j++) {
                    assert(std::isfinite(poses[p].rot[j].x) && fabsf(poses[p].rot[j].x) < 3.2f);
                    assert(std::isfinite(poses[p].rot[j].y) && fabsf(poses[p].rot[j].y) < 3.2f);
                    assert(std::isfinite(poses[p].rot[j].z) && fabsf(poses[p].rot[j].z) < 3.2f);
                }
            }
        }
    }
}

int main() {
    testEntityModels();
    testFacesPointOutward();
    testTextureFilled();
    testWorldMatrixFacesYaw();
    testRestPoseIsIdentityAndJointsPivot();
    testGaitFollowsTravel();
    testPosesStayFinite();
    std::cout << "All creature model tests passed.\n";
    return 0;
}
