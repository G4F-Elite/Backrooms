#pragma once
// LIMEN shaders, continuous procedural material synthesizer, lazy per-(seed, level)
// texture manager, and the 128x128 world-space environment map (`lmEnvTex`: R = unlimited
// far-lamp lightmap, G = cell AO, B = continuous LG_HUE pole blend, A = smooth vault factor).
#include "limen_vestige.h"

inline GLuint lmMegaTex[LM_MEGA_POLES] = {0, 0, 0};
inline GLuint lmDadoTex = 0;
inline GLuint lmEnvTex = 0;

enum {
    UTEX_MONOLITH = 0,
    UTEX_MONOLITH_ALT,
    UTEX_SNOW,
    UTEX_BASALT,
    UTEX_CAVE_ROCK,
    UTEX_CRIMSON_LEAF,
    UTEX_EARTH,
    UTEX_EARTH_WARM,
    UTEX_DRY_GRASS,
    UTEX_SKY,
    LM_UNSTRUCT_TEX_COUNT
};
inline GLuint lmUnstructTex[LM_UNSTRUCT_TEX_COUNT] = {};
inline int lmEnvOriginX = -999999, lmEnvOriginZ = -999999;
inline unsigned int lmEnvBuiltSeed = 0, lmEnvBuiltEpoch = 0;
inline int lmEnvBuiltLevel = -999999999;

inline unsigned int lmTexBuiltSeed = 0;
inline int lmTexBuiltLevel = -999999999;
inline bool lmTexBuiltValid = false;

const int LM_ENV_SIZE = 128;   // 128x128 cells = 640x640 m around player

#include "limen_scene_shader.h"

inline bool lmPatchShadersOnce() {
    static std::string mainVsSrc, lightVsSrc;
    auto withProj = [](const char* body) {
        std::string s = body;
        const std::string mark = "//QUIRK_PROJ";
        size_t p = s.find(mark);
        if (p != std::string::npos) s.replace(p, mark.size(), lmQuirkProjGLSL);
        return s;
    };
    mainVsSrc = withProj(lmMainVSBody);
    lightVsSrc = withProj(lmLightVSBody);
    mainVS = mainVsSrc.c_str();
    mainFS = lmMainFS;
    lightVS = lightVsSrc.c_str();
    lightFS = lmLightFS;
    return true;
}
inline bool gLmShadersReady = lmPatchShadersOnce();

#include "limen_envmap_bake.h"
#include "limen_surface_textures.h"
