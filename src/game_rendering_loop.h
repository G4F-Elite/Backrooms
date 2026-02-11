#pragma once
inline Mat4 composeModelMatrix(const Vec3& pos, float yaw, float pitch, const Vec3& scale);

void renderScene(){
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    struct MainUniforms {
        GLint P, V, M, vp, tm, danger, flashOn, flashDir, flashPos, rfc, rfp, rfd, nl, lp, tint;
    };
    struct LightUniforms {
        GLint P, V, M, inten, tm, fade, danger;
    };
    static MainUniforms mu = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    static LightUniforms lu = {-1,-1,-1,-1,-1,-1,-1};
    if(mu.P < 0){
        mu.P = glGetUniformLocation(mainShader,"P");
        mu.V = glGetUniformLocation(mainShader,"V");
        mu.M = glGetUniformLocation(mainShader,"M");
        mu.vp = glGetUniformLocation(mainShader,"vp");
        mu.tm = glGetUniformLocation(mainShader,"tm");
        mu.danger = glGetUniformLocation(mainShader,"danger");
        mu.flashOn = glGetUniformLocation(mainShader,"flashOn");
        mu.flashDir = glGetUniformLocation(mainShader,"flashDir");
        mu.flashPos = glGetUniformLocation(mainShader,"flashPos");
        mu.rfc = glGetUniformLocation(mainShader,"rfc");
        mu.rfp = glGetUniformLocation(mainShader,"rfp");
        mu.rfd = glGetUniformLocation(mainShader,"rfd");
        mu.nl = glGetUniformLocation(mainShader,"nl");
        mu.lp = glGetUniformLocation(mainShader,"lp");
        mu.tint = glGetUniformLocation(mainShader,"modelTint");
    }
    if(lu.P < 0){
        lu.P = glGetUniformLocation(lightShader,"P");
        lu.V = glGetUniformLocation(lightShader,"V");
        lu.M = glGetUniformLocation(lightShader,"M");
        lu.inten = glGetUniformLocation(lightShader,"inten");
        lu.tm = glGetUniformLocation(lightShader,"tm");
        lu.fade = glGetUniformLocation(lightShader,"fade");
        lu.danger = glGetUniformLocation(lightShader,"danger");
    }

    glUseProgram(mainShader);
    Mat4 proj=Mat4::persp(1.2f,(float)winW/winH,0.1f,100.0f);
    float shX=camShake*(rand()%100-50)/500.0f,shY=camShake*(rand()%100-50)/500.0f;
    float moveSway = sndState.moveIntensity * 0.006f;
    float sprintSway = sndState.sprintIntensity * 0.004f;
    shX += sinf(vhsTime * (8.0f + sndState.sprintIntensity * 4.0f)) * (moveSway + sprintSway);
    shY += cosf(vhsTime * (12.5f + sndState.sprintIntensity * 5.0f)) * (moveSway * 0.75f + sprintSway);
    // Camera view (world rendering) uses full shake.
    float viewYaw = cam.yaw + shX;
    float viewPitch = cam.pitch + shY;

    // Viewmodel (held item) should be minimally affected by shake, otherwise it feels like it
    // "jumps" near the face. Keep it stable; the world can shake independently.
    float vmYaw = cam.yaw + shX * vmShake;
    float vmPitch = cam.pitch + shY * vmShake;
    Vec3 la=cam.pos+Vec3(mSin(viewYaw)*mCos(viewPitch),mSin(viewPitch),
                         mCos(viewYaw)*mCos(viewPitch));
    Mat4 view=Mat4::look(cam.pos,la,Vec3(0,1,0)),model;
    
    glUniformMatrix4fv(mu.P,1,GL_FALSE,proj.m);
    glUniformMatrix4fv(mu.V,1,GL_FALSE,view.m);
    glUniformMatrix4fv(mu.M,1,GL_FALSE,model.m);
    glUniform3f(mu.vp,cam.pos.x,cam.pos.y,cam.pos.z);
    glUniform1f(mu.tm,vhsTime);
    glUniform1f(mu.danger,entityMgr.dangerLevel);
    if(mu.tint >= 0) glUniform3f(mu.tint,1.0f,1.0f,1.0f);

    // === Viewmodel (held item) state ===
    // Keep it deterministic and camera-relative (no spring physics) to avoid jitter/"floating".
    static float deviceEquip = 0.0f;
    float equipTarget = (activeDeviceSlot > 0) ? 1.0f : 0.0f;
    deviceEquip = equipTarget;

    // Camera directions
    Vec3 camFwd(mSin(viewYaw)*mCos(viewPitch),
                mSin(viewPitch),
                mCos(viewYaw)*mCos(viewPitch));
    Vec3 camRight(mCos(viewYaw), 0.0f, -mSin(viewYaw));
    Vec3 worldUp(0.0f, 1.0f, 0.0f);

    // Viewmodel orientation follows pitch, but POSITION should not ride pitch.
    // If we use full forward/up basis for position, looking up/down pushes the item into the face.
    // So we split:
    //  - vmFwdOri: orientation forward (uses pitch)
    //  - vmFwdPos/vmRightPos/worldUp: placement basis (yaw-only + world up)
    Vec3 vmFwdOri(mSin(vmYaw)*mCos(vmPitch),
                  mSin(vmPitch),
                  mCos(vmYaw)*mCos(vmPitch));
    Vec3 vmFwdPos(mSin(vmYaw), 0.0f, mCos(vmYaw));
    Vec3 vmRightPos(mCos(vmYaw), 0.0f, -mSin(vmYaw));
    bool flashVisualOn = flashlightOn;
    if(flashlightOn && flashlightShutdownBlinkActive){
        flashVisualOn = isFlashlightOnDuringShutdownBlink(flashlightShutdownBlinkTimer);
    }
    glUniform1i(mu.flashOn,flashVisualOn?1:0);
    glUniform3f(mu.flashDir, camFwd.x, camFwd.y, camFwd.z);
    // Default flashlight cone origin is camera; if we render a held flashlight we override below.
    glUniform3f(mu.flashPos, cam.pos.x, cam.pos.y, cam.pos.z);
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

    // If we have a held flashlight, update flashPos BEFORE the world geometry draw.
    // This ensures the shader uses flashlight origin, not camera origin.
    if(activeDeviceSlot == 1 && flashVisualOn && mu.flashPos >= 0){
        // Base position of the flashlight in view (tuned to sit below/side of crosshair)
        float fwd = vmHandFwd;
        float side = vmHandSide;
        float up = vmHandUp;

        Vec3 base = cam.pos + vmFwdPos * fwd + vmRightPos * side + worldUp * up;
        Vec3 lens = base + vmFwdPos * vmFlashLensFwd + vmRightPos * vmFlashLensSide + worldUp * vmFlashLensUp;
        glUniform3f(mu.flashPos, lens.x, lens.y, lens.z);
    }
    
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

    // Held item render (viewmodel)
    GLuint heldVAO = 0;
    int heldVC = 0;
    float handSide = vmHandSide;
    float handUp = vmHandUp;
    float handFwd = vmHandFwd;
    float yawAdd = 0.16f;
    float pitchAdd = -0.14f;
    Vec3 scale = Vec3(0.95f, 0.95f, 1.0f);
    Vec3 heldTint(1.0f, 1.0f, 1.0f);

    if(activeDeviceSlot == 1){
        heldVAO = flashlightVAO;
        heldVC = flashlightVC;
        yawAdd = 0.20f;
        pitchAdd = -0.20f;
        scale = Vec3(0.92f, 0.92f, 0.92f);
        heldTint = Vec3(0.74f, 0.74f, 0.75f);
    }else if(activeDeviceSlot == 2){
        heldVAO = scannerVAO;
        heldVC = scannerVC;
        handSide = vmHandSide * 0.88f;
        handUp = vmHandUp - 0.04f;
        handFwd = vmHandFwd - 0.02f;
        yawAdd = 0.12f;
        pitchAdd = -0.22f;
        scale = Vec3(1.05f, 1.0f, 1.25f);
        heldTint = Vec3(0.78f, 0.86f, 0.90f);
    }else if(activeDeviceSlot == 3){
        // Slot 3: consumable
        handSide = vmHandSide * 0.88f;
        handUp = vmHandUp - 0.08f;
        handFwd = vmHandFwd - 0.04f;

        if(heldConsumableType == ITEM_BATTERY){
            heldVAO = batteryVAO;
            heldVC = batteryVC;
            yawAdd = 0.28f;
            pitchAdd = -0.32f;
            scale = Vec3(0.85f, 0.85f, 0.85f);
            heldTint = Vec3(0.92f, 0.90f, 0.72f);
        }else{
            heldVAO = plushVAO;
            heldVC = plushVC;
            yawAdd = 0.22f;
            pitchAdd = -0.30f;
            scale = Vec3(0.95f, 0.95f, 0.95f);
            heldTint = Vec3(0.96f, 0.88f, 0.78f);
        }
    }

    if(heldVC>0 && deviceEquip > 0.02f){
        Vec3 baseTarget = cam.pos + vmFwdPos * handFwd + vmRightPos * handSide + worldUp * handUp;

        // "In-hand physics": smooth held item position slightly for weight.
        static Vec3 baseSmoothed(0.0f, 0.0f, 0.0f);
        static bool baseInit = false;
        if(!baseInit){ baseSmoothed = baseTarget; baseInit = true; }
        float a = 1.0f - expf(-22.0f * dTime);
        baseSmoothed = baseSmoothed + (baseTarget - baseSmoothed) * a;

        Vec3 drawScale = scale * (0.8f + 0.2f * deviceEquip);
        float heldPitch = vmPitch + pitchAdd;
        Mat4 heldModel = composeModelMatrix(baseSmoothed, vmYaw + yawAdd, heldPitch, drawScale);
        glUniformMatrix4fv(mu.M,1,GL_FALSE,heldModel.m);
        if(mu.tint >= 0) glUniform3f(mu.tint,heldTint.x,heldTint.y,heldTint.z);

        // Use a dedicated texture for handheld devices so they don't look like wood.
        GLuint heldTex = propTex;
        if(activeDeviceSlot == 1 || activeDeviceSlot == 2){
            if(deviceTex != 0) heldTex = deviceTex;
        }else if(activeDeviceSlot == 3 && heldConsumableType == ITEM_PLUSH_TOY){
            if(plushTex != 0) heldTex = plushTex;
        }
        glBindTexture(GL_TEXTURE_2D,heldTex);
        glBindVertexArray(heldVAO);
        glDrawArrays(GL_TRIANGLES,0,heldVC);

        glUniformMatrix4fv(mu.M,1,GL_FALSE,model.m);
        if(mu.tint >= 0) glUniform3f(mu.tint,1.0f,1.0f,1.0f);
    }
    
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
    glUniform1f(lu.danger, entityMgr.dangerLevel);
    glBindTexture(GL_TEXTURE_2D,lampTex);glBindVertexArray(lightVAO);glDrawArrays(GL_TRIANGLES,0,lightVC);
    if(lightOffVC>0){
        glUniform1f(lu.inten,0.15f);
        glUniform1f(lu.fade,1.0f);
        glBindVertexArray(lightOffVAO);glDrawArrays(GL_TRIANGLES,0,lightOffVC);
    }
    entityMgr.render(mainShader,proj,view);
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

inline Mat4 composeModelMatrix(const Vec3& pos, float yaw, float pitch, const Vec3& scale){
    Vec3 forward(mSin(yaw) * mCos(pitch), mSin(pitch), mCos(yaw) * mCos(pitch));
    forward.normalize();

    Vec3 worldUp(0.0f, 1.0f, 0.0f);
    Vec3 right = worldUp.cross(forward);
    if(right.lenSq() < 0.0001f){
        right = Vec3(mCos(yaw), 0.0f, -mSin(yaw));
    }else{
        right.normalize();
    }
    Vec3 up = forward.cross(right).norm();

    Mat4 r;
    r.m[0] = right.x * scale.x;
    r.m[1] = right.y * scale.x;
    r.m[2] = right.z * scale.x;
    r.m[3] = 0.0f;

    r.m[4] = up.x * scale.y;
    r.m[5] = up.y * scale.y;
    r.m[6] = up.z * scale.y;
    r.m[7] = 0.0f;

    r.m[8] = forward.x * scale.z;
    r.m[9] = forward.y * scale.z;
    r.m[10] = forward.z * scale.z;
    r.m[11] = 0.0f;

    r.m[12] = pos.x;
    r.m[13] = pos.y;
    r.m[14] = pos.z;
    r.m[15] = 1.0f;
    return r;
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

























