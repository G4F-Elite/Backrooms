#ifndef ENTITY_MODEL_H
#define ENTITY_MODEL_H

#include <vector>
#include <cmath>
#include "entity_types.h"

// Stalker model vertices (procedural humanoid silhouette)
inline void buildStalkerModel(std::vector<float>& verts) {
    verts.clear();
    float h = 2.4f, w = 0.3f, d = 0.15f;
    
    // Body - Front/Back/Left/Right faces
    float body[] = {
        -w, 0, d, 0, 0, 0, 0, 1, w, 0, d, 1, 0, 0, 0, 1, w, h*0.7f, d, 1, 1, 0, 0, 1,
        -w, 0, d, 0, 0, 0, 0, 1, w, h*0.7f, d, 1, 1, 0, 0, 1, -w, h*0.7f, d, 0, 1, 0, 0, 1,
        w, 0, -d, 0, 0, 0, 0, -1, -w, 0, -d, 1, 0, 0, 0, -1, -w, h*0.7f, -d, 1, 1, 0, 0, -1,
        w, 0, -d, 0, 0, 0, 0, -1, -w, h*0.7f, -d, 1, 1, 0, 0, -1, w, h*0.7f, -d, 0, 1, 0, 0, -1,
        -w, 0, -d, 0, 0, -1, 0, 0, -w, 0, d, 1, 0, -1, 0, 0, -w, h*0.7f, d, 1, 1, -1, 0, 0,
        -w, 0, -d, 0, 0, -1, 0, 0, -w, h*0.7f, d, 1, 1, -1, 0, 0, -w, h*0.7f, -d, 0, 1, -1, 0, 0,
        w, 0, d, 0, 0, 1, 0, 0, w, 0, -d, 1, 0, 1, 0, 0, w, h*0.7f, -d, 1, 1, 1, 0, 0,
        w, 0, d, 0, 0, 1, 0, 0, w, h*0.7f, -d, 1, 1, 1, 0, 0, w, h*0.7f, d, 0, 1, 1, 0, 0,
    };
    for(size_t i = 0; i < sizeof(body)/sizeof(float); i++) verts.push_back(body[i]);
    
    // Spherical head
    float hx = 0, hy = h * 0.78f, hr = 0.2f;
    for(int i = 0; i < 6; i++) for(int j = 0; j < 6; j++) {
        float a1 = (float)i / 6 * 3.14159f, a2 = (float)(i+1) / 6 * 3.14159f;
        float b1 = (float)j / 6 * 6.28318f, b2 = (float)(j+1) / 6 * 6.28318f;
        float x1=hr*sinf(a1)*cosf(b1), y1=hr*cosf(a1), z1=hr*sinf(a1)*sinf(b1);
        float x2=hr*sinf(a1)*cosf(b2), y2=hr*cosf(a1), z2=hr*sinf(a1)*sinf(b2);
        float x3=hr*sinf(a2)*cosf(b2), y3=hr*cosf(a2), z3=hr*sinf(a2)*sinf(b2);
        float x4=hr*sinf(a2)*cosf(b1), y4=hr*cosf(a2), z4=hr*sinf(a2)*sinf(b1);
        float tri[] = { hx+x1,hy+y1,z1,0,0,x1/hr,y1/hr,z1/hr, hx+x2,hy+y2,z2,1,0,x2/hr,y2/hr,z2/hr,
            hx+x3,hy+y3,z3,1,1,x3/hr,y3/hr,z3/hr, hx+x1,hy+y1,z1,0,0,x1/hr,y1/hr,z1/hr,
            hx+x3,hy+y3,z3,1,1,x3/hr,y3/hr,z3/hr, hx+x4,hy+y4,z4,0,1,x4/hr,y4/hr,z4/hr };
        for(int k=0;k<48;k++) verts.push_back(tri[k]);
    }
    
    // Arms
    float armW = 0.08f, armLen = 1.0f;
    for(int side = -1; side <= 1; side += 2) {
        float ax = side * (w + 0.05f), ay = h * 0.6f;
        float arm[] = { ax-armW,ay,armW,0,0,0,0,1, ax+armW,ay,armW,1,0,0,0,1, ax+armW+side*0.3f,ay-armLen,armW,1,1,0,0,1,
            ax-armW,ay,armW,0,0,0,0,1, ax+armW+side*0.3f,ay-armLen,armW,1,1,0,0,1, ax-armW+side*0.3f,ay-armLen,armW,0,1,0,0,1 };
        for(size_t i=0;i<sizeof(arm)/sizeof(float);i++) verts.push_back(arm[i]);
    }
    
    // Legs
    float legW = 0.1f, legLen = h * 0.4f;
    for(int side = -1; side <= 1; side += 2) {
        float lx = side * w * 0.5f;
        float leg[] = { lx-legW,0,d,0,0,0,0,1, lx+legW,0,d,1,0,0,0,1, lx+legW,legLen,d,1,1,0,0,1,
            lx-legW,0,d,0,0,0,0,1, lx+legW,legLen,d,1,1,0,0,1, lx-legW,legLen,d,0,1,0,0,1 };
        for(size_t i=0;i<sizeof(leg)/sizeof(float);i++) verts.push_back(leg[i]);
    }

    auto emitQuad = [&](
        float ax,float ay,float az,float au,float av,
        float bx,float by,float bz,float bu,float bv,
        float cx,float cy,float cz,float cu,float cv,
        float dx,float dy,float dz,float du,float dv,
        float nx,float ny,float nz
    ) {
        float tri[] = {
            ax,ay,az,au,av,nx,ny,nz, bx,by,bz,bu,bv,nx,ny,nz, cx,cy,cz,cu,cv,nx,ny,nz,
            ax,ay,az,au,av,nx,ny,nz, cx,cy,cz,cu,cv,nx,ny,nz, dx,dy,dz,du,dv,nx,ny,nz
        };
        for(int i=0;i<48;i++) verts.push_back(tri[i]);
    };

    auto addBox = [&](float cx,float cy,float cz,float sx,float sy,float sz) {
        float hx=sx*0.5f, hy=sy*0.5f, hz=sz*0.5f;
        emitQuad(cx-hx,cy-hy,cz+hz,0,0, cx+hx,cy-hy,cz+hz,1,0, cx+hx,cy+hy,cz+hz,1,1, cx-hx,cy+hy,cz+hz,0,1, 0,0,1);
        emitQuad(cx+hx,cy-hy,cz-hz,0,0, cx-hx,cy-hy,cz-hz,1,0, cx-hx,cy+hy,cz-hz,1,1, cx+hx,cy+hy,cz-hz,0,1, 0,0,-1);
        emitQuad(cx+hx,cy-hy,cz+hz,0,0, cx+hx,cy-hy,cz-hz,1,0, cx+hx,cy+hy,cz-hz,1,1, cx+hx,cy+hy,cz+hz,0,1, 1,0,0);
        emitQuad(cx-hx,cy-hy,cz-hz,0,0, cx-hx,cy-hy,cz+hz,1,0, cx-hx,cy+hy,cz+hz,1,1, cx-hx,cy+hy,cz-hz,0,1, -1,0,0);
        emitQuad(cx-hx,cy+hy,cz+hz,0,0, cx+hx,cy+hy,cz+hz,1,0, cx+hx,cy+hy,cz-hz,1,1, cx-hx,cy+hy,cz-hz,0,1, 0,1,0);
        emitQuad(cx-hx,cy-hy,cz-hz,0,0, cx+hx,cy-hy,cz-hz,1,0, cx+hx,cy-hy,cz+hz,1,1, cx-hx,cy-hy,cz+hz,0,1, 0,-1,0);
    };

    // Extra silhouette details for a more readable monster profile.
    addBox(0.0f, h * 0.60f, 0.0f, 0.82f, 0.16f, 0.28f);      // shoulders
    addBox(0.0f, h * 0.30f, -0.14f, 0.34f, 0.52f, 0.10f);     // spine hump
    addBox(0.0f, h * 0.52f, 0.18f, 0.36f, 0.30f, 0.10f);      // chest ridge
    addBox(0.0f, h * 0.04f, -0.02f, 0.54f, 0.08f, 0.32f);     // pelvis skirt
    for(int side=-1;side<=1;side+=2){
        addBox(side * (w + 0.17f), h * 0.18f, 0.04f, 0.10f, 0.58f, 0.10f); // forearm bulk
        addBox(side * (w + 0.26f), h * 0.02f, 0.16f, 0.16f, 0.06f, 0.24f); // claw mass
        addBox(side * 0.16f, h * 0.86f, -0.05f, 0.08f, 0.20f, 0.08f);      // horn spike
    }
}

