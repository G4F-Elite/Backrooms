// Backrooms VHS Horror: the game in the LIMEN world (limen_*.h, see LIMEN.md): an endless
// synthesized world of slabs and megastructures, human-scale thin partitions, walkable dry
// basins, generated sound, a 128x128 far-lamp lightmap + cell AO, and a per-slab GPU cache.
// The game's own modules are included first with the functions LIMEN replaces renamed to
// legacy*, LIMEN's versions are defined under the original names, then game_core.h (the game
// itself) is compiled against them.
//
// With -DLIMEN_SHOTS the same translation unit becomes a frame grabber (tools/limen_shots.cpp).

#define _USE_MATH_DEFINES
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <unordered_map>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// The game puts the rig down in rigPlace() and asks for the first fault delay right after it
// has fixed the start cell and the heading. That call is where the narrator learns the lane.
void lmNarRigHook();
#define rigFaultDelay legacyRigFaultDelay
#include "rig_rules.h"
#undef rigFaultDelay
inline float rigFaultDelay(int level, float roll01) {
    lmNarRigHook();
    return legacyRigFaultDelay(level, roll01);
}

#include "math.h"
#define persp(fov, asp, n, f) persp(fov, asp, n, ((f) == 100.0f ? 320.0f : (f)))

#define fillAudio legacyFillAudio
#define updateGameplayAudioState legacyUpdateGameplayAudioState
#include "audio.h"
#undef fillAudio
#undef updateGameplayAudioState

#include "limen_audio.h"

void lmSyncPlayerElevationAndAudio();
inline void updateGameplayAudioState(float moveIntensity, float sprintIntensity, float staminaNorm,
                                     float monsterProximity, float monsterMenace, float monsterType = 0.0f) {
    legacyUpdateGameplayAudioState(moveIntensity, sprintIntensity, staminaNorm,
                                   monsterProximity, monsterMenace, monsterType);
    lmSyncPlayerElevationAndAudio();
}

#include "shaders.h"

#define initQuad legacyInitQuad
#include "render.h"
#undef initQuad

#define generateChunk legacyGenerateChunk
#define updateVisibleChunks legacyUpdateVisibleChunks
#define updateLightsAndPillars legacyUpdateLightsAndPillars
#define reshuffleBehind legacyReshuffleBehind
#define chunkStyleAt legacyChunkStyleAt
#define collideWorld legacyCollideWorld
#define floorCanOpen legacyFloorCanOpen
#include "world.h"
#undef floorCanOpen
#undef generateChunk
#undef updateVisibleChunks
#undef updateLightsAndPillars
#undef reshuffleBehind
#undef chunkStyleAt
#undef collideWorld

#include "limen_hooks.h"

// Trap holes and the abyss are cut by the per-cell floor mesher (limen_mega_frame.h). Vault
// floors, terraces and pits come whole from the slab cache, where a hole would only be a fall
// through a floor that is still drawn: the game opens none there.
inline bool floorCanOpen(int wx, int wz) { return !lmHighAt(wx, wz) && !lmIsPitTag(lmTagAt(wx, wz)); }

// Falling (fall_sim.h) asks the world one thing: is the floor missing in this cell. LIMEN's
// shafts are missing floor too, so a body going down one slides along its real walls.
inline bool lmIsVoidCell(int wx, int wz) { return lmTagAt(wx, wz) == LT_VOID; }
#define fallStep legacyFallStep
#include "fall_sim.h"
#undef fallStep
template <class OpenFn>
inline void fallStep(FallState& s, float dt, float cs, OpenFn isOpen) {
    legacyFallStep(s, dt, cs, [&](int wx, int wz) { return isOpen(wx, wz) || lmIsVoidCell(wx, wz); });
}

#define updateMapContent legacyUpdateMapContent
#include "map_content.h"
#undef updateMapContent

#include "limen_props.h"

// The game's own world mesher (shell + props + z-fight repair of the whole view) is replaced
// wholesale by `lmPostBuildGeom`, which overwrites every buffer it fills. Building it anyway
// cost ~0.9 s per chunk crossing, so `buildGeom` gets empty stand-ins instead.
#define buildWorldBaseMesh legacyBuildWorldBaseMesh
#define buildWorldLampMesh legacyBuildWorldLampMesh
#define resolveWorldMesh legacyResolveWorldMesh
#include "world_mesh.h"
#undef buildWorldBaseMesh
#undef buildWorldLampMesh
#undef resolveWorldMesh
template <class HoleFn> inline void buildWorldBaseMesh(WorldMesh&, int, int, HoleFn) {}
inline void buildWorldLampMesh(WorldMesh&) {}
inline ZfReport resolveWorldMesh(WorldMesh&) { return ZfReport{}; }

inline void initQuad(GLuint& vao, GLuint& vbo) {
    legacyInitQuad(vao, vbo);
    lmPostBuildGeom();
}

#define gatherNearestSceneLights legacyGatherNearestSceneLights
#include "perf_tuning.h"
#undef gatherNearestSceneLights

inline int gatherNearestSceneLights(const std::vector<Light>& sceneLights, const Vec3& camPos, float outPos[SCENE_LIGHT_LIMIT * 3]) {
    return lmGatherLightsAndDrawMega(sceneLights, camPos, outPos);
}

