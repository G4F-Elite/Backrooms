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
    float moveSway = sndState.moveIntensity * 0.006f;
    float sprintSway = sndState.sprintIntensity * 0.004f;
    shX += sinf(vhsTime * (8.0f + sndState.sprintIntensity * 4.0f)) * (moveSway + sprintSway);
    shY += cosf(vhsTime * (12.5f + sndState.sprintIntensity * 5.0f)) * (moveSway * 0.75f + sprintSway);
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

