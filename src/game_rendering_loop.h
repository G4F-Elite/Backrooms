#pragma once
void renderScene(){
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    struct MainUniforms {
        GLint P, V, M, vp, tm, danger, flashOn, flashDir, rfc, rfp, rfd, nl, lp;
    };
    struct LightUniforms {
        GLint P, V, M, inten, tm, fade;
    };
    static MainUniforms mu = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    static LightUniforms lu = {-1,-1,-1,-1,-1,-1};
    if(mu.P < 0){
        mu.P = glGetUniformLocation(mainShader,"P");
        mu.V = glGetUniformLocation(mainShader,"V");
        mu.M = glGetUniformLocation(mainShader,"M");
        mu.vp = glGetUniformLocation(mainShader,"vp");
        mu.tm = glGetUniformLocation(mainShader,"tm");
        mu.danger = glGetUniformLocation(mainShader,"danger");
        mu.flashOn = glGetUniformLocation(mainShader,"flashOn");
        mu.flashDir = glGetUniformLocation(mainShader,"flashDir");
        mu.rfc = glGetUniformLocation(mainShader,"rfc");
        mu.rfp = glGetUniformLocation(mainShader,"rfp");
        mu.rfd = glGetUniformLocation(mainShader,"rfd");
        mu.nl = glGetUniformLocation(mainShader,"nl");
        mu.lp = glGetUniformLocation(mainShader,"lp");
    }
    if(lu.P < 0){
        lu.P = glGetUniformLocation(lightShader,"P");
        lu.V = glGetUniformLocation(lightShader,"V");
        lu.M = glGetUniformLocation(lightShader,"M");
        lu.inten = glGetUniformLocation(lightShader,"inten");
        lu.tm = glGetUniformLocation(lightShader,"tm");
        lu.fade = glGetUniformLocation(lightShader,"fade");
    }

    glUseProgram(mainShader);
    Mat4 proj=Mat4::persp(1.2f,(float)winW/winH,0.1f,100.0f);
    float shX=camShake*(rand()%100-50)/500.0f,shY=camShake*(rand()%100-50)/500.0f;
    Vec3 la=cam.pos+Vec3(mSin(cam.yaw+shX)*mCos(cam.pitch+shY),mSin(cam.pitch+shY),
                         mCos(cam.yaw+shX)*mCos(cam.pitch+shY));
    Mat4 view=Mat4::look(cam.pos,la,Vec3(0,1,0)),model;
    
    glUniformMatrix4fv(mu.P,1,GL_FALSE,proj.m);
    glUniformMatrix4fv(mu.V,1,GL_FALSE,view.m);
    glUniformMatrix4fv(mu.M,1,GL_FALSE,model.m);
    glUniform3f(mu.vp,cam.pos.x,cam.pos.y,cam.pos.z);
    glUniform1f(mu.tm,vhsTime);
    glUniform1f(mu.danger,entityMgr.dangerLevel);
    bool flashVisualOn = flashlightOn;
    if(flashlightOn && flashlightShutdownBlinkActive){
        flashVisualOn = isFlashlightOnDuringShutdownBlink(flashlightShutdownBlinkTimer);
    }
    glUniform1i(mu.flashOn,flashVisualOn?1:0);
    glUniform3f(mu.flashDir,mSin(cam.yaw)*mCos(cam.pitch),
                mSin(cam.pitch),mCos(cam.yaw)*mCos(cam.pitch));
    float remoteFlashPos[12] = {0};
    float remoteFlashDir[12] = {0};
    int remoteFlashCount = 0;
    if(multiState==MULTI_IN_GAME){
        remoteFlashCount = gatherRemoteFlashlights(netMgr.myId, remoteFlashPos, remoteFlashDir);
    }
    glUniform1i(mu.rfc,remoteFlashCount);
    if(remoteFlashCount>0){
        glUniform3fv(mu.rfp,remoteFlashCount,remoteFlashPos);
        glUniform3fv(mu.rfd,remoteFlashCount,remoteFlashDir);
    }
    
    // Gather nearest lights - fade is now computed in shader based on distance
    float lpos[SCENE_LIGHT_LIMIT * 3] = {0};
    int nl = gatherNearestSceneLights(lights, cam.pos, lpos);
    glUniform1i(mu.nl,nl);
    if(nl>0) glUniform3fv(mu.lp,nl,lpos);
    
    glBindTexture(GL_TEXTURE_2D,wallTex);glBindVertexArray(wallVAO);glDrawArrays(GL_TRIANGLES,0,wallVC);
    glBindVertexArray(pillarVAO);glDrawArrays(GL_TRIANGLES,0,pillarVC);
    if(decorVC>0){
        glDisable(GL_CULL_FACE);
        glBindTexture(GL_TEXTURE_2D,propTex);
        glBindVertexArray(decorVAO);glDrawArrays(GL_TRIANGLES,0,decorVC);
        glEnable(GL_CULL_FACE);
    }
    glBindTexture(GL_TEXTURE_2D,floorTex);glBindVertexArray(floorVAO);glDrawArrays(GL_TRIANGLES,0,floorVC);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D,ceilTex);glBindVertexArray(ceilVAO);glDrawArrays(GL_TRIANGLES,0,ceilVC);
    if(noteVC>0){glBindTexture(GL_TEXTURE_2D,lightTex);glBindVertexArray(noteVAO);glDrawArrays(GL_TRIANGLES,0,noteVC);}
    glEnable(GL_CULL_FACE);
    
    if(multiState==MULTI_IN_GAME && playerModelsInit){
        renderPlayers(mainShader, proj, view, netMgr.myId);
    }
    
    glUseProgram(lightShader);
    glUniformMatrix4fv(lu.P,1,GL_FALSE,proj.m);
    glUniformMatrix4fv(lu.V,1,GL_FALSE,view.m);
    glUniformMatrix4fv(lu.M,1,GL_FALSE,model.m);
    glUniform1f(lu.inten,1.2f);
    glUniform1f(lu.tm,vhsTime);
    glUniform1f(lu.fade,1.0f); // Light sprites always full brightness when visible
    glBindTexture(GL_TEXTURE_2D,lampTex);glBindVertexArray(lightVAO);glDrawArrays(GL_TRIANGLES,0,lightVC);
    if(lightOffVC>0){
        glUniform1f(lu.inten,0.15f);
        glUniform1f(lu.fade,1.0f);
        glBindVertexArray(lightOffVAO);glDrawArrays(GL_TRIANGLES,0,lightOffVC);
    }
    entityMgr.render(mainShader,proj,view);
    
    // RTX deferred passes (SSAO + SSR + Volumetric + Composition)
    if (rtxPipeline.initialized && isRtxEnabled(settings.rtxMode)) {
        const RtxConfig& rc = rtxPipeline.config;
        
        // === PASS 1: G-Buffer ===
        // Re-render geometry to G-buffer with PBR normal/material maps
        glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.gbuf.fbo);
        glViewport(0, 0, rtxPipeline.gbuf.width, rtxPipeline.gbuf.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(rtxPipeline.gbufShader);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.gbufShader, "P"), 1, GL_FALSE, proj.m);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.gbufShader, "V"), 1, GL_FALSE, view.m);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.gbufShader, "M"), 1, GL_FALSE, model.m);
        glUniform3f(glGetUniformLocation(rtxPipeline.gbufShader, "vp"), cam.pos.x, cam.pos.y, cam.pos.z);
        
        GLint gbTexLoc = glGetUniformLocation(rtxPipeline.gbufShader, "tex");
        GLint gbNmLoc = glGetUniformLocation(rtxPipeline.gbufShader, "normalMap");
        GLint gbMatLoc = glGetUniformLocation(rtxPipeline.gbufShader, "materialMap");
        GLint gbSurfLoc = glGetUniformLocation(rtxPipeline.gbufShader, "surfaceType");
        
        glUniform1i(gbTexLoc, 0);
        glUniform1i(gbNmLoc, 1);
        glUniform1i(gbMatLoc, 2);
        
        // Wall
        glUniform1i(gbSurfLoc, 0);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, pbrWall.normalMap);
        glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, pbrWall.materialMap);
        glBindVertexArray(wallVAO); glDrawArrays(GL_TRIANGLES, 0, wallVC);
        glBindVertexArray(pillarVAO); glDrawArrays(GL_TRIANGLES, 0, pillarVC);
        
        // Floor
        glUniform1i(gbSurfLoc, 1);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, floorTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, pbrFloor.normalMap);
        glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, pbrFloor.materialMap);
        glBindVertexArray(floorVAO); glDrawArrays(GL_TRIANGLES, 0, floorVC);
        
        // Ceiling
        glUniform1i(gbSurfLoc, 2);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, ceilTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, pbrCeil.normalMap);
        glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, pbrCeil.materialMap);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(ceilVAO); glDrawArrays(GL_TRIANGLES, 0, ceilVC);
        glEnable(GL_CULL_FACE);
        
        // Decor/Props
        if (decorVC > 0) {
            glUniform1i(gbSurfLoc, 3);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, propTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, pbrProp.normalMap);
            glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, pbrProp.materialMap);
            glDisable(GL_CULL_FACE);
            glBindVertexArray(decorVAO); glDrawArrays(GL_TRIANGLES, 0, decorVC);
            glEnable(GL_CULL_FACE);
        }
        
        glActiveTexture(GL_TEXTURE0);
        
        // === PASS 2: SSAO ===
        glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.ssao.fbo);
        glViewport(0, 0, rtxPipeline.ssao.width, rtxPipeline.ssao.height);
        glClearColor(1.0f,1.0f,1.0f,1.0f); // Clear to 1.0 = no occlusion (not 0 = black!)
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.02f,0.02f,0.02f,1.0f); // Restore
        glDisable(GL_DEPTH_TEST);
        
        glUseProgram(rtxPipeline.ssaoShader);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.posTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.normTex);
        glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, rtxPipeline.ssao.noiseTex);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssaoShader, "gPosition"), 0);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssaoShader, "gNormal"), 1);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssaoShader, "noiseTex"), 2);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.ssaoShader, "proj"), 1, GL_FALSE, proj.m);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.ssaoShader, "view"), 1, GL_FALSE, view.m);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssaoShader, "sampleCount"), rc.ssao.samples);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssaoShader, "radius"), rc.ssao.radius);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssaoShader, "bias"), rc.ssao.bias);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssaoShader, "intensity"), rc.ssao.intensity);
        glUniform3f(glGetUniformLocation(rtxPipeline.ssaoShader, "noiseScale"),
                    (float)rtxPipeline.ssao.width / 4.0f, (float)rtxPipeline.ssao.height / 4.0f, 0.0f);
        glUniform3fv(glGetUniformLocation(rtxPipeline.ssaoShader, "kernel"), 32, rtxSsaoKernel);
        glUniform3f(glGetUniformLocation(rtxPipeline.ssaoShader, "viewPos"), cam.pos.x, cam.pos.y, cam.pos.z);
        
        glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // SSAO Blur passes
        for (int bp = 0; bp < rc.ssao.blurPasses; bp++) {
            // Horizontal
            glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.ssao.blurFbo);
            glClearColor(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.02f,0.02f,0.02f,1);
            glUseProgram(rtxPipeline.ssaoBlurShader);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, rtxPipeline.ssao.aoTex);
            glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.posTex);
            glUniform1i(glGetUniformLocation(rtxPipeline.ssaoBlurShader, "aoTex"), 0);
            glUniform1i(glGetUniformLocation(rtxPipeline.ssaoBlurShader, "gPosition"), 1);
            glUniform3f(glGetUniformLocation(rtxPipeline.ssaoBlurShader, "texelSize"),
                        1.0f / (float)rtxPipeline.ssao.width, 1.0f / (float)rtxPipeline.ssao.height, 0.0f);
            glUniform1i(glGetUniformLocation(rtxPipeline.ssaoBlurShader, "horizontal"), 1);
            glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Vertical
            glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.ssao.fbo);
            glClearColor(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.02f,0.02f,0.02f,1);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, rtxPipeline.ssao.blurTex);
            glUniform1i(glGetUniformLocation(rtxPipeline.ssaoBlurShader, "horizontal"), 0);
            glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        // === PASS 3: SSR ===
        glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.ssr.fbo);
        glViewport(0, 0, rtxPipeline.ssr.width, rtxPipeline.ssr.height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(rtxPipeline.ssrShader);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.posTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.normTex);
        glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, fboTex); // forward-rendered scene
        glUniform1i(glGetUniformLocation(rtxPipeline.ssrShader, "gPosition"), 0);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssrShader, "gNormal"), 1);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssrShader, "colorTex"), 2);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.ssrShader, "proj"), 1, GL_FALSE, proj.m);
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.ssrShader, "view"), 1, GL_FALSE, view.m);
        glUniform1i(glGetUniformLocation(rtxPipeline.ssrShader, "maxSteps"), rc.ssr.maxSteps);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssrShader, "stepSize"), rc.ssr.stepSize);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssrShader, "maxDist"), rc.ssr.maxDist);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssrShader, "thickness"), rc.ssr.thickness);
        glUniform1f(glGetUniformLocation(rtxPipeline.ssrShader, "fadeEdge"), rc.ssr.fadeEdge);
        glUniform3f(glGetUniformLocation(rtxPipeline.ssrShader, "viewPos"), cam.pos.x, cam.pos.y, cam.pos.z);
        
        glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // === PASS 4: Volumetric Light ===
        glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.vol.fbo);
        glViewport(0, 0, rtxPipeline.vol.width, rtxPipeline.vol.height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(rtxPipeline.volShader);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.posTex);
        glUniform1i(glGetUniformLocation(rtxPipeline.volShader, "gPosition"), 0);
        
        // Compute inverse view-projection for ray reconstruction
        Mat4 vp_mat;
        for (int i = 0; i < 16; i++) vp_mat.m[i] = 0;
        // Simple multiply proj * view
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                for (int k = 0; k < 4; k++)
                    vp_mat.m[r * 4 + c] += proj.m[r * 4 + k] * view.m[k * 4 + c];
        // We pass identity as placeholder - shader will use viewPos + screen UV
        glUniformMatrix4fv(glGetUniformLocation(rtxPipeline.volShader, "invViewProj"), 1, GL_FALSE, vp_mat.m);
        glUniform3f(glGetUniformLocation(rtxPipeline.volShader, "viewPos"), cam.pos.x, cam.pos.y, cam.pos.z);
        glUniform1i(glGetUniformLocation(rtxPipeline.volShader, "lightCount"), nl);
        if (nl > 0) glUniform3fv(glGetUniformLocation(rtxPipeline.volShader, "lightPositions"), nl, lpos);
        glUniform1i(glGetUniformLocation(rtxPipeline.volShader, "samples"), rc.volumetric.samples);
        glUniform1f(glGetUniformLocation(rtxPipeline.volShader, "density"), rc.volumetric.density);
        glUniform1f(glGetUniformLocation(rtxPipeline.volShader, "scatterPower"), rc.volumetric.scatterPower);
        glUniform1f(glGetUniformLocation(rtxPipeline.volShader, "volIntensity"), rc.volumetric.intensity);
        glUniform1f(glGetUniformLocation(rtxPipeline.volShader, "time"), vhsTime);
        
        glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // === PASS 5: Composition - render to comp.fbo (not fbo, to avoid read-write feedback) ===
        glBindFramebuffer(GL_FRAMEBUFFER, rtxPipeline.comp.fbo);
        glViewport(0, 0, rtxPipeline.comp.width, rtxPipeline.comp.height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(rtxPipeline.compShader);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.posTex);
        glActiveTexture(GL_TEXTURE0+1); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.normTex);
        glActiveTexture(GL_TEXTURE0+2); glBindTexture(GL_TEXTURE_2D, rtxPipeline.gbuf.albedoTex);
        glActiveTexture(GL_TEXTURE0+3); glBindTexture(GL_TEXTURE_2D, rtxPipeline.ssao.aoTex);
        glActiveTexture(GL_TEXTURE0+4); glBindTexture(GL_TEXTURE_2D, rtxPipeline.ssr.reflTex);
        glActiveTexture(GL_TEXTURE0+5); glBindTexture(GL_TEXTURE_2D, rtxPipeline.vol.scatterTex);
        glActiveTexture(GL_TEXTURE0+6); glBindTexture(GL_TEXTURE_2D, fboTex);
        
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "gPosition"), 0);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "gNormal"), 1);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "gAlbedo"), 2);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "ssaoTex"), 3);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "ssrTex"), 4);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "volTex"), 5);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "forwardTex"), 6);
        
        glUniform3f(glGetUniformLocation(rtxPipeline.compShader, "viewPos"), cam.pos.x, cam.pos.y, cam.pos.z);
        glUniform1f(glGetUniformLocation(rtxPipeline.compShader, "time"), vhsTime);
        glUniform1f(glGetUniformLocation(rtxPipeline.compShader, "danger"), entityMgr.dangerLevel);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "lightCount"), nl);
        if (nl > 0) glUniform3fv(glGetUniformLocation(rtxPipeline.compShader, "lightPositions"), nl, lpos);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "flashOn"), flashVisualOn ? 1 : 0);
        glUniform3f(glGetUniformLocation(rtxPipeline.compShader, "flashDir"),
                    mSin(cam.yaw)*mCos(cam.pitch), mSin(cam.pitch), mCos(cam.yaw)*mCos(cam.pitch));
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "remoteFc"), remoteFlashCount);
        if (remoteFlashCount > 0) {
            glUniform3fv(glGetUniformLocation(rtxPipeline.compShader, "remoteFp"), remoteFlashCount, remoteFlashPos);
            glUniform3fv(glGetUniformLocation(rtxPipeline.compShader, "remoteFd"), remoteFlashCount, remoteFlashDir);
        }
        glUniform1f(glGetUniformLocation(rtxPipeline.compShader, "giBounceMul"), rc.giBounceMul);
        glUniform1i(glGetUniformLocation(rtxPipeline.compShader, "contactShadows"), rc.contactShadows ? 1 : 0);
        
        glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_DEPTH_TEST);
    }
}