#ifdef LIMEN_SHOTS
#define main backrooms_game_main
#include "game_core.h"
#undef main
#include "limen_mega.h"
#include "limen_narrator_game.h"
#else
// The F10 debug menu is LIMEN's own (limen_debug.h): the game's key polls and its one buffer
// swap go through two shims, which is all the menu needs to take keys and draw last.
int lmDbgGetKey(GLFWwindow* w, int key);
void lmDbgSwapBuffers(GLFWwindow* w);
#define glfwGetKey(w, k) lmDbgGetKey(w, k)
#define glfwSwapBuffers(w) lmDbgSwapBuffers(w)
#include "game_core.h"
#undef glfwGetKey
#undef glfwSwapBuffers
#include "limen_mega.h"
#include "limen_narrator_game.h"
#include "limen_debug.h"
#endif

// Skull above the eye point, and how far the player ducks under a lintel without being asked.
const float LM_HEAD_ABOVE_EYE = 0.14f;
const float LM_AUTO_DUCK = 0.14f;

// Highest eye height (above the floor) that still fits under the lintels over (x, z).
inline float lmEyeCapAt(float x, float z) {
    float lb = lmLintelBottomAt(x, z, PR);
    if (lb > 1e8f) return 1e9f;
    float cap = lb - LM_HEAD_ABOVE_EYE - lmGroundHeightAt(x, z, CS);
    return cap < PH_CROUCH ? PH_CROUCH : cap;   // a crouched player always fits
}

// A lintel stops the player on foot when the head is clearly above it: crouch to get through.
// Small deficits are ducked automatically (see the clamp below). Once under a lintel nothing is
// blocked any more, so standing up inside a doorway can never trap anyone.
bool lmPlayerHeadBlocked(float x, float z, float r) {
    if (debugTools.flyMode || playerFalling || fabsf(r - PR) > 1e-4f) return false;
    float dx = x - cam.pos.x, dz = z - cam.pos.z;
    if (dx * dx + dz * dz > 1.5f * 1.5f) return false;   // a probe elsewhere, not a step
    float cap = lmEyeCapAt(x, z);
    if (cam.curH <= cap + LM_AUTO_DUCK) return false;
    if (lmLintelBottomAt(cam.pos.x, cam.pos.z, PR) < 1e8f) return false;
    return true;
}

// The game's player walks on a leg simulation that knows only a floor at y = 0, and the picture
// is taken from the eyes of that body (legsBeginView), not from cam.pos. So the ground height is
// given to the body itself: the eyes go up by it for good (they are computed anew by every
// walk), the drawn skeleton only between two walks, each foot onto the step it stands on.
// lmLegsUnlift() runs right before the next simulation step (limen_debug.h, the sprint key poll).
struct LmLegLift { bool on = false; float orig[5] = {}, put[5] = {}; };
inline LmLegLift gLmLegLift;
inline float* lmLegLiftField(int k) {
    LegSim& s = gLegs.sim;
    return k == 0 ? &s.pelvisPos.y : (k <= 2 ? &s.hip[k - 1].y : &s.foot[k - 3].pose.anchor.y);
}
void lmLegsUnlift() {
    LmLegLift& L = gLmLegLift;
    if (!L.on) return;
    L.on = false;
    // (a body put down anew in the meantime stands on fresh numbers: leave those alone)
    for (int k = 0; k < 5; k++) { float* f = lmLegLiftField(k); if (*f == L.put[k]) *f = L.orig[k]; }
}
inline void lmLegsLift(float g) {
    lmLegsUnlift();
    if (!gLegs.live || !gLegs.placed) return;
    gLegs.eye.y += g;
    LmLegLift& L = gLmLegLift;
    for (int k = 0; k < 5; k++) {
        float* f = lmLegLiftField(k);
        float lift = g;
        if (k >= 3) {
            const Vec3& a = gLegs.sim.foot[k - 3].pose.anchor;
            float fg = lmGroundHeightAt(a.x, a.z, CS);
            lift = fg < g - 0.45f ? g - 0.45f : (fg > g + 0.45f ? g + 0.45f : fg);
        }
        L.orig[k] = *f; *f += lift; L.put[k] = *f;
    }
    L.on = true;
}

inline void lmSyncPlayerElevationAndAudio() {
    if (!debugTools.flyMode && !playerFalling) {
        // The game has just written cam.pos.y = curH + bob (walking) or eased last frame's value
        // toward curH (standing), knowing nothing of the ground. Recover the bob alone, so the
        // ground offset is never fed back into it (that made the camera saw on gentle slopes).
        static float prevY = 1e9f, prevBob = 0.0f, smoothG = 0.0f;
        static bool hasSmoothG = false;
        float raw = cam.pos.y - cam.curH;
        float bob = (fabsf(raw - (prevY - cam.curH) * 0.9f) < 1e-4f) ? prevBob * 0.9f : raw;
        if (fabsf(bob) > 0.06f) bob = 0.0f;
        float targetG = lmGroundHeightAt(cam.pos.x, cam.pos.z, CS);
        if (!hasSmoothG || fabsf(targetG - smoothG) > 1.6f) { smoothG = targetG; hasSmoothG = true; }
        else { float k = fminf(1.0f, fmaxf(dTime, 0.001f) * 14.0f); smoothG += (targetG - smoothG) * k; }
        float cap = lmEyeCapAt(cam.pos.x, cam.pos.z);
        if (cam.curH > cap) cam.curH = cap;               // head stays under the lintel
        cam.pos.y = cam.curH + bob + smoothG;
        prevY = cam.pos.y; prevBob = bob;
        lmLegsLift(smoothG);
    } else lmLegsUnlift();
    lmNarFrame();
    lmUpdateSpatialAudio(cam.pos.x, cam.pos.z, cam.yaw, CS);
}
