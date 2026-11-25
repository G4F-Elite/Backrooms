#pragma once
// Draws the rig. It moves every frame, so unlike the other heist sets it is not baked into
// the decor mesh: the hull is one mesh in rig space placed with a model matrix, the auger a
// second one that spins. The break room lamp is fed to the shader as one more scene light.

#include "rig_models.h"
#include "zfight_guard.h"

#include "model_transform.h"

inline GLuint rigHullVAO = 0, rigHullVBO = 0;
inline GLuint rigAugerVAO = 0, rigAugerVBO = 0;
inline int rigHullVC = 0, rigAugerVC = 0;
inline int gRigHullSig = -1;
inline float gRigAugerSpin = 0.0f;

inline RigLook rigCurrentLook() {
    RigLook look;
    look.lamps = heistDrillLamps();
    look.gaugeLamps = DRILL_GAUGE_LAMPS;
    look.ready = heistDrillReady() || heist.stage == HEIST_DRILLING;
    look.pulled = heist.stage == HEIST_DRILLING;
    look.fault = heist.rigFault;
    look.power = rigLampLit();
    look.charged = rigChargerReady();
    return look;
}

inline void rigRefreshMeshes() {
    if (rigAugerVC == 0) {
        std::vector<float> m;
        DecorBuilder b(m, 0.0f, 0.0f, 0.0f, 1.0f);
        buildRigAugerModel(b);
        resolveModelMesh(m, "rig auger");
        rigAugerVC = (int)m.size() / 8;
        setupVAO(rigAugerVAO, rigAugerVBO, m, true);
    }
    RigLook look = rigCurrentLook();
    int sig = rigLookSignature(look);
    if (sig == gRigHullSig && rigHullVC > 0) return;
    gRigHullSig = sig;
    std::vector<float> m;
    DecorBuilder b(m, 0.0f, 0.0f, 0.0f, 1.0f);
    buildRigHullModel(b, look);
    resolveModelMesh(m, "rig hull");
    rigHullVC = (int)m.size() / 8;
    setupVAO(rigHullVAO, rigHullVBO, m, true);
}

inline float rigAugerSpinRate() {
    if (heist.stage == HEIST_DRILLING) return 17.0f;
    if (rigMoving()) return 5.5f;
    return rigEngineOn() ? 1.2f : 0.0f;
}

// The auger mesh stands on its base: its up axis is laid along the heading and the other
// two roll around it.
inline Mat4 rigAugerMatrix(float yaw, float spin, float lift) {
    Vec3 fwd(mSin(yaw), 0.0f, mCos(yaw)), right(mCos(yaw), 0.0f, -mSin(yaw)), up(0.0f, 1.0f, 0.0f);
    float c = cosf(spin), s = sinf(spin);
    Vec3 ax = right * c + up * s;
    Vec3 az = right * s - up * c;
    Vec3 at = rigWorld(0.0f, RIG_AUGER_BASE, RIG_AUGER_HEIGHT + lift);
    Mat4 m;
    m.m[0] = ax.x; m.m[1] = ax.y; m.m[2] = ax.z;
    m.m[4] = fwd.x; m.m[5] = fwd.y; m.m[6] = fwd.z;
    m.m[8] = az.x; m.m[9] = az.y; m.m[10] = az.z;
    m.m[12] = at.x; m.m[13] = at.y; m.m[14] = at.z;
    return m;
}

// Called from renderScene with the main shader bound. Leaves M and the tint reset.
inline void renderRig(GLint uModel, GLint uTint) {
    if (!coop.initialized) return;
    rigRefreshMeshes();
    float yaw = rigYaw(heist.rigDir);
    gRigAugerSpin += rigAugerSpinRate() * dTime;
    if (gRigAugerSpin > 6.2831853f) gRigAugerSpin -= 6.2831853f;

    // A running engine shivers; a drilling one shakes.
    float shiver = 0.0f;
    if (heist.stage == HEIST_DRILLING) shiver = sinf(vhsTime * 47.0f) * 0.02f;
    else if (rigEngineOn()) shiver = sinf(vhsTime * 31.0f) * 0.0035f;
    Vec3 tint = rigLampLit() ? Vec3(1.14f, 1.10f, 1.00f) : Vec3(0.50f, 0.47f, 0.47f);

    glDisable(GL_CULL_FACE); // decor meshes are two-sided
    glBindTexture(GL_TEXTURE_2D, decorTex);
    if (uTint >= 0) glUniform3f(uTint, tint.x, tint.y, tint.z);
    Mat4 hull = composeModelMatrix(Vec3(heist.drillPos.x, shiver, heist.drillPos.z), yaw, 0.0f, Vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(uModel, 1, GL_FALSE, hull.m);
    glBindVertexArray(rigHullVAO);
    glDrawArrays(GL_TRIANGLES, 0, rigHullVC);
    Mat4 auger = rigAugerMatrix(yaw, gRigAugerSpin, shiver);
    glUniformMatrix4fv(uModel, 1, GL_FALSE, auger.m);
    glBindVertexArray(rigAugerVAO);
    glDrawArrays(GL_TRIANGLES, 0, rigAugerVC);
    glEnable(GL_CULL_FACE);

    Mat4 identity;
    glUniformMatrix4fv(uModel, 1, GL_FALSE, identity.m);
    if (uTint >= 0) glUniform3f(uTint, 1.0f, 1.0f, 1.0f);
}

// The break room lamp: a pool of light that travels with the rig, also through a blackout.
// Takes a free slot of the scene light list, or the last one when the list is full.
inline int rigInjectLight(float* lightPos, int count, int limit) {
    if (!coop.initialized || !rigLampLit()) return count;
    Vec3 lamp = rigWorld(0.0f, RIG_HAVEN_Z, RIG_CABIN_H - 0.25f);
    Vec3 d = lamp - cam.pos;
    if (d.x * d.x + d.z * d.z > 45.0f * 45.0f) return count;
    int slot = count < limit ? count++ : limit - 1;
    lightPos[slot * 3] = lamp.x;
    lightPos[slot * 3 + 1] = lamp.y;
    lightPos[slot * 3 + 2] = lamp.z;
    return count;
}
