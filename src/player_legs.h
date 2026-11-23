#pragma once
// ============================================================================
// PLAYER LEGS
// The local player walks on the leg simulation (leg_sim.h and friends, brought over
// from the legsim demo as they are). This file is the only place where the game and
// the simulation meet:
//   - the keys push the simulated body, the game's walls, doors and props stop it;
//   - `cam.pos` stays what the rest of the game takes it for - where the player
//     stands, at standing or crouching height - and is copied from the body;
//   - the picture is taken from the eyes of that body: for rendering and the HUD the
//     camera is swapped for them and put back (the way spectator_cam.h does it), so
//     all the camera motion comes from the legs;
//   - heel strikes trigger the footstep sound;
//   - the legs are drawn, so looking down shows them.
// Anything that moves `cam.pos` on its own (spawns, teleports, respawns, the dev
// self-test) is noticed by legsSync(): the body is put down again at the new place.
// ============================================================================
#include "leg_model.h"
#include "ai_senses.h"

// Top speeds are the game's own - the ones the monsters' hearing, the stamina and the chases
// are balanced around (ai_senses.h) - not the demo's 1.7 / 4.2 m/s. At 4 m/s the legs jog;
// the walking gait shows when crouching and when downed.
inline constexpr float LEGS_WALK_SPEED = PLAYER_WALK_SPEED;    // m/s, forward
inline constexpr float LEGS_SPRINT_SCALE = PLAYER_SPRINT_SPEED / PLAYER_WALK_SPEED;
inline constexpr float LEGS_CROUCH_SCALE = 0.5f;
inline constexpr float LEGS_SIM_CROUCH_SCALE = 0.55f;  // what LegSim::moveBody takes off by itself when crouching
inline constexpr float LEGS_BOB = 1.0f;           // 0 = steady camera, 1 = everything the legs do
inline constexpr float LEGS_EYE_FWD = 0.09f;      // m, the eyes sit in front of the axis the head turns on
inline constexpr float LEGS_EYE_CLEAR = 0.18f;    // m the eyes keep from walls: the near plane must not cut into them
inline constexpr float LEGS_SHOVE_MAX = 0.75f;    // m in one frame that is still a shove; further is a teleport
inline constexpr float LEGS_MAX_FRAME = 0.1f;     // s, the longest frame the legs are asked to make up for
inline constexpr float LEGS_SHUFFLE_WEIGHT = -0.45f;  // footstep weight of a step taken on the spot

struct PlayerLegs {
    LegSim sim;
    HeadState headState;
    HeadView head;
    BodyModel model;
    GLuint shader = 0;      // the main shader with soft shading, see modelShaderSource()
    bool gfxReady = false;
    bool placed = false;    // the body stands where the player is
    bool live = false;      // the player is on foot: the view and the legs are the body's
    Vec3 synced;            // cam.pos as the body last left it
    float view = 0.0f, viewPitch = 0.0f;  // where the eyes look: settles on cam.yaw / cam.pitch (leg_view.h)
    float look = 0.0f;                    // where the head and the body are heading: comes round after the view
    float lookAim = 0.0f, lookRate = 0.0f;  // kept for LegSim::followLook
    Vec3 eye;
    // The gameplay camera while the eyes are in its place (legsBeginView / legsEndView).
    bool viewActive = false;
    Vec3 savedPos;
    float savedYaw = 0.0f, savedPitch = 0.0f;
};
inline PlayerLegs gLegs;
// Scripted push forward, added to the keys: the dev self-test walks with it.
inline float gLegsScriptFwd = 0.0f;

// The simulation wants exact square roots: a planted foot must not creep.
struct LegsExactMath {
    bool was = g_fastMathEnabled;
    LegsExactMath() { g_fastMathEnabled = false; }
    ~LegsExactMath() { g_fastMathEnabled = was; }
};

// Everything that stops a walking player. Used for the body and, with a small radius, for the feet.
inline bool legsBlocked(float x, float z, float r) {
    if (collideWorld(x, z, r) || collideCoopDoor(x, z, r) || collideHeistSolids(x, z, r) || collideMapProps(x, z, r)) return true;
    return falseDoorTimer > 0 && nearPoint2D(Vec3(x, 0, z), falseDoorPos, 1.0f);
}