inline void buildCrawlerModel(std::vector<float>& verts) {
    verts.clear();
    auto emitQuad = [&](float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz,float dx,float dy,float dz,float nx,float ny,float nz){
        float tri[] = {
            ax,ay,az,0,0,nx,ny,nz, bx,by,bz,1,0,nx,ny,nz, cx,cy,cz,1,1,nx,ny,nz,
            ax,ay,az,0,0,nx,ny,nz, cx,cy,cz,1,1,nx,ny,nz, dx,dy,dz,0,1,nx,ny,nz
        };
        for(int i=0;i<48;i++) verts.push_back(tri[i]);
    };
    auto addBox = [&](float cx,float cy,float cz,float sx,float sy,float sz){
        float hx=sx*0.5f, hy=sy*0.5f, hz=sz*0.5f;
        emitQuad(cx-hx,cy-hy,cz+hz, cx+hx,cy-hy,cz+hz, cx+hx,cy+hy,cz+hz, cx-hx,cy+hy,cz+hz, 0,0,1);
        emitQuad(cx+hx,cy-hy,cz-hz, cx-hx,cy-hy,cz-hz, cx-hx,cy+hy,cz-hz, cx+hx,cy+hy,cz-hz, 0,0,-1);
        emitQuad(cx+hx,cy-hy,cz+hz, cx+hx,cy-hy,cz-hz, cx+hx,cy+hy,cz-hz, cx+hx,cy+hy,cz+hz, 1,0,0);
        emitQuad(cx-hx,cy-hy,cz-hz, cx-hx,cy-hy,cz+hz, cx-hx,cy+hy,cz+hz, cx-hx,cy+hy,cz-hz, -1,0,0);
        emitQuad(cx-hx,cy+hy,cz+hz, cx+hx,cy+hy,cz+hz, cx+hx,cy+hy,cz-hz, cx-hx,cy+hy,cz-hz, 0,1,0);
        emitQuad(cx-hx,cy-hy,cz-hz, cx+hx,cy-hy,cz-hz, cx+hx,cy-hy,cz+hz, cx-hx,cy-hy,cz+hz, 0,-1,0);
    };

    addBox(0.0f, -0.42f, 0.0f, 1.00f, 0.34f, 0.70f);
    addBox(0.0f, -0.22f, 0.28f, 0.54f, 0.18f, 0.28f);
    addBox(0.0f, -0.20f, -0.30f, 0.62f, 0.20f, 0.26f);
    addBox(0.0f, -0.05f, 0.0f, 0.36f, 0.18f, 0.44f);
    for(int side=-1;side<=1;side+=2){
        addBox(side*0.42f, -0.36f, 0.22f, 0.16f, 0.14f, 0.42f);
        addBox(side*0.42f, -0.36f, -0.20f, 0.16f, 0.14f, 0.42f);
        addBox(side*0.54f, -0.40f, 0.48f, 0.10f, 0.06f, 0.18f);
        addBox(side*0.54f, -0.40f, -0.46f, 0.10f, 0.06f, 0.18f);
    }
    addBox(0.0f, 0.00f, 0.40f, 0.24f, 0.12f, 0.18f);
}

