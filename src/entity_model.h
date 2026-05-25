#ifndef ENTITY_MODEL_H
#define ENTITY_MODEL_H

#include <vector>
#include <cmath>
#include "entity_types.h"

// Stalker model — elongated, asymmetric horror humanoid
inline void buildStalkerModel(std::vector<float>& verts) {
    verts.clear();
    float h = 2.4f, w = 0.3f, d = 0.15f;

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

    // addBox with optional color (uses normal channel as vertex color)
    auto addBox = [&](float cx,float cy,float cz,float sx,float sy,float sz,
                      float cr=0.0f,float cg=0.0f,float cb=0.0f) {
        float hx=sx*0.5f, hy=sy*0.5f, hz=sz*0.5f;
        bool hasColor = (cr != 0.0f || cg != 0.0f || cb != 0.0f);
        // Front
        emitQuad(cx-hx,cy-hy,cz+hz,0,0, cx+hx,cy-hy,cz+hz,1,0, cx+hx,cy+hy,cz+hz,1,1, cx-hx,cy+hy,cz+hz,0,1,
            hasColor?cr:0, hasColor?cg:0, hasColor?cb:1);
        // Back
        emitQuad(cx+hx,cy-hy,cz-hz,0,0, cx-hx,cy-hy,cz-hz,1,0, cx-hx,cy+hy,cz-hz,1,1, cx+hx,cy+hy,cz-hz,0,1,
            hasColor?cr:0, hasColor?cg:0, hasColor?cb:-1);
        // Right
        emitQuad(cx+hx,cy-hy,cz+hz,0,0, cx+hx,cy-hy,cz-hz,1,0, cx+hx,cy+hy,cz-hz,1,1, cx+hx,cy+hy,cz+hz,0,1,
            hasColor?cr:1, hasColor?cg:0, hasColor?cb:0);
        // Left
        emitQuad(cx-hx,cy-hy,cz-hz,0,0, cx-hx,cy-hy,cz+hz,1,0, cx-hx,cy+hy,cz+hz,1,1, cx-hx,cy+hy,cz-hz,0,1,
            hasColor?cr:-1, hasColor?cg:0, hasColor?cb:0);
        // Top
        emitQuad(cx-hx,cy+hy,cz+hz,0,0, cx+hx,cy+hy,cz+hz,1,0, cx+hx,cy+hy,cz-hz,1,1, cx-hx,cy+hy,cz-hz,0,1,
            hasColor?cr:0, hasColor?cg:1, hasColor?cb:0);
        // Bottom
        emitQuad(cx-hx,cy-hy,cz-hz,0,0, cx+hx,cy-hy,cz-hz,1,0, cx+hx,cy-hy,cz+hz,1,1, cx-hx,cy-hy,cz+hz,0,1,
            hasColor?cr:0, hasColor?cg:-1, hasColor?cb:0);
    };

    // Body — hunched torso
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

    // Head — slightly elongated, asymmetric
    float hx = 0, hy = h * 0.78f, hr = 0.22f;
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

    // Glowing eyes — bright orange-red emissive (uses normal-as-color)
    addBox(-0.09f, h*0.82f, 0.14f, 0.05f, 0.035f, 0.02f, 1.0f, 0.2f, 0.0f);
    addBox( 0.09f, h*0.82f, 0.14f, 0.05f, 0.035f, 0.02f, 1.0f, 0.2f, 0.0f);
    // Eye glow halo — slightly larger dimmer box behind
    addBox(-0.09f, h*0.82f, 0.12f, 0.08f, 0.06f, 0.01f, 0.8f, 0.1f, 0.0f);
    addBox( 0.09f, h*0.82f, 0.12f, 0.08f, 0.06f, 0.01f, 0.8f, 0.1f, 0.0f);

    // Jaw / mouth — opening with teeth
    addBox(0.0f, h*0.70f, 0.14f, 0.16f, 0.04f, 0.06f, 0.3f, 0.15f, 0.1f);
    // Teeth — small bright boxes along jaw
    for(int i = 0; i < 5; i++) {
        float tx = -0.06f + i * 0.03f;
        addBox(tx, h*0.69f, 0.17f, 0.015f, 0.03f, 0.015f, 0.9f, 0.85f, 0.75f);
    }

    // Asymmetric arms — left arm longer and more angled
    float armW = 0.08f;
    // Left arm (longer, more menacing)
    {
        float ax = -(w + 0.05f), ay = h * 0.6f;
        float armLen = 1.2f; // elongated
        float arm[] = { ax-armW,ay,armW,0,0,0,0,1, ax+armW,ay,armW,1,0,0,0,1, ax+armW-0.45f,ay-armLen,armW,1,1,0,0,1,
            ax-armW,ay,armW,0,0,0,0,1, ax+armW-0.45f,ay-armLen,armW,1,1,0,0,1, ax-armW-0.45f,ay-armLen,armW,0,1,0,0,1 };
        for(size_t i=0;i<sizeof(arm)/sizeof(float);i++) verts.push_back(arm[i]);
        // Forearm extension
        addBox(ax - 0.40f, ay - armLen - 0.15f, armW, 0.07f, 0.35f, 0.07f);
        // Claw fingers — 3 talons on left hand
        float handX = ax - 0.40f, handY = ay - armLen - 0.35f;
        for(int i = 0; i < 3; i++) {
            float angle = (i - 1) * 0.35f;
            addBox(handX + sinf(angle)*0.06f, handY - 0.08f, armW + cosf(angle)*0.06f,
                   0.02f, 0.18f, 0.02f, 0.7f, 0.6f, 0.5f);
        }
    }
    // Right arm (shorter, different angle)
    {
        float ax = (w + 0.05f), ay = h * 0.6f;
        float armLen = 0.95f;
        float arm[] = { ax-armW,ay,armW,0,0,0,0,1, ax+armW,ay,armW,1,0,0,0,1, ax+armW+0.35f,ay-armLen,armW,1,1,0,0,1,
            ax-armW,ay,armW,0,0,0,0,1, ax+armW+0.35f,ay-armLen,armW,1,1,0,0,1, ax-armW+0.35f,ay-armLen,armW,0,1,0,0,1 };
        for(size_t i=0;i<sizeof(arm)/sizeof(float);i++) verts.push_back(arm[i]);
        addBox(ax + 0.32f, ay - armLen - 0.12f, armW, 0.07f, 0.28f, 0.07f);
        float handX = ax + 0.32f, handY = ay - armLen - 0.30f;
        for(int i = 0; i < 3; i++) {
            float angle = (i - 1) * 0.35f;
            addBox(handX + sinf(angle)*0.06f, handY - 0.08f, armW + cosf(angle)*0.06f,
                   0.02f, 0.15f, 0.02f, 0.7f, 0.6f, 0.5f);
        }
    }

    // Legs
    float legW = 0.1f, legLen = h * 0.4f;
    for(int side = -1; side <= 1; side += 2) {
        float lx = side * w * 0.5f;
        float leg[] = { lx-legW,0,d,0,0,0,0,1, lx+legW,0,d,1,0,0,0,1, lx+legW,legLen,d,1,1,0,0,1,
            lx-legW,0,d,0,0,0,0,1, lx+legW,legLen,d,1,1,0,0,1, lx-legW,legLen,d,0,1,0,0,1 };
        for(size_t i=0;i<sizeof(leg)/sizeof(float);i++) verts.push_back(leg[i]);
    }

    // Spine spikes — visible vertebrae along back
    for(int i = 0; i < 6; i++) {
        float spineY = h * 0.10f + i * (h * 0.09f);
        float spikeH = 0.06f + (i % 3) * 0.02f;
        addBox(0.0f, spineY, -0.18f, 0.03f, spikeH, 0.03f, 0.5f, 0.4f, 0.4f);
    }

    // Exposed ribs — horizontal bars across torso
    for(int i = 0; i < 4; i++) {
        float ribY = h * 0.30f + i * 0.12f;
        float ribW = 0.28f - i * 0.02f;
        addBox(0.0f, ribY, 0.16f, ribW, 0.025f, 0.025f, 0.55f, 0.45f, 0.4f);
    }

    // Shoulders — hunched, asymmetric
    addBox(-0.05f, h * 0.62f, 0.0f, 0.42f, 0.14f, 0.26f);
    addBox( 0.08f, h * 0.58f, 0.0f, 0.38f, 0.12f, 0.24f);

    // Spine hump
    addBox(0.0f, h * 0.30f, -0.14f, 0.34f, 0.52f, 0.10f);
    // Chest ridge
    addBox(0.0f, h * 0.52f, 0.18f, 0.36f, 0.30f, 0.10f);
    // Pelvis skirt
    addBox(0.0f, h * 0.04f, -0.02f, 0.54f, 0.08f, 0.32f);

    // Horn spikes on head
    for(int side=-1;side<=1;side+=2){
        addBox(side * 0.16f, h * 0.86f, -0.05f, 0.06f, 0.22f, 0.06f, 0.4f, 0.35f, 0.3f);
    }

    // Tattered cloth strips hanging from body
    for(int i = 0; i < 5; i++) {
        float stripX = -0.15f + i * 0.075f;
        float stripLen = 0.25f + (i % 3) * 0.1f;
        float stripZ = 0.08f + sinf(i * 1.7f) * 0.04f;
        addBox(stripX, h * 0.02f - stripLen * 0.5f, stripZ, 0.03f, stripLen, 0.015f, 0.25f, 0.2f, 0.22f);
    }
    // Cloth strips from arms
    addBox(-0.45f, h * 0.28f, 0.12f, 0.025f, 0.30f, 0.012f, 0.22f, 0.18f, 0.2f);
    addBox( 0.38f, h * 0.32f, 0.10f, 0.025f, 0.22f, 0.012f, 0.22f, 0.18f, 0.2f);
}

// Crawler model — insectoid multi-legged horror
inline void buildCrawlerModel(std::vector<float>& verts) {
    verts.clear();
    auto emitQuad = [&](float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz,float dx,float dy,float dz,float nx,float ny,float nz){
        float tri[] = {
            ax,ay,az,0,0,nx,ny,nz, bx,by,bz,1,0,nx,ny,nz, cx,cy,cz,1,1,nx,ny,nz,
            ax,ay,az,0,0,nx,ny,nz, cx,cy,cz,1,1,nx,ny,nz, dx,dy,dz,0,1,nx,ny,nz
        };
        for(int i=0;i<48;i++) verts.push_back(tri[i]);
    };
    auto addBox = [&](float cx,float cy,float cz,float sx,float sy,float sz,
                      float cr=0.0f,float cg=0.0f,float cb=0.0f){
        float hx=sx*0.5f, hy=sy*0.5f, hz=sz*0.5f;
        bool hc = (cr!=0.0f||cg!=0.0f||cb!=0.0f);
        emitQuad(cx-hx,cy-hy,cz+hz, cx+hx,cy-hy,cz+hz, cx+hx,cy+hy,cz+hz, cx-hx,cy+hy,cz+hz, hc?cr:0,hc?cg:0,hc?cb:1);
        emitQuad(cx+hx,cy-hy,cz-hz, cx-hx,cy-hy,cz-hz, cx-hx,cy+hy,cz-hz, cx+hx,cy+hy,cz-hz, hc?cr:0,hc?cg:0,hc?cb:-1);
        emitQuad(cx+hx,cy-hy,cz+hz, cx+hx,cy-hy,cz-hz, cx+hx,cy+hy,cz-hz, cx+hx,cy+hy,cz+hz, hc?cr:1,hc?cg:0,hc?cb:0);
        emitQuad(cx-hx,cy-hy,cz-hz, cx-hx,cy-hy,cz+hz, cx-hx,cy+hy,cz+hz, cx-hx,cy+hy,cz-hz, hc?cr:-1,hc?cg:0,hc?cb:0);
        emitQuad(cx-hx,cy+hy,cz+hz, cx+hx,cy+hy,cz+hz, cx+hx,cy+hy,cz-hz, cx-hx,cy+hy,cz-hz, hc?cr:0,hc?cg:1,hc?cb:0);
        emitQuad(cx-hx,cy-hy,cz-hz, cx+hx,cy-hy,cz-hz, cx+hx,cy-hy,cz+hz, cx-hx,cy-hy,cz+hz, hc?cr:0,hc?cg:-1,hc?cb:0);
    };

    // 3 segmented body — head, thorax, abdomen
    addBox(0.0f, -0.38f, 0.28f, 0.44f, 0.22f, 0.32f);       // head segment
    addBox(0.0f, -0.34f, -0.08f, 0.56f, 0.28f, 0.42f);      // thorax (largest)
    addBox(0.0f, -0.36f, -0.48f, 0.40f, 0.24f, 0.38f);      // abdomen

    // Glowing underbelly — sickly green-yellow
    addBox(0.0f, -0.50f, -0.08f, 0.30f, 0.04f, 0.50f, 0.4f, 0.8f, 0.2f);
    addBox(0.0f, -0.48f, 0.20f, 0.22f, 0.03f, 0.20f, 0.3f, 0.7f, 0.15f);
    addBox(0.0f, -0.48f, -0.40f, 0.20f, 0.03f, 0.22f, 0.3f, 0.6f, 0.15f);

    // Mandibles / pincers at front
    addBox(-0.14f, -0.32f, 0.48f, 0.08f, 0.04f, 0.16f, 0.5f, 0.4f, 0.35f);
    addBox( 0.14f, -0.32f, 0.48f, 0.08f, 0.04f, 0.16f, 0.5f, 0.4f, 0.35f);
    // Mandible tips — sharper, darker
    addBox(-0.16f, -0.30f, 0.58f, 0.04f, 0.03f, 0.10f, 0.6f, 0.5f, 0.4f);
    addBox( 0.16f, -0.30f, 0.58f, 0.04f, 0.03f, 0.10f, 0.6f, 0.5f, 0.4f);

    // 4 pairs of legs — insectoid spread
    for(int i = 0; i < 4; i++) {
        float legZ = 0.10f - i * 0.18f;
        float legSpread = 0.32f + (i % 2) * 0.06f;
        float legThick = 0.035f;
        // Left leg — two segments for jointed look
        addBox(-legSpread, -0.42f, legZ, 0.28f, legThick, legThick);
        addBox(-legSpread - 0.12f, -0.50f, legZ, 0.18f, legThick, legThick);
        // Right leg
        addBox( legSpread, -0.42f, legZ, 0.28f, legThick, legThick);
        addBox( legSpread + 0.12f, -0.50f, legZ, 0.18f, legThick, legThick);
    }

    // Spines along back
    for(int i = 0; i < 7; i++) {
        float spineZ = 0.20f - i * 0.12f;
        float spineH = 0.05f + (i % 3) * 0.025f;
        addBox(0.0f, -0.20f - spineH*0.5f, spineZ, 0.025f, spineH, 0.025f, 0.45f, 0.35f, 0.3f);
    }
    // Lateral spines
    for(int i = 0; i < 3; i++) {
        float spineZ = 0.0f - i * 0.20f;
        addBox(-0.30f, -0.30f, spineZ, 0.02f, 0.06f, 0.02f, 0.4f, 0.3f, 0.25f);
        addBox( 0.30f, -0.30f, spineZ, 0.02f, 0.06f, 0.02f, 0.4f, 0.3f, 0.25f);
    }

    // Eye cluster on head — multiple small glowing dots
    addBox(-0.10f, -0.28f, 0.42f, 0.04f, 0.04f, 0.02f, 1.0f, 0.6f, 0.0f);
    addBox( 0.10f, -0.28f, 0.42f, 0.04f, 0.04f, 0.02f, 1.0f, 0.6f, 0.0f);
    addBox(-0.06f, -0.24f, 0.44f, 0.03f, 0.03f, 0.02f, 0.9f, 0.5f, 0.0f);
    addBox( 0.06f, -0.24f, 0.44f, 0.03f, 0.03f, 0.02f, 0.9f, 0.5f, 0.0f);

    // Chitinous plate details on thorax
    addBox(-0.12f, -0.22f, -0.06f, 0.18f, 0.04f, 0.20f, 0.35f, 0.28f, 0.22f);
    addBox( 0.12f, -0.22f, -0.06f, 0.18f, 0.04f, 0.20f, 0.35f, 0.28f, 0.22f);
}

// Shadow model — volumetric ghost with flowing tentacles
inline void buildShadowModel(std::vector<float>& verts) {
    verts.clear();
    auto emitQuad = [&](float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz,float dx,float dy,float dz,float nx,float ny,float nz){
        float tri[] = {
            ax,ay,az,0,0,nx,ny,nz, bx,by,bz,1,0,nx,ny,nz, cx,cy,cz,1,1,nx,ny,nz,
            ax,ay,az,0,0,nx,ny,nz, cx,cy,cz,1,1,nx,ny,nz, dx,dy,dz,0,1,nx,ny,nz
        };
        for(int i=0;i<48;i++) verts.push_back(tri[i]);
    };
    auto addBox = [&](float cx,float cy,float cz,float sx,float sy,float sz,
                      float cr=0.0f,float cg=0.0f,float cb=0.0f){
        float hx=sx*0.5f, hy=sy*0.5f, hz=sz*0.5f;
        bool hc = (cr!=0.0f||cg!=0.0f||cb!=0.0f);
        emitQuad(cx-hx,cy-hy,cz+hz, cx+hx,cy-hy,cz+hz, cx+hx,cy+hy,cz+hz, cx-hx,cy+hy,cz+hz, hc?cr:0,hc?cg:0,hc?cb:1);
        emitQuad(cx+hx,cy-hy,cz-hz, cx-hx,cy-hy,cz-hz, cx-hx,cy+hy,cz-hz, cx+hx,cy+hy,cz-hz, hc?cr:0,hc?cg:0,hc?cb:-1);
        emitQuad(cx+hx,cy-hy,cz+hz, cx+hx,cy-hy,cz-hz, cx+hx,cy+hy,cz-hz, cx+hx,cy+hy,cz+hz, hc?cr:1,hc?cg:0,hc?cb:0);
        emitQuad(cx-hx,cy-hy,cz-hz, cx-hx,cy-hy,cz+hz, cx-hx,cy+hy,cz+hz, cx-hx,cy+hy,cz-hz, hc?cr:-1,hc?cg:0,hc?cb:0);
        emitQuad(cx-hx,cy+hy,cz+hz, cx+hx,cy+hy,cz+hz, cx+hx,cy+hy,cz-hz, cx-hx,cy+hy,cz-hz, hc?cr:0,hc?cg:1,hc?cb:0);
        emitQuad(cx-hx,cy-hy,cz-hz, cx+hx,cy-hy,cz-hz, cx+hx,cy-hy,cz+hz, cx-hx,cy-hy,cz+hz, hc?cr:0,hc?cg:-1,hc?cb:0);
    };
    // Rotated plane for layered ghost body
    auto addRotatedPlane = [&](float cx,float cy,float cz,float hw,float hh,float angle,
                                float cr,float cg,float cb){
        float c = cosf(angle), s = sinf(angle);
        // 4 corners rotated around Y axis
        float x0=-hw, x1=hw, x3=-hw;
        float rx0=x0*c, rz0=x0*s, rx1=x1*c, rz1=x1*s, rx2=x1*c, rz2=x1*s, rx3=x3*c, rz3=x3*s;
        emitQuad(cx+rx0,cy-0.6f,cz+rz0, cx+rx1,cy-0.6f,cz+rz1, cx+rx2,cy+0.6f,cz+rz2, cx+rx3,cy+0.6f,cz+rz3, cr,cg,cb);
        emitQuad(cx+rx3,cy+0.6f,cz+rz3, cx+rx2,cy+0.6f,cz+rz2, cx+rx1,cy-0.6f,cz+rz1, cx+rx0,cy-0.6f,cz+rz0, cr,cg,cb);
    };

    // Main body — 3 overlapping planes at 60-degree angles
    addRotatedPlane(0.0f, 0.9f, 0.0f, 0.45f, 1.0f, 0.0f,    0.12f, 0.08f, 0.18f);
    addRotatedPlane(0.0f, 0.9f, 0.0f, 0.38f, 0.9f, 1.047f,  0.10f, 0.07f, 0.16f);
    addRotatedPlane(0.0f, 0.9f, 0.0f, 0.42f, 0.95f, 2.094f, 0.11f, 0.07f, 0.17f);

    // Inner core — smaller, brighter planes
    addRotatedPlane(0.0f, 0.85f, 0.0f, 0.22f, 0.55f, 0.524f,  0.25f, 0.12f, 0.35f);
    addRotatedPlane(0.0f, 0.85f, 0.0f, 0.20f, 0.50f, 1.571f,  0.22f, 0.10f, 0.32f);

    // Hollow eyes — black voids (zero color = pure black)
    addBox(-0.12f, 1.15f, 0.08f, 0.09f, 0.07f, 0.03f, 0.0f, 0.0f, 0.0f);
    addBox( 0.12f, 1.15f, 0.08f, 0.09f, 0.07f, 0.03f, 0.0f, 0.0f, 0.0f);
    // Eye rim — faint purple glow around voids
    addBox(-0.12f, 1.15f, 0.06f, 0.12f, 0.10f, 0.01f, 0.3f, 0.1f, 0.5f);
    addBox( 0.12f, 1.15f, 0.06f, 0.12f, 0.10f, 0.01f, 0.3f, 0.1f, 0.5f);

    // Flowing tentacles below — varying lengths
    for(int i = 0; i < 6; i++) {
        float tentX = (i - 2.5f) * 0.12f;
        float tentLen = 0.35f + (i % 3) * 0.15f;
        float tentZ = sinf(i * 1.3f) * 0.06f;
        addBox(tentX, 0.25f - tentLen * 0.5f, tentZ, 0.04f, tentLen, 0.04f, 0.08f, 0.06f, 0.12f);
        // Tentacle tip — slightly brighter
        addBox(tentX, 0.25f - tentLen + 0.02f, tentZ, 0.03f, 0.04f, 0.03f, 0.15f, 0.1f, 0.22f);
    }

    // Wispy tendrils extending outward
    for(int i = 0; i < 4; i++) {
        float angle = i * 1.571f + 0.4f;
        float tx = cosf(angle) * 0.35f;
        float tz = sinf(angle) * 0.35f;
        addBox(tx, 0.7f, tz, 0.03f, 0.5f, 0.03f, 0.06f, 0.04f, 0.1f);
    }

    // Head crown — jagged top
    for(int i = 0; i < 5; i++) {
        float crownX = (i - 2) * 0.1f;
        float crownH = 0.12f + (i % 2) * 0.08f;
        addBox(crownX, 1.55f + crownH * 0.5f, 0.0f, 0.04f, crownH, 0.04f, 0.1f, 0.07f, 0.15f);
    }

    // Mouth — gaping dark opening
    addBox(0.0f, 0.95f, 0.10f, 0.14f, 0.06f, 0.04f, 0.0f, 0.0f, 0.0f);
}