inline bool legsBlockedHard(float x, float z, float r) {
    if (collideWorld(x, z, r) || collideCoopDoor(x, z, r) || collideHeistSolids(x, z, r)) return true;
    return falseDoorTimer > 0 && nearPoint2D(Vec3(x, 0, z), falseDoorPos, 1.0f);
}

// Where the eyes are, from the head the simulation gives.
inline void legsPlaceEye() {
    PlayerLegs& g = gLegs;
    Vec3 root = g.sim.root;
    Vec3 eye = g.head.eye + headingFwd(g.view + g.head.yaw) * LEGS_EYE_FWD;
    // The head leans out over the feet (a lot when crouching). Next to a wall it stays over them.
    for (int k = 0; k < 4 && legsBlockedHard(eye.x, eye.z, LEGS_EYE_CLEAR); k++) {
        float keep = k < 3 ? 0.5f : 0.0f;
        eye.x = root.x + (eye.x - root.x) * keep;
        eye.z = root.z + (eye.z - root.z) * keep;
    }
    g.eye = eye;
}

// Puts the body down where the player is, standing, looking where the camera looks.
inline void legsPlace() {
    PlayerLegs& g = gLegs;
    LegsExactMath exact;
    g.sim.reset(cam.pos, cam.yaw);
    g.headState = HeadState();
    g.head = computeHead(g.sim, g.headState, 1.0f / 120.0f, LEGS_BOB);
    g.view = g.look = g.lookAim = cam.yaw;
    g.viewPitch = cam.pitch;
    g.lookRate = 0.0f;
    g.synced = cam.pos;
    g.placed = true;
    legsPlaceEye();
}

// The game moved the player behind the body's back. A short way is a shove: the body goes,
// the feet are left where they stand and step after it. Anything else is a new place.
inline void legsSync() {
    PlayerLegs& g = gLegs;
    float dx = cam.pos.x - g.synced.x, dz = cam.pos.z - g.synced.z;
    if (g.placed && dx == 0.0f && dz == 0.0f) return;
    if (!g.placed || dx * dx + dz * dz > LEGS_SHOVE_MAX * LEGS_SHOVE_MAX) {
        legsPlace();
        return;
    }
    g.sim.root.x = cam.pos.x;
    g.sim.root.z = cam.pos.z;
    g.head.eye.x += dx;
    g.head.eye.z += dz;
    g.eye.x += dx;
    g.eye.z += dz;
    g.synced = cam.pos;
}

// The floor under the player moved by `delta` and took the player along (the rig's deck):
// the feet stand on it, so the whole body goes, footprints and all. Call it right after
// moving cam.pos by the same amount.
inline void legsCarry(const Vec3& delta) {
    PlayerLegs& g = gLegs;
    if (!g.placed) return;
    Vec3 d(delta.x, 0.0f, delta.z);
    LegSim& s = g.sim;
    s.root += d;
    s.lastCop += d;
    s.pelvisPos += d;
    s.lastStrikePos += d;
    for (int i = 0; i < 2; i++) {
        s.hip[i] += d;
        s.foot[i].pose.anchor += d;
        s.foot[i].from += d;
        s.foot[i].target += d;
        s.foot[i].landRoot += d;
    }
    g.head.eye += d;
    g.eye += d;
    g.synced.x += delta.x;
    g.synced.z += delta.z;
}

// The player is not on foot (falling, flying, riding along as a ghost): the camera is the
// game's again. The body is put down anew when the player is back.
inline void legsRest() {
    gLegs.live = false;
    gLegs.placed = false;
}

// How fast the body is walking right now (nothing when the player is not on foot).
inline Vec3 legsVelocity() {
    return (gLegs.live && gLegs.placed) ? gLegs.sim.vel : Vec3();
}

// A pushable prop in the way is shoved along, as walking into it always did. The body is
// about to move `reach` metres along (dirX, dirZ).
inline void legsPushProps(float dirX, float dirZ, float reach) {
    const Vec3& at = gLegs.sim.root;
    float nx = at.x + dirX * reach, nz = at.z + dirZ * reach;
    if (fabsf(dirX) > 0.05f && !legsBlockedHard(nx, at.z, PR) && collideMapProps(nx, at.z, PR)) tryPushMapProps(nx, at.z, PR, dirX, 0.0f);
    if (fabsf(dirZ) > 0.05f && !legsBlockedHard(at.x, nz, PR) && collideMapProps(at.x, nz, PR)) tryPushMapProps(at.x, nz, PR, 0.0f, dirZ);
}

// One frame of walking. `top` is the speed the player may make right now, forward (sprint,
// crouch, load and injuries already in it); `onFoot` is false when the keys drive something
// else. Returns true while the body is really moving.
inline bool legsWalk(GLFWwindow* w, float top, bool sprinting, bool onFoot) {
    PlayerLegs& g = gLegs;
    LegsExactMath exact;
    legsSync();
    g.live = true;
    float dt = dTime < LEGS_MAX_FRAME ? dTime : LEGS_MAX_FRAME;

    // The eyes are the mouse's, with a little weight. The body's look comes round after them
    // as fast as the legs can step; the keys move the body where the eyes look (LegInput::steer).
    float settle = viewSettle(dt);
    float turned = (cam.yaw - g.view) * settle;
    g.view += turned;
    g.viewPitch += (cam.pitch - g.viewPitch) * settle;
    g.look = g.sim.followLook(g.look, g.lookAim, g.lookRate, turned, dt);

    LegInput in;
    auto held = [w](int key) { return glfwGetKey(w, key) == GLFW_PRESS ? 1.0f : 0.0f; };
    if (onFoot) {
        in.moveFwd = held(settings.binds.forward) - held(settings.binds.back) + gLegsScriptFwd;
        in.moveRight = held(settings.binds.right) - held(settings.binds.left);
    }
    in.yaw = g.look;
    in.steer = g.view - g.look;
    in.crouch = cam.crouch;
    in.run = sprinting && !cam.crouch;
    // (The simulation slows a crouching body by itself; `top` has the game's own factor in it.)
    in.walkSpeed = in.runSpeed = cam.crouch ? top / LEGS_SIM_CROUCH_SCALE : top;

    float wish = sqrtf(in.moveFwd * in.moveFwd + in.moveRight * in.moveRight);
    if (wish > 0.01f) {
        Vec3 dir = (headingFwd(g.view) * in.moveFwd + headingRight(g.view) * in.moveRight) / wish;
        float reach = g.sim.vel.len() * dt;
        legsPushProps(dir.x, dir.z, reach > 0.03f ? reach : 0.03f);
    }

    // No substep longer than 1/120 s; the state is then exactly at frame time.
    int n = (int)ceilf(dt * 120.0f);
    if (n < 1) n = 1;
    float h = dt / n;
    for (int k = 0; k < n; k++) {
        g.sim.strikeCount = 0;
        g.sim.step(in, h, legsBlocked);
        g.head = computeHead(g.sim, g.headState, h, LEGS_BOB);
        for (int i = 0; i < g.sim.strikeCount; i++) {
            const LegStrike& s = g.sim.strikes[i];
            bool shuffle = s.force < 0.3f;  // a corrective step on the spot, not a step of the walk
            sndState.stepPitch = 0.88f + (float)(rand() % 30) / 100.0f + 0.10f * g.sim.run;
            sndState.stepWeight = shuffle ? LEGS_SHUFFLE_WEIGHT : 0.0f;
            sndState.stepFoot = s.foot;
            sndState.stepTrig = true;
        }
    }

    cam.pos.x = g.sim.root.x;
    cam.pos.z = g.sim.root.z;
    cam.pos.y = cam.curH;
    g.synced = cam.pos;
    legsPlaceEye();
    return g.sim.speed > 0.25f;
}

// Swaps the gameplay camera for the eyes of the body around rendering and the HUD, then
// puts it back. Call after spectatorBeginView() / before spectatorEndView().
inline void legsBeginView() {
    PlayerLegs& g = gLegs;
    g.viewActive = false;
    if (!g.gfxReady || !g.live || gSpectator.viewActive) return;
    if (isPlayerDead || playerFalling || debugTools.flyMode || spectatorRiding()) return;
    legsSync();  // moved after this frame's walk: look from the new place, not from the old one
    g.savedPos = cam.pos;
    g.savedYaw = cam.yaw;
    g.savedPitch = cam.pitch;
    cam.pos = g.eye;
    cam.yaw = g.view + g.head.yaw;
    cam.pitch = gClamp(g.viewPitch + g.head.pitch, -1.5f, 1.5f);
    g.viewActive = true;
}

