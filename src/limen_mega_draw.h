#pragma once
// Part of limen_mega.h: lights gathering and drawing the world.

inline int lmGatherLightsAndDrawMega(const std::vector<Light>& sceneLights, const Vec3& camPos, float* outPos) {
    lmReset(worldSeed, gCurrentLevel);
    lmRunFrameSlice();
    int cwx = (int)floorf(camPos.x / CS), cwz = (int)floorf(camPos.z / CS);

#ifndef LIMEN_SHOTS
    // The game's own fall (fall_sim.h) with LIMEN's shafts as the hole: the floor holds until the
    // body is past the lip, then it goes over with its walking speed (the rig bridges voids).
    if (!playerFalling && !debugTools.flyMode && lmTagAt(cwx, cwz) == LT_VOID && !rigPosInCabin(cam.pos) &&
        !fallSupported(cam.pos.x, cam.pos.z, CS, lmIsVoidCell)) {
        gFall = fallBegin(cam.pos, legsVelocity(), cam.yaw, cam.pitch);
        playerFalling = true;
        fallVelocity = 0.0f;
        fallTimer = 0.0f;
        setEchoStatus("THE VOID SWALLOWS YOU.");
    }
#endif

    float megaEnv = lmHighAt(cwx, cwz) ? 1.0f : 0.0f;
    if (megaEnv < 0.5f) {
        float bestD2 = 1e9f;
        for (int dx = -4; dx <= 4; dx++) {
            for (int dz = -4; dz <= 4; dz++) {
                if (!lmHighAt(cwx + dx, cwz + dz)) continue;
                float cx = (cwx + dx + 0.5f) * CS - camPos.x, cz = (cwz + dz + 0.5f) * CS - camPos.z;
                float d2 = cx * cx + cz * cz;
                if (d2 < bestD2) bestD2 = d2;
            }
        }
        if (bestD2 < 484.0f) megaEnv = 1.0f - sqrtf(bestD2) / 22.0f;
    }

    const float maxD2 = 42.0f * 42.0f;
    float bestDist2[SCENE_LIGHT_LIMIT];
    Vec3 bestPos[SCENE_LIGHT_LIMIT];
    int count = 0;
    for (const auto& l : sceneLights) {
        if (!l.on) continue;
        float d2 = (l.pos - camPos).lenSq();
        if (d2 > maxD2) continue;
        if (count < SCENE_LIGHT_LIMIT) {
            int k = count++;
            while (k > 0 && bestDist2[k - 1] > d2) { bestDist2[k] = bestDist2[k - 1]; bestPos[k] = bestPos[k - 1]; k--; }
            bestDist2[k] = d2; bestPos[k] = l.pos;
        } else if (d2 < bestDist2[count - 1]) {
            int k = count - 1;
            while (k > 0 && bestDist2[k - 1] > d2) { bestDist2[k] = bestDist2[k - 1]; bestPos[k] = bestPos[k - 1]; k--; }
            bestDist2[k] = d2; bestPos[k] = l.pos;
        }
    }
    for (int i = 0; i < count; i++) {
        outPos[i * 3 + 0] = bestPos[i].x;
        outPos[i * 3 + 1] = bestPos[i].y;
        outPos[i * 3 + 2] = bestPos[i].z;
    }

    lmEnsureLevelTextures();

    float uLight[3] = {
        (float)gLimenDNA.lightRGB[0] / 255.0f,
        (float)gLimenDNA.lightRGB[1] / 255.0f,
        (float)gLimenDNA.lightRGB[2] / 255.0f
    };
    float uFog[3] = {
        (float)gLimenDNA.fogRGB[0] / 255.0f,
        (float)gLimenDNA.fogRGB[1] / 255.0f,
        (float)gLimenDNA.fogRGB[2] / 255.0f
    };
    float fogDX = 0.032f + (float)gLimenDNA.fogDensity * 0.068f / 255.0f;
    // Scale mega-space fog inversely with slab size so 80..180 m megastructures stay legible end-to-end!
    float spanMeters = (float)(gLimen.rowPitch + gLimen.colPitch) * 0.5f * CS;
    float fogScaleM  = 95.0f / fmaxf(spanMeters, 75.0f);
    float fogDY = (0.0046f + (float)gLimenDNA.megaFogDensity * 0.0076f / 255.0f) * fogScaleM;
    float uMegaFog[9], uMegaAmb[9];
    for (int p = 0; p < LM_MEGA_POLES; p++) {
        for (int c = 0; c < 3; c++) {
            float pr = (float)gLimenDNA.megaPoles[p].rgb[c] / 255.0f;
            uMegaFog[p * 3 + c] = uFog[c] * 0.56f + pr * 0.17f;
            uMegaAmb[p * 3 + c] = 0.30f + pr * 0.36f;
        }
    }
    float floorSheen = (float)gLimenDNA.floorSurf.sheen / 255.0f;

    // Bind `lmEnvTex` to Texture Unit 5 and upload DNA atmosphere uniforms to both shaders
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_2D, lmEnvTex);
    glActiveTexture(GL_TEXTURE0);
    float invExt = 1.0f / (float)(LM_ENV_SIZE * CS);
    float oxM = lmEnvOriginX * CS, ozM = lmEnvOriginZ * CS;

    GLint locMega  = glGetUniformLocation(mainShader, "megaEnv");
    GLint locEnv   = glGetUniformLocation(mainShader, "envMap");
    GLint locRect  = glGetUniformLocation(mainShader, "envRect");
    GLint locLCol  = glGetUniformLocation(mainShader, "uLightCol");
    GLint locFCol  = glGetUniformLocation(mainShader, "uFogCol");
    GLint locFDen  = glGetUniformLocation(mainShader, "uFogDens");
    GLint locMFog  = glGetUniformLocation(mainShader, "uMegaFog");
    GLint locMAmb  = glGetUniformLocation(mainShader, "uMegaAmb");
    GLint locFJ    = glGetUniformLocation(mainShader, "uFloorJoint");
    int fjSel = ((int)gLimenDNA.floorSurf.seamFreqX + (int)gLimenDNA.floorSeamStrength) / 2;
    float fjStep = (fjSel > 165) ? 1.25f : ((fjSel > 102) ? 2.50f : 5.00f);
    float fjAmt  = gLimenDNA.floorSeamStrength < 24 ? 0.0f : 0.11f + (float)gLimenDNA.floorSeamStrength * 0.22f / 255.0f;
    if (locMega  >= 0) glUniform1f(locMega, megaEnv);
    if (locEnv   >= 0) glUniform1i(locEnv, 5);
    if (locRect  >= 0) glUniform3f(locRect, oxM, ozM, invExt);
    if (locLCol  >= 0) glUniform3fv(locLCol, 1, uLight);
    if (locFCol  >= 0) glUniform3fv(locFCol, 1, uFog);
    if (locFDen  >= 0) glUniform3f(locFDen, fogDX, fogDY, floorSheen);
    if (locMFog  >= 0) glUniform3fv(locMFog, 3, uMegaFog);
    if (locMAmb  >= 0) glUniform3fv(locMAmb, 3, uMegaAmb);
    if (locFJ    >= 0) glUniform3f(locFJ, fjStep, fjAmt, 0.0f);
    GLint locMood = glGetUniformLocation(mainShader, "uMood");
    if (locMood  >= 0) glUniform3f(locMood, (float)gLimenDNA.ambientGain / 128.0f, (float)gLimenDNA.lampGain / 128.0f, 0.0f);

    // Level quirks: one block of uniforms for both shaders (layout: lmQuirkProjGLSL in limen_envmap.h)
    float uq[32];
    float skyTop[3], skyHor[3], pixRows;
    lmQuirkUniforms(uq, skyTop, skyHor, pixRows);
    float uqA[24], uqB[24];   // the minimal GL loader has no glUniform4fv: xyz and w travel as two vec3 arrays
    for (int k = 0; k < 8; k++) {
        uqA[k * 3] = uq[k * 4]; uqA[k * 3 + 1] = uq[k * 4 + 1]; uqA[k * 3 + 2] = uq[k * 4 + 2];
        uqB[k * 3] = uq[k * 4 + 3]; uqB[k * 3 + 1] = 0.0f; uqB[k * 3 + 2] = 0.0f;
    }
    {
        GLint locQA = glGetUniformLocation(mainShader, "uQA"), locQB = glGetUniformLocation(mainShader, "uQB");
        if (locQA >= 0) glUniform3fv(locQA, 8, uqA);
        if (locQB >= 0) glUniform3fv(locQB, 8, uqB);
        GLint locCam = glGetUniformLocation(mainShader, "uCam");
        if (locCam >= 0) glUniform3f(locCam, camPos.x, camPos.y, camPos.z);
        GLint locSkyT = glGetUniformLocation(mainShader, "uSkyTop");
        if (locSkyT >= 0) glUniform3fv(locSkyT, 1, skyTop);
        GLint locSkyH = glGetUniformLocation(mainShader, "uSkyHor");
        if (locSkyH >= 0) glUniform3fv(locSkyH, 1, skyHor);
        GLint locPass = glGetUniformLocation(mainShader, "uPass");
        if (locPass >= 0) glUniform1f(locPass, 0.0f);
    }
    gPostPixelRows = pixRows;

    if (lightShader != 0) {
        glUseProgram(lightShader);
        GLint lMega = glGetUniformLocation(lightShader, "megaEnv");
        GLint lEnv  = glGetUniformLocation(lightShader, "envMap");
        GLint lRect = glGetUniformLocation(lightShader, "envRect");
        GLint lLCol = glGetUniformLocation(lightShader, "uLightCol");
        GLint lFCol = glGetUniformLocation(lightShader, "uFogCol");
        GLint lFDen = glGetUniformLocation(lightShader, "uFogDens");
        GLint lMFog = glGetUniformLocation(lightShader, "uMegaFog");
        if (lMega >= 0) glUniform1f(lMega, megaEnv);
        if (lEnv  >= 0) glUniform1i(lEnv, 5);
        if (lRect >= 0) glUniform3f(lRect, oxM, ozM, invExt);
        if (lLCol >= 0) glUniform3fv(lLCol, 1, uLight);
        if (lFCol >= 0) glUniform3fv(lFCol, 1, uFog);
        if (lFDen >= 0) glUniform3f(lFDen, fogDX, fogDY, floorSheen);
        if (lMFog >= 0) glUniform3fv(lMFog, 3, uMegaFog);
        GLint lQA = glGetUniformLocation(lightShader, "uQA"), lQB = glGetUniformLocation(lightShader, "uQB");
        if (lQA >= 0) glUniform3fv(lQA, 8, uqA);
        if (lQB >= 0) glUniform3fv(lQB, 8, uqB);
        GLint lCam = glGetUniformLocation(lightShader, "uCam");
        if (lCam >= 0) glUniform3f(lCam, camPos.x, camPos.y, camPos.z);
        GLint lPass = glGetUniformLocation(lightShader, "uPass");
        if (lPass >= 0) glUniform1f(lPass, 0.0f);
        glUseProgram(mainShader);
    }

    // Everything drawn from here is lit exactly like the shell the game draws afterwards: same
    // rig lamp in the light list, same held-flashlight lens as the cone origin.
    float drawLp[SCENE_LIGHT_LIMIT * 3];
    for (int k = 0; k < count * 3; k++) drawLp[k] = outPos[k];
    int drawCount = rigInjectLight(drawLp, count, SCENE_LIGHT_LIMIT);
    GLint locNl = glGetUniformLocation(mainShader, "nl");
    GLint locLp = glGetUniformLocation(mainShader, "lp");
    if (locNl >= 0) glUniform1i(locNl, drawCount);
    if (drawCount > 0 && locLp >= 0) glUniform3fv(locLp, drawCount, drawLp);
    if (activeDeviceSlot == 1 && flashlightOn) {
        GLint locFlashPos = glGetUniformLocation(mainShader, "flashPos");
        if (locFlashPos >= 0) {
            float fx = sinf(cam.yaw), fz = cosf(cam.yaw);
            float fwd = vmHandFwd + vmFlashLensFwd, side = vmHandSide + vmFlashLensSide;
            glUniform3f(locFlashPos, cam.pos.x + fx * fwd + fz * side,
                        cam.pos.y + vmHandUp + vmFlashLensUp,
                        cam.pos.z + fz * fwd - fx * side);
        }
    }

    // Open-sky levels: the sky is a box around the camera drawn first, behind everything.
    if (lmHasQuirk(gLimenDNA, LQ_SKY)) {
        static GLuint skyVAO = 0, skyVBO = 0;
        if (skyVAO == 0) {
            std::vector<float> sv;
            lmPushBox(sv, -60.0f, -60.0f, -60.0f, 60.0f, 60.0f, 60.0f, 1.0f);
            setupVAO(skyVAO, skyVBO, sv, true);
        }
        GLint locPass = glGetUniformLocation(mainShader, "uPass");
        if (locPass >= 0) {
            glUniform1f(locPass, 1.0f);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);
            glBindVertexArray(skyVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glEnable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
            glUniform1f(locPass, 0.0f);
        }
    }

    // Mega-unstructures: the sky box is drawn in place of the roof (uPass = 1, uUnstruct.y = 2).
    GLint locUU = glGetUniformLocation(mainShader, "uUnstruct");
    GLint locUI = glGetUniformLocation(mainShader, "uUnstructInfo");
    {
        int skyKind = 0;
        LimenRect here = lmSlabAt(cwx, cwz);
        int hereMf = lmSlab(here.j, here.i).megaForm;
        if (lmIsMegaUnstruct(hereMf)) skyKind = lmUnstructKind(hereMf);
        for (long long skey : gLmVisibleSlabKeys) {
            if (skyKind) break;
            auto it = gLmSlabMeshes.find(skey);
            if (it != gLmSlabMeshes.end() && it->second.unstruct) skyKind = it->second.unstruct;
        }
        GLint locPass = glGetUniformLocation(mainShader, "uPass");
        if (skyKind > 0 && locUU >= 0 && locPass >= 0) {
            static GLuint uSkyVAO = 0, uSkyVBO = 0;
            if (uSkyVAO == 0) {
                std::vector<float> sv;
                lmPushBox(sv, -60.0f, -60.0f, -60.0f, 60.0f, 60.0f, 60.0f, 1.0f);
                setupVAO(uSkyVAO, uSkyVBO, sv, true);
            }
            glUniform3f(locUU, (float)skyKind, 2.0f, 0.0f);
            glUniform1f(locPass, 1.0f);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);
            glBindVertexArray(uSkyVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glEnable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
            glUniform1f(locPass, 0.0f);
            glUniform3f(locUU, 0.0f, 0.0f, 0.0f);
        }
    }

    // Draw cached per-slab megastructure, grafted pod walls, and static slab decor with culling!
    float fwdX = sinf(cam.yaw), fwdZ = cosf(cam.yaw);
    for (long long skey : gLmVisibleSlabKeys) {
        auto it = gLmSlabMeshes.find(skey);
        if (it == gLmSlabMeshes.end()) continue;
        const LmCachedSlabMesh& cm = it->second;
        if (!lmSlabVisibleFrom(cm.rect, camPos, fwdX, fwdZ)) continue;
        if (locUU >= 0) glUniform3f(locUU, (float)cm.unstruct, 0.0f, 0.0f);
        if (locUI >= 0) glUniform3f(locUI, cm.uinfo[0], cm.uinfo[1], cm.uinfo[2]);
        if (cm.megaVC > 0) {
            glBindTexture(GL_TEXTURE_2D, cm.unstruct ? lmUnstructSlotTex(cm.unstruct, 0) : lmEnsureMegaTex(cm.palette));
            glBindVertexArray(cm.megaVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.megaVC);
        }
        if (cm.skyVC > 0 && locUU >= 0) {
            glUniform3f(locUU, (float)cm.unstruct, 2.0f, 0.0f);
            glDisable(GL_CULL_FACE);
            glBindVertexArray(cm.skyVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.skyVC);
            glEnable(GL_CULL_FACE);
            glUniform3f(locUU, (float)cm.unstruct, 0.0f, 0.0f);
        }
        if (cm.megaAltVC > 0) {
            glBindTexture(GL_TEXTURE_2D, cm.unstruct ? lmUnstructSlotTex(cm.unstruct, 1) : lmEnsureMegaTex(cm.altPalette));
            glBindVertexArray(cm.megaAltVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.megaAltVC);
        }
        if (cm.wallVC > 0) {
            glBindTexture(GL_TEXTURE_2D, cm.unstruct ? lmUnstructSlotTex(cm.unstruct, 2) : wallTex);
            glBindVertexArray(cm.wallVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.wallVC);
        }
        if (cm.dadoVC > 0) {
            glBindTexture(GL_TEXTURE_2D, lmDadoTex);
            glBindVertexArray(cm.dadoVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.dadoVC);
        }
        if (cm.vitFloorVC > 0) {
            glBindTexture(GL_TEXTURE_2D, floorTex);
            glBindVertexArray(cm.vitFloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.vitFloorVC);
        }
        if (cm.vitCeilVC > 0) {
            glDisable(GL_CULL_FACE);
            glBindTexture(GL_TEXTURE_2D, ceilTex);
            glBindVertexArray(cm.vitCeilVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.vitCeilVC);
            glEnable(GL_CULL_FACE);
        }
        if (cm.xeno.active && (cm.xenoWallVC > 0 || cm.xenoDadoVC > 0 || cm.xenoFloorVC > 0 || cm.xenoCeilVC > 0)) {
            GLuint xwTex = 0, xfTex = 0, xcTex = 0, xdTex = 0;
            lmEnsureXenoTextures(cm.xeno, xwTex, xfTex, xcTex, xdTex);
            if (cm.xenoWallVC > 0) {
                glBindTexture(GL_TEXTURE_2D, xwTex);
                glBindVertexArray(cm.xenoWallVAO);
                glDrawArrays(GL_TRIANGLES, 0, cm.xenoWallVC);
            }
            if (cm.xenoDadoVC > 0) {
                glBindTexture(GL_TEXTURE_2D, xdTex);
                glBindVertexArray(cm.xenoDadoVAO);
                glDrawArrays(GL_TRIANGLES, 0, cm.xenoDadoVC);
            }
            if (cm.xenoFloorVC > 0) {
                glBindTexture(GL_TEXTURE_2D, xfTex);
                glBindVertexArray(cm.xenoFloorVAO);
                glDrawArrays(GL_TRIANGLES, 0, cm.xenoFloorVC);
            }
            if (cm.xenoCeilVC > 0) {
                glDisable(GL_CULL_FACE);
                glBindTexture(GL_TEXTURE_2D, xcTex);
                glBindVertexArray(cm.xenoCeilVAO);
                glDrawArrays(GL_TRIANGLES, 0, cm.xenoCeilVC);
                glEnable(GL_CULL_FACE);
            }
        }
        if (cm.decorVC > 0) {
            glDisable(GL_CULL_FACE);
            glBindTexture(GL_TEXTURE_2D, decorTex);
            glBindVertexArray(cm.decorVAO);
            glDrawArrays(GL_TRIANGLES, 0, cm.decorVC);
            glEnable(GL_CULL_FACE);
        }
    }

    if (locUU >= 0) glUniform3f(locUU, 0.0f, 0.0f, 0.0f);

    // Props and POI sets, one cached mesh per chunk, culled like the slabs they stand in
    if (!gLmChunkPropMeshes.empty()) {
        glDisable(GL_CULL_FACE);
        glBindTexture(GL_TEXTURE_2D, decorTex);
        for (const auto& kv : gLmChunkPropMeshes) {
            const LmChunkPropMesh& pm = kv.second;
            if (pm.vc <= 0) continue;
            LimenRect pr{};
            pr.x0 = pm.tx * LM_PROP_TILE - 1; pr.x1 = (pm.tx + 1) * LM_PROP_TILE;   // one cell of slack
            pr.z0 = pm.tz * LM_PROP_TILE - 1; pr.z1 = (pm.tz + 1) * LM_PROP_TILE;   // for footprints
            if (!lmSlabVisibleFrom(pr, camPos, fwdX, fwdZ)) continue;
            glBindVertexArray(pm.vao);
            glDrawArrays(GL_TRIANGLES, 0, pm.vc);
        }
        glEnable(GL_CULL_FACE);
    }
    return count;
}