inline void buildShadowModel(std::vector<float>& verts) {
    verts.clear();
    auto emitBillboard = [&](float y0,float y1,float halfW,float nz){
        float tri[] = {
            -halfW,y0,0,0,0,0,0,nz, halfW,y0,0,1,0,0,0,nz, halfW,y1,0,1,1,0,0,nz,
            -halfW,y0,0,0,0,0,0,nz, halfW,y1,0,1,1,0,0,nz, -halfW,y1,0,0,1,0,0,nz
        };
        for(int i=0;i<48;i++) verts.push_back(tri[i]);
    };
    emitBillboard(-0.05f, 2.65f, 0.58f, 1.0f);
    emitBillboard(-0.05f, 2.65f, 0.58f, -1.0f);
    emitBillboard(0.10f, 2.45f, 0.30f, 1.0f);
    emitBillboard(0.10f, 2.45f, 0.30f, -1.0f);
}

// Generate dark entity texture
inline void genEntityTextureByType(EntityType type, unsigned char* data, int w, int h) {
    for(int y = 0; y < h; y++) for(int x = 0; x < w; x++) {
        int idx = (y * w + x) * 4;
        float noise = (float)(rand() % 40) / 255.0f;
        float u = (float)x / (float)(w > 1 ? (w - 1) : 1);
        float v = (float)y / (float)(h > 1 ? (h - 1) : 1);
        float pat = fabsf(sinf((u * 7.0f + v * 5.0f) * 3.14159f));
        float r = 10.0f + noise * 20.0f;
        float g = 8.0f + noise * 16.0f;
        float b = 12.0f + noise * 24.0f;
        if(type == ENTITY_STALKER){
            float grime = 0.3f + 0.7f * pat;
            r += grime * 8.0f;
            g += grime * 6.0f;
            b += grime * 10.0f;
            bool eyeBand = (v > 0.58f && v < 0.68f) && ((u > 0.34f && u < 0.42f) || (u > 0.58f && u < 0.66f));
            bool chestScar = (fabsf(u - 0.5f) < 0.03f && v > 0.25f && v < 0.85f);
            if(eyeBand){
                r = 138.0f + noise * 60.0f;
                g = 22.0f + noise * 20.0f;
                b = 20.0f + noise * 20.0f;
            }else if(chestScar){
                r += 32.0f;
                g += 8.0f;
                b += 8.0f;
            }
        }else if(type == ENTITY_CRAWLER){
            float bone = 0.55f + 0.45f * pat;
            r = 36.0f + noise * 24.0f + bone * 34.0f;
            g = 26.0f + noise * 18.0f + bone * 26.0f;
            b = 18.0f + noise * 14.0f + bone * 14.0f;
            bool maw = (v > 0.40f && v < 0.56f) && fabsf(u - 0.5f) < 0.24f;
            bool teeth = maw && (fmodf(u * 26.0f, 2.0f) > 1.0f);
            if(maw){
                r = 58.0f + noise * 14.0f;
                g = 10.0f + noise * 7.0f;
                b = 9.0f + noise * 7.0f;
                if(teeth){
                    r = 180.0f;
                    g = 170.0f;
                    b = 146.0f;
                }
            }
        }else{
            float veil = 0.2f + 0.8f * pat;
            r = 14.0f + noise * 12.0f + veil * 16.0f;
            g = 12.0f + noise * 10.0f + veil * 14.0f;
            b = 22.0f + noise * 22.0f + veil * 44.0f;
            bool core = fabsf(u - 0.5f) < 0.12f && v > 0.30f && v < 0.72f;
            if(core){
                r = 96.0f + noise * 32.0f;
                g = 40.0f + noise * 20.0f;
                b = 140.0f + noise * 36.0f;
            }
        }
        if(r > 255.0f) r = 255.0f;
        if(g > 255.0f) g = 255.0f;
        if(b > 255.0f) b = 255.0f;
        data[idx] = (unsigned char)r;
        data[idx+1] = (unsigned char)g;
        data[idx+2] = (unsigned char)b;
        data[idx+3] = 255;
    }
}

inline void genEntityTexture(unsigned char* data, int w, int h) {
    genEntityTextureByType(ENTITY_STALKER, data, w, h);
}

#endif
