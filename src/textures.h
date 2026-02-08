#pragma once
#include <glad/glad.h>
#include <cmath>

inline float noise2d(float x, float y) {
    int n=(int)x+(int)y*57; n=(n<<13)^n;
    return (1.0f-((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.0f);
}

inline float perlin(float x, float y, int oct) {
    float r=0, a=1, f=1, mx=0;
    for(int i=0;i<oct;i++) {
        int ix=(int)(x*f), iy=(int)(y*f);
        float fx=x*f-ix, fy=y*f-iy;
        float n00=noise2d((float)ix,(float)iy), n10=noise2d((float)(ix+1),(float)iy);
        float n01=noise2d((float)ix,(float)(iy+1)), n11=noise2d((float)(ix+1),(float)(iy+1));
        float nx0=n00*(1-fx)+n10*fx, nx1=n01*(1-fx)+n11*fx;
        r+=(nx0*(1-fy)+nx1*fy)*a;
        mx+=a; a*=0.5f; f*=2;
    }
    return r/mx;
}

inline GLuint genTex(int type) {
    const int sz=(type==3)?256:512;
    unsigned char* d=new unsigned char[sz*sz*3];
    for(int y=0;y<sz;y++) for(int x=0;x<sz;x++) {
        float r=128,g=128,b=128;
        if(type==0) { // wall - yellow wallpaper
            r=205+perlin(x*0.05f,y*0.05f,4)*25+(sinf(x*0.3f)*0.5f+0.5f)*10;
            g=185+perlin(x*0.05f,y*0.05f,4)*25-perlin(x*0.02f,y*0.02f,2)*15;
            b=95+perlin(x*0.05f,y*0.05f,4)*10-perlin(x*0.02f,y*0.02f,2)*25;
            if(y>sz*0.75f) { float wd=perlin(x*0.1f,y*0.05f,3)*(y-sz*0.75f)*0.4f; r-=wd*0.8f; g-=wd; b-=wd*0.6f; }
            float streak=sinf(x*0.2f)*5.0f; r+=streak; g+=streak*0.9f; b+=streak*0.5f;
        } else if(type==1) { // floor - carpet
            float cp=perlin(x*0.15f,y*0.15f,4)*20;
            float dt=perlin(x*0.03f,y*0.03f,5)*35;
            float fib=(perlin(x*0.5f,y*0.5f,2)+1.0f)*8;
            r=135+cp-dt+fib; g=115+cp-dt*1.3f+fib; b=75+cp*0.5f-dt*0.7f+fib*0.5f;
            if(perlin(x*0.08f,y*0.08f,2)>0.6f) { r-=25; g-=25; b-=15; }
        } else if(type==2) { // ceiling - realistic drop ceiling panels
            int ts=64; float lx=(float)(x%ts), ly=(float)(y%ts);
            // Soft edge shadow (AO) - darker near edges
            float dx=fminf(lx,ts-lx), dy=fminf(ly,ts-ly);
            float edgeDist=fminf(dx,dy); // distance to nearest edge
            float ao=(edgeDist<8.0f)?(1.0f-edgeDist/8.0f)*0.12f:0;
            // Soft texture variation
            float var=perlin(x*0.06f,y*0.06f,5)*8;
            float detail=perlin(x*0.25f,y*0.25f,3)*4;
            // Water stains
            float stain=perlin(x*0.015f+2,y*0.015f+1,4);
            float damage=(stain>0.55f)?(stain-0.55f)*25:0;
            // Soft fiber texture
            float fiber=perlin(x*0.4f+y*0.1f,y*0.4f+x*0.1f,2)*4;
            // Base beige color with all effects
            r=230-var-damage+detail+fiber; g=218-var*1.05f-damage*1.15f+detail+fiber*0.9f; b=175-var*0.7f-damage*0.7f+detail*0.7f+fiber*0.6f;
            // Apply AO darkening near edges
            r-=ao*60; g-=ao*55; b-=ao*45;
            // Subtle random variation
            if(perlin(x*0.12f,y*0.12f,2)>0.65f) { r-=8; g-=10; b-=6; }
        } else if(type==3) { // light panel
            float cx=x-sz/2, cy=y-sz/2;
            float dd=sqrtf(cx*cx*0.5f+cy*cy*0.5f)/sz;
            float glow=1.0f-dd*1.8f; if(glow<0) glow=0;
            r=255*glow; g=252*glow; b=240*glow;
        }
        d[(y*sz+x)*3+0]=(unsigned char)(r<0?0:(r>255?255:(int)r));
        d[(y*sz+x)*3+1]=(unsigned char)(g<0?0:(g>255?255:(int)g));
        d[(y*sz+x)*3+2]=(unsigned char)(b<0?0:(b>255?255:(int)b));
    }
    GLuint tex; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,sz,sz,0,GL_RGB,GL_UNSIGNED_BYTE,d);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] d; return tex;
}