// Generate dark entity texture with type-specific horror details
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
            // Base grime with vein network
            float grime = 0.3f + 0.7f * pat;
            r += grime * 8.0f;
            g += grime * 6.0f;
            b += grime * 10.0f;
            // Vein pattern — branching red lines
            float vein = sinf(x * 0.3f + sinf(y * 0.2f) * 2.0f) * 0.5f + 0.5f;
            vein = (vein > 0.72f) ? 1.0f : 0.0f;
            float vein2 = sinf(y * 0.4f + cosf(x * 0.15f) * 3.0f) * 0.5f + 0.5f;
            vein2 = (vein2 > 0.75f) ? 1.0f : 0.0f;
            r += (vein + vein2) * 28.0f;
            g += (vein + vein2) * 4.0f;
            b += (vein + vein2) * 4.0f;
            // Scar tissue — rough patches
            float scar = sinf(u * 13.0f) * cosf(v * 11.0f);
            if(scar > 0.6f) { r += 18.0f; g += 10.0f; b += 8.0f; }
            // Glowing eyes
            bool eyeBand = (v > 0.58f && v < 0.68f) && ((u > 0.34f && u < 0.42f) || (u > 0.58f && u < 0.66f));
            bool chestScar = (fabsf(u - 0.5f) < 0.03f && v > 0.25f && v < 0.85f);
            if(eyeBand){
                r = 180.0f + noise * 60.0f;
                g = 30.0f + noise * 20.0f;
                b = 10.0f + noise * 15.0f;
            }else if(chestScar){
                r += 40.0f; g += 10.0f; b += 10.0f;
            }
            // Muscle fiber striations
            float fiber = sinf(v * 30.0f + u * 5.0f) * 0.5f + 0.5f;
            if(fiber > 0.8f) { r += 12.0f; g += 6.0f; }
        }else if(type == ENTITY_CRAWLER){
            // Chitinous plate pattern
            float bone = 0.55f + 0.45f * pat;
            r = 36.0f + noise * 24.0f + bone * 34.0f;
            g = 26.0f + noise * 18.0f + bone * 26.0f;
            b = 18.0f + noise * 14.0f + bone * 14.0f;
            // Plate segmentation — hexagonal-ish boundaries
            float plateX = fmodf(u * 6.0f, 1.0f);
            float plateY = fmodf(v * 8.0f, 1.0f);
            bool plateEdge = (plateX < 0.06f || plateX > 0.94f || plateY < 0.06f || plateY > 0.94f);
            if(plateEdge){ r += 20.0f; g += 16.0f; b += 12.0f; }
            // Joint details — darker at segment boundaries
            float joint = sinf(v * 16.0f) * 0.5f + 0.5f;
            if(joint < 0.15f) { r *= 0.6f; g *= 0.6f; b *= 0.6f; }
            // Maw / mouth
            bool maw = (v > 0.40f && v < 0.56f) && fabsf(u - 0.5f) < 0.24f;
            bool teeth = maw && (fmodf(u * 26.0f, 2.0f) > 1.0f);
            if(maw){
                r = 58.0f + noise * 14.0f;
                g = 10.0f + noise * 7.0f;
                b = 9.0f + noise * 7.0f;
                if(teeth){ r = 180.0f; g = 170.0f; b = 146.0f; }
            }
            // Glowing underbelly spots
            float glow = sinf(u * 10.0f) * sinf(v * 12.0f);
            if(glow > 0.7f && v > 0.6f){
                r += 40.0f; g += 80.0f; b += 20.0f;
            }
        }else{
            // Shadow — swirling smoke pattern
            float smoke = sinf(u * 8.0f + sinf(v * 6.0f) * 2.0f) * 0.5f + 0.5f;
            float smoke2 = cosf(v * 10.0f + cosf(u * 5.0f) * 3.0f) * 0.5f + 0.5f;
            float veil = smoke * 0.5f + smoke2 * 0.5f;
            r = 14.0f + noise * 12.0f + veil * 18.0f;
            g = 12.0f + noise * 10.0f + veil * 14.0f;
            b = 22.0f + noise * 22.0f + veil * 48.0f;
            // Face-like features emerging from smoke
            float faceL = expf(-((u-0.38f)*(u-0.38f)*80.0f + (v-0.42f)*(v-0.42f)*60.0f));
            float faceR = expf(-((u-0.62f)*(u-0.62f)*80.0f + (v-0.42f)*(v-0.42f)*60.0f));
            float faceM = expf(-((u-0.50f)*(u-0.50f)*40.0f + (v-0.55f)*(v-0.55f)*50.0f));
            // Hollow eyes — dark voids
            if(faceL > 0.5f || faceR > 0.5f){
                r = 2.0f; g = 2.0f; b = 4.0f;
            }
            // Mouth — dark gash
            if(faceM > 0.6f && v > 0.52f && v < 0.58f){
                r = 4.0f; g = 2.0f; b = 6.0f;
            }
            // Core glow — faint purple luminescence
            bool core = fabsf(u - 0.5f) < 0.12f && v > 0.30f && v < 0.72f;
            if(core && faceL < 0.3f && faceR < 0.3f){
                r = 60.0f + noise * 30.0f + smoke * 36.0f;
                g = 24.0f + noise * 16.0f + smoke * 16.0f;
                b = 120.0f + noise * 36.0f + smoke * 40.0f;
            }
            // Smoke wisps — lighter streaks
            float wisp = sinf(u * 20.0f + v * 15.0f) * sinf(u * 7.0f - v * 9.0f);
            if(wisp > 0.8f){ r += 12.0f; g += 8.0f; b += 20.0f; }
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
