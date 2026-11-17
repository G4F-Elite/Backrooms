#pragma once
// Scene data for the screen-space effects in the post shader (SSAO, GI, god rays, bloom).
// renderScene() captures the camera projection and the visible lamps every frame;
// the post pass uploads them together with the EFFECTS settings.

inline constexpr int POST_FX_RAY_LIGHTS = 4;

struct PostFxFrame {
    bool sceneDrawn = false;
    float tanHalfX = 1.0f;
    float tanHalfY = 1.0f;
    int rayCount = 0;
    float rays[POST_FX_RAY_LIGHTS * 3] = {0};
};
inline PostFxFrame gPostFx;

// Remembers the projection and picks the lamps whose shafts are worth drawing:
// in front of the camera, on screen (or just outside it) and not too far away.
inline void postFxCaptureScene(const Mat4& proj, const Mat4& view, const float* lightPos, int lightCount) {
    gPostFx.sceneDrawn = true;
    gPostFx.tanHalfX = 1.0f / proj.m[0];
    gPostFx.tanHalfY = 1.0f / proj.m[5];
    gPostFx.rayCount = 0;
    float strength[POST_FX_RAY_LIGHTS] = {0};
    for (int i = 0; i < lightCount; i++) {
        Vec3 v = view.transformPoint(Vec3(lightPos[i * 3], lightPos[i * 3 + 1], lightPos[i * 3 + 2]));
        float depth = -v.z;
        if (depth < 0.5f || depth > 30.0f) continue;
        float u = proj.m[0] * v.x / depth * 0.5f + 0.5f;
        float w = proj.m[5] * v.y / depth * 0.5f + 0.5f;
        float outside = fmaxf(fmaxf(-u, u - 1.0f), fmaxf(-w, w - 1.0f));
        if (outside > 0.25f) continue;
        float s = (1.0f - depth / 30.0f) * (outside > 0.0f ? 1.0f - outside / 0.25f : 1.0f);
        int slot = gPostFx.rayCount;
        if (slot < POST_FX_RAY_LIGHTS) {
            gPostFx.rayCount++;
        } else {
            slot = 0;
            for (int k = 1; k < POST_FX_RAY_LIGHTS; k++) {
                if (strength[k] < strength[slot]) slot = k;
            }
            if (strength[slot] >= s) continue;
        }
        strength[slot] = s;
        gPostFx.rays[slot * 3] = u;
        gPostFx.rays[slot * 3 + 1] = w;
        gPostFx.rays[slot * 3 + 2] = s;
    }
}

// Uploads the effect settings. With enabled == false (menus, the final TAA blit)
// every effect is switched off so nothing is applied twice or to a blank frame.
inline void postFxSetUniforms(GLuint shader, bool enabled) {
    static GLuint cachedShader = 0;
    static GLint ssaoLoc = -1, giLoc = -1, raysLoc = -1, bloomLoc = -1;
    static GLint denoiseLoc = -1, denoiseStrLoc = -1, projLoc = -1, rayCountLoc = -1, rayPosLoc = -1;
    if (cachedShader != shader) {
        cachedShader = shader;
        ssaoLoc = glGetUniformLocation(shader, "rtxA");
        giLoc = glGetUniformLocation(shader, "rtxG");
        raysLoc = glGetUniformLocation(shader, "rtxR");
        bloomLoc = glGetUniformLocation(shader, "rtxB");
        denoiseLoc = glGetUniformLocation(shader, "rtxDenoiseOn");
        denoiseStrLoc = glGetUniformLocation(shader, "rtxDenoiseStrength");
        projLoc = glGetUniformLocation(shader, "projInfo");
        rayCountLoc = glGetUniformLocation(shader, "rayN");
        rayPosLoc = glGetUniformLocation(shader, "rayL");
    }
    const bool on = enabled && gPostFx.sceneDrawn;
    const int rayCount = on && settings.godRays ? gPostFx.rayCount : 0;
    glUniform1i(ssaoLoc, on ? clampSsao(settings.ssaoQuality) : 0);
    glUniform1i(giLoc, on ? clampGi(settings.giQuality) : 0);
    glUniform1i(raysLoc, rayCount > 0 ? 1 : 0);
    glUniform1i(bloomLoc, on && settings.bloom ? 1 : 0);
    glUniform1i(denoiseLoc, settings.rtxDenoise ? 1 : 0);
    glUniform1f(denoiseStrLoc, settings.rtxDenoiseStrength);
    glUniform3f(projLoc, gPostFx.tanHalfX, gPostFx.tanHalfY, 0.0f);
    glUniform1i(rayCountLoc, rayCount);
    if (rayCount > 0) glUniform3fv(rayPosLoc, rayCount, gPostFx.rays);
}