inline void legsEndView() {
    PlayerLegs& g = gLegs;
    if (!g.viewActive) return;
    g.viewActive = false;
    cam.pos = g.savedPos;
    cam.yaw = g.savedYaw;
    cam.pitch = g.savedPitch;
}

// The camera's up vector: the head rolls a little with the hips, and a lot with a body
// tumbling down a shaft (it stays as it was when the fall killed).
inline Vec3 legsViewUp(float viewYaw) {
    bool fell = (playerFalling || isPlayerDead) && !gSpectator.viewActive;
    if (!gLegs.viewActive && !fell) return Vec3(0, 1, 0);
    float roll = gLegs.viewActive ? gLegs.head.roll : fallViewRoll(gFall);
    return Vec3(0, cosf(roll), 0) + headingRight(viewYaw) * sinf(roll);
}

inline void legsInitGfx() {
    PlayerLegs& g = gLegs;
    g.shader = mkShader(mainVS, modelShaderSource(mainFS).c_str());
    modelInit(g.model, g.sim.dims);
    g.gfxReady = true;
}

// What renderScene() gave the main shader this frame; the body is lit the same way.
struct LegsLighting {
    Vec3 flashDir, flashPos;
    bool flashOn = false;
    int remoteFlashCount = 0;
    const float* remoteFlashPos = nullptr;
    const float* remoteFlashDir = nullptr;
    int lightCount = 0;
    const float* lightPos = nullptr;
};

// Draws the legs (first person: everything up to the waist). Leaves the main shader bound.
inline void legsDraw(const Mat4& proj, const Mat4& view, const LegsLighting& l) {
    PlayerLegs& g = gLegs;
    if (!g.viewActive || gCoopDead) return;  // (a ghost has no legs to look at)
    struct Uniforms {
        GLint P = -2, V, M, vp, tm, danger, flashOn, flashDir, flashPos, rfc, rfp, rfd, nl, lp, tint;
    };
    static Uniforms u;
    if (u.P == -2) {
        u.P = glGetUniformLocation(g.shader, "P");
        u.V = glGetUniformLocation(g.shader, "V");
        u.M = glGetUniformLocation(g.shader, "M");
        u.vp = glGetUniformLocation(g.shader, "vp");
        u.tm = glGetUniformLocation(g.shader, "tm");
        u.danger = glGetUniformLocation(g.shader, "danger");
        u.flashOn = glGetUniformLocation(g.shader, "flashOn");
        u.flashDir = glGetUniformLocation(g.shader, "flashDir");
        u.flashPos = glGetUniformLocation(g.shader, "flashPos");
        u.rfc = glGetUniformLocation(g.shader, "rfc");
        u.rfp = glGetUniformLocation(g.shader, "rfp");
        u.rfd = glGetUniformLocation(g.shader, "rfd");
        u.nl = glGetUniformLocation(g.shader, "nl");
        u.lp = glGetUniformLocation(g.shader, "lp");
        u.tint = glGetUniformLocation(g.shader, "modelTint");
    }
    glUseProgram(g.shader);
    glUniformMatrix4fv(u.P, 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(u.V, 1, GL_FALSE, view.m);
    glUniform3f(u.vp, cam.pos.x, cam.pos.y, cam.pos.z);
    glUniform1f(u.tm, vhsTime);
    glUniform1f(u.danger, entityMgr.dangerLevel);
    glUniform1i(u.flashOn, l.flashOn ? 1 : 0);
    glUniform3f(u.flashDir, l.flashDir.x, l.flashDir.y, l.flashDir.z);
    glUniform3f(u.flashPos, l.flashPos.x, l.flashPos.y, l.flashPos.z);
    glUniform1i(u.rfc, l.remoteFlashCount);
    if (l.remoteFlashCount > 0) {
        glUniform3fv(u.rfp, l.remoteFlashCount, l.remoteFlashPos);
        glUniform3fv(u.rfd, l.remoteFlashCount, l.remoteFlashDir);
    }
    glUniform1i(u.nl, l.lightCount);
    if (l.lightCount > 0) glUniform3fv(u.lp, l.lightCount, l.lightPos);
    {
        LegsExactMath exact;
        drawModel(g.model, u.M, u.tint, g.sim, g.head, cam.yaw, cam.pitch, false);
    }
    glUseProgram(mainShader);
}