#include "hud.h"

inline int detectActiveRefreshRateHz(GLFWwindow* w){
    GLFWmonitor* mon = glfwGetWindowMonitor(w);
    if(!mon){
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetWindowPos(w, &wx, &wy);
        glfwGetWindowSize(w, &ww, &wh);
        int count = 0;
        GLFWmonitor** mons = glfwGetMonitors(&count);
        int bestArea = -1;
        for(int i=0;i<count;i++){
            int mx = 0, my = 0;
            glfwGetMonitorPos(mons[i], &mx, &my);
            const GLFWvidmode* vm = glfwGetVideoMode(mons[i]);
            if(!vm) continue;
            int mw = vm->width;
            int mh = vm->height;
            int ix0 = wx > mx ? wx : mx;
            int iy0 = wy > my ? wy : my;
            int ix1 = (wx + ww) < (mx + mw) ? (wx + ww) : (mx + mw);
            int iy1 = (wy + wh) < (my + mh) ? (wy + wh) : (my + mh);
            int iw = ix1 - ix0;
            int ih = iy1 - iy0;
            if(iw <= 0 || ih <= 0) continue;
            int area = iw * ih;
            if(area > bestArea){
                bestArea = area;
                mon = mons[i];
            }
        }
    }
    if(!mon) mon = glfwGetPrimaryMonitor();
    if(!mon) return 60;
    const GLFWvidmode* vm = glfwGetVideoMode(mon);
    if(!vm || vm->refreshRate <= 0) return 60;
    return vm->refreshRate;
}

inline void applyFramePacing(double frameStartTime, int targetFps){
    if(targetFps <= 0) return;
    double targetFrameSec = 1.0 / (double)targetFps;
    double elapsed = glfwGetTime() - frameStartTime;
    double remain = targetFrameSec - elapsed;
    if(remain <= 0.0) return;

    if(remain > 0.002){
        DWORD sleepMs = (DWORD)((remain - 0.001) * 1000.0);
        if(sleepMs > 0) Sleep(sleepMs);
    }
    while((glfwGetTime() - frameStartTime) < targetFrameSec){}
}

