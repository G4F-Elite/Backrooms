#pragma once

inline void mouse(GLFWwindow*,double xp,double yp){
    if(gameState!=STATE_GAME&&gameState!=STATE_INTRO)return;
    if(debugTools.open) {
        lastX = (float)xp;
        lastY = (float)yp;
        firstMouse = true;
        return;
    }
    if(isPlayerDead || playerEscaped || playerFalling) return;
    if(firstMouse){lastX=(float)xp;lastY=(float)yp;firstMouse=false;}
    // A dead co-op player riding along with somebody turns the spectator camera instead.
    if(spectatorMouseLook((float)xp-lastX, lastY-(float)yp, settings.mouseSens)){
        lastX=(float)xp;lastY=(float)yp;
        return;
    }
    cam.yaw-=((float)xp-lastX)*settings.mouseSens;
    cam.pitch+=(lastY-(float)yp)*settings.mouseSens;
    cam.pitch=cam.pitch>1.4f?1.4f:(cam.pitch<-1.4f?-1.4f:cam.pitch);
    lastX=(float)xp;lastY=(float)yp;
}
