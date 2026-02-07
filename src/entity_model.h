#ifndef ENTITY_MODEL_H
#define ENTITY_MODEL_H

#include <vector>
#include <cmath>

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
}

// Generate dark entity texture
inline void genEntityTexture(unsigned char* data, int w, int h) {
    for(int y = 0; y < h; y++) for(int x = 0; x < w; x++) {
        int idx = (y * w + x) * 4;
        float noise = (float)(rand() % 20) / 255.0f;
        data[idx] = (unsigned char)(5 + noise * 10);
        data[idx+1] = (unsigned char)(3 + noise * 8);
        data[idx+2] = (unsigned char)(8 + noise * 12);
        data[idx+3] = 255;
    }
}

#endif