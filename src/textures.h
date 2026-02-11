#pragma once
#include <glad/glad.h>
#include <cmath>
#include "progression.h"

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

// Generate RGBA texture with height in alpha for parallax
inline GLuint genTex(int type) {
    int sz = 512;
    if (type == 0 || type == 1) sz = 2048;
    else if (type == 2) sz = 1024;
    else if (type == 3 || type == 4) sz = 256;
    unsigned char* d=new unsigned char[sz*sz*4]; // RGBA for height map
    for(int y=0;y<sz;y++) for(int x=0;x<sz;x++) {
        float r=128,g=128,b=128,h=128; // h = height for parallax
        if(type==0 && isParkingLevel(gCurrentLevel)) { // parking walls: concrete + lower caution stripe
            float u = (float)x / (float)sz;
            float v = (float)y / (float)sz;
            float rough = perlin(x * 0.10f, y * 0.10f, 4) * 20.0f;
            float speck = perlin(x * 0.42f + 3.0f, y * 0.42f + 7.0f, 2) * 10.0f;
            float seam = (fabsf(fmodf(u * 6.0f, 1.0f) - 0.5f) < 0.02f) ? 12.0f : 0.0f;
            float stripeBand = (v > 0.08f && v < 0.22f) ? 1.0f : 0.0f;
            float stripePattern = (fmodf((u + v * 0.8f) * 14.0f, 1.0f) < 0.5f) ? 1.0f : 0.0f;
            float cautionY = stripeBand * (stripePattern > 0.5f ? 75.0f : -18.0f);
            r = 118.0f + rough * 0.7f + speck * 0.4f + seam + cautionY;
            g = 120.0f + rough * 0.7f + speck * 0.4f + seam + cautionY * 0.84f;
            b = 124.0f + rough * 0.8f + speck * 0.5f + seam - cautionY * 0.65f;
            h = 128.0f + rough * 0.35f + seam * 0.8f;
        } else if(type==0) { // wall - vintage yellow wallpaper with damask pattern
            // Normalized UV for resolution-independent pattern scaling
            float u = (float)x / (float)sz;
            float v = (float)y / (float)sz;
            
            // Base wallpaper color - warm aged yellow
            float baseR = 195, baseG = 175, baseB = 85;
            
            // --- Vertical stripes: 16 stripes across texture, two-tone ---
            float stripePhase = u * 16.0f;
            float stripeFrac = stripePhase - floorf(stripePhase);
            // Smoothstep-style transition between light and dark bands
            float stripeT = 0.5f + 0.5f * sinf(stripeFrac * 6.28318f);
            float stripeShade = stripeT * 8.0f;
            
            // --- Damask diamond motif: tiled 8x12, offset rows ---
            float motifU = u * 8.0f;
            float motifV = v * 12.0f;
            float colIdx = floorf(motifU);
            // Offset every other column by half a cell for brick-like layout
            float adjV = motifV + (fmodf(colIdx, 2.0f) < 1.0f ? 0.0f : 0.5f);
            float mu = motifU - floorf(motifU); // 0-1 within cell
            float mv = adjV - floorf(adjV);     // 0-1 within cell
            // Diamond distance from cell center
            float dcx = fabsf(mu - 0.5f) * 2.0f;
            float dcy = fabsf(mv - 0.5f) * 2.0f;
            float diamond = 1.0f - (dcx + dcy);
            if(diamond < 0.0f) diamond = 0.0f;
            // Inner detail ring
            float innerD = 1.0f - (dcx * 1.6f + dcy * 1.6f);
            if(innerD < 0.0f) innerD = 0.0f;
            // Floral accent at diamond center
            float centerDist = sqrtf((mu-0.5f)*(mu-0.5f) + (mv-0.5f)*(mv-0.5f));
            float floral = (centerDist < 0.12f) ? (0.12f - centerDist) * 50.0f : 0.0f;
            float motifValue = diamond * 10.0f + innerD * 5.0f + floral;
            
            // --- Thin horizontal decorative lines ---
            float hLineV = v * 24.0f;
            float hLineFrac = hLineV - floorf(hLineV);
            float hLine = (fabsf(hLineFrac - 0.5f) < 0.035f) ? 5.0f : 0.0f;
            
            // --- Fine paper/fabric grain ---
            float grain = perlin(x * 0.5f, y * 0.5f, 3) * 5.0f
                        + perlin(x * 1.2f, y * 1.2f, 2) * 2.5f;
            
            // --- Large-scale aging color variation ---
            float colorVar = perlin(x * 0.008f, y * 0.008f, 4) * 14.0f;
            
            // --- Dirt/age stains ---
            float stain = perlin(x * 0.005f, y * 0.006f, 5);
            float dirtMask = (stain > 0.4f) ? (stain - 0.4f) * 28.0f : 0.0f;
            
            // --- Water damage near bottom edge ---
            float waterDamage = 0.0f;
            if(v > 0.75f) {
                float wetness = perlin(x * 0.015f, y * 0.01f, 4);
                waterDamage = wetness * (v - 0.75f) * 80.0f;
            }
            
            // Combine all layers
            r = baseR + stripeShade + motifValue + hLine + grain + colorVar - dirtMask - waterDamage * 0.7f;
            g = baseG + stripeShade * 0.9f + motifValue * 0.9f + hLine * 0.9f + grain + colorVar * 0.85f - dirtMask * 1.2f - waterDamage;
            b = baseB + stripeShade * 0.4f + motifValue * 0.4f + hLine * 0.4f + grain * 0.5f + colorVar * 0.4f - dirtMask * 0.8f - waterDamage * 0.6f;
            
            // Height map for parallax - raised pattern
            h = 128.0f + motifValue * 2.0f + stripeShade * 0.5f + grain * 0.3f - dirtMask * 0.5f;
            
            // Rare peeling effect
            float peelNoise = perlin(x * 0.003f + 7.0f, y * 0.003f + 3.0f, 3);
            if(peelNoise > 0.75f) {
                float peelAmount = (peelNoise - 0.75f) * 80.0f;
                r -= peelAmount * 0.3f;
                g -= peelAmount * 0.4f;
                b -= peelAmount * 0.2f;
                h -= peelAmount * 0.8f;
            }
            
        } else if(type==1 && isParkingLevel(gCurrentLevel)) { // parking floor - painted concrete
            float rough = perlin(x * 0.08f, y * 0.08f, 4) * 24.0f;
            float crack = perlin(x * 0.025f + 9.0f, y * 0.025f + 2.0f, 4);
            float crackMask = crack > 0.62f ? (crack - 0.62f) * 85.0f : 0.0f;
            float strip = (fabsf(fmodf((float)x, 192.0f) - 96.0f) < 6.0f) ? 42.0f : 0.0f;
            r = 102.0f + rough * 0.8f - crackMask * 0.45f + strip * 1.25f;
            g = 103.0f + rough * 0.82f - crackMask * 0.4f + strip * 1.2f;
            b = 106.0f + rough * 0.90f - crackMask * 0.35f + strip * 0.25f;
            h = 128.0f + rough * 0.4f - crackMask * 0.2f;
        } else if(type==1) { // floor - detailed industrial carpet
            // Base carpet color - worn brown/tan
            float baseR = 130, baseG = 110, baseB = 70;
            
            // Carpet pile texture - random fiber direction
            float fiberAngle = perlin(x * 0.3f + 5.0f, y * 0.3f + 8.0f, 2) * 3.14159f;
            float fiberX = cosf(fiberAngle);
            float fiberY = sinf(fiberAngle);
            float fiberHighlight = perlin(x * 0.5f + fiberX * 2.0f, y * 0.5f + fiberY * 2.0f, 3) * 15.0f;
            
            // Carpet weave pattern - small loops
            float loopX = fmodf(x + perlin(y * 0.1f, 0, 2) * 3.0f, 8.0f);
            float loopY = fmodf(y + perlin(x * 0.1f, 0, 2) * 3.0f, 8.0f);
            float loopDist = sqrtf((loopX - 4.0f) * (loopX - 4.0f) + (loopY - 4.0f) * (loopY - 4.0f));
            float loopPattern = (loopDist < 3.0f) ? (3.0f - loopDist) * 3.0f : 0.0f;
            
            // Large color variation - worn paths
            float wearPath = perlin(x * 0.025f, y * 0.025f, 5) * 30.0f;
            
            // Dirt accumulation
            float dirt = perlin(x * 0.04f, y * 0.04f, 4);
            float dirtAmount = (dirt > 0.4f) ? (dirt - 0.4f) * 50.0f : 0.0f;
            
            // Stains
            float stainNoise = perlin(x * 0.02f + 3.0f, y * 0.02f + 7.0f, 4);
            float stainMask = (stainNoise > 0.65f) ? (stainNoise - 0.65f) * 60.0f : 0.0f;
            
            // Fine fiber detail
            float fineDetail = perlin(x * 0.7f, y * 0.7f, 2) * 8.0f;
            
            r = baseR + fiberHighlight + loopPattern + wearPath - dirtAmount - stainMask + fineDetail;
            g = baseG + fiberHighlight * 0.9f + loopPattern * 0.9f + wearPath * 0.85f - dirtAmount * 1.3f - stainMask * 1.2f + fineDetail * 0.9f;
            b = baseB + fiberHighlight * 0.6f + loopPattern * 0.5f + wearPath * 0.5f - dirtAmount * 0.7f - stainMask * 0.8f + fineDetail * 0.5f;
            
            // Height for parallax - fiber texture
            h = 128.0f + loopPattern * 1.5f + fiberHighlight * 0.5f - dirtAmount * 0.3f;
            
        } else if(type==2 && isParkingLevel(gCurrentLevel)) { // parking ceiling - flat concrete slabs
            float slabX = fmodf((float)x, 192.0f);
            float slabY = fmodf((float)y, 192.0f);
            float joint = (slabX < 4.0f || slabY < 4.0f) ? 16.0f : 0.0f;
            float rough = perlin(x * 0.09f, y * 0.09f, 4) * 17.0f;
            float drip = perlin(x * 0.03f + 5.0f, y * 0.03f + 1.0f, 3) * 8.0f;
            r = 132.0f + rough - joint - drip * 0.4f;
            g = 133.0f + rough - joint - drip * 0.45f;
            b = 136.0f + rough - joint - drip * 0.3f;
            h = 128.0f + rough * 0.35f - joint * 0.8f;
        } else if(type==2) { // ceiling - acoustic drop ceiling tiles with detailed texture
            int tileSize = 128;
            float lx = (float)(x % tileSize);
            float ly = (float)(y % tileSize);
            
            // Distance to tile edges
            float dx = fminf(lx, tileSize - lx);
            float dy = fminf(ly, tileSize - ly);
            float edgeDist = fminf(dx, dy);
            
            // Metal grid frame between tiles
            bool isFrame = (dx < 3.0f || dy < 3.0f);
            
            if(isFrame) {
                // Aluminum T-bar grid
                float metalBase = 180.0f;
                float metalNoise = perlin(x * 0.15f, y * 0.15f, 2) * 8.0f;
                float scratch = perlin(x * 0.5f + y * 0.2f, y * 0.5f + x * 0.2f, 2) * 5.0f;
                r = metalBase + metalNoise + scratch;
                g = metalBase - 5.0f + metalNoise + scratch;
                b = metalBase - 12.0f + metalNoise * 0.8f + scratch;
                h = 160.0f; // Frame is raised
            } else {
                // Acoustic tile surface
                float baseColor = 225.0f;
                
                // Porous acoustic texture - small holes
                float poreX = fmodf(lx * 1.15f, 9.0f);
                float poreY = fmodf(ly * 1.15f, 9.0f);
                float poreDist = sqrtf((poreX - 3.0f) * (poreX - 3.0f) + (poreY - 3.0f) * (poreY - 3.0f));
                float poreDepth = (poreDist < 1.5f) ? (1.5f - poreDist) * 10.0f : 0.0f;
                
                // Fibrous texture
                float fiber1 = perlin(x * 0.3f, y * 0.15f, 3) * 8.0f;
                float fiber2 = perlin(x * 0.15f, y * 0.3f, 3) * 8.0f;
                float fiberPattern = (fiber1 + fiber2) * 0.5f;
                
                // General surface variation
                float surfaceVar = perlin(x * 0.08f, y * 0.08f, 4) * 12.0f;
                
                // Water stains - yellow/brown discoloration
                float stainNoise = perlin(x * 0.012f + 1.0f, y * 0.012f + 2.0f, 5);
                float waterStain = (stainNoise > 0.55f) ? (stainNoise - 0.55f) * 60.0f : 0.0f;
                
                // Edge shadow (AO)
                float ao = (edgeDist < 6.0f) ? (1.0f - edgeDist / 6.0f) * 0.1f : 0.0f;
                
                // Micro detail
                float microTex = perlin(x * 0.6f, y * 0.6f, 2) * 4.0f;
                
                r = baseColor - poreDepth * 0.8f + fiberPattern + surfaceVar - waterStain * 0.6f - ao * 50.0f + microTex;
                g = baseColor - 8.0f - poreDepth * 0.8f + fiberPattern + surfaceVar * 0.95f - waterStain - ao * 45.0f + microTex;
                b = baseColor - 45.0f - poreDepth * 0.6f + fiberPattern * 0.7f + surfaceVar * 0.6f - waterStain * 0.5f - ao * 35.0f + microTex * 0.7f;
                
                // Height map - pores are indented
                h = 128.0f - poreDepth * 2.0f + fiberPattern * 0.3f - ao * 20.0f;
            }
            
        } else if(type==3) { // note/light glow sprite
            float cx=x-sz/2, cy=y-sz/2;
            float dd=sqrtf(cx*cx*0.5f+cy*cy*0.5f)/sz;
            float glow=1.0f-dd*1.8f; if(glow<0) glow=0;
            r=255*glow; g=252*glow; b=240*glow;
            h=128.0f + glow * 80.0f;
            
        } else if(type==4) { // ceiling lamp - detailed plastic diffuser panel
            float cx = x - sz * 0.5f;
            float cy = y - sz * 0.5f;
            float nx = cx / (sz * 0.5f);
            float ny = cy / (sz * 0.5f);
            float adx = fabsf(nx), ady = fabsf(ny);
            
            // Frame detection
            bool isOuterFrame = (adx > 0.88f || ady > 0.88f);
            bool isInnerFrame = (adx > 0.82f || ady > 0.82f) && !isOuterFrame;
            bool isDiffuser = (adx < 0.80f && ady < 0.80f);
            
            if(isOuterFrame) {
                // Metal frame - brushed aluminum
                float metalBase = 88.0f;
                float brushDir = (adx > ady) ? x : y;
                float brushed = sinf(brushDir * 0.8f) * 4.0f + perlin(x * 0.12f, y * 0.12f, 2) * 6.0f;
                float edgeHighlight = (adx > 0.94f || ady > 0.94f) ? 15.0f : 0.0f;
                r = metalBase + brushed + edgeHighlight;
                g = metalBase - 3.0f + brushed + edgeHighlight;
                b = metalBase - 8.0f + brushed * 0.8f + edgeHighlight;
                h = 180.0f; // Frame is highest
                
            } else if(isInnerFrame) {
                // Inner frame lip
                float lipShade = 130.0f + perlin(x * 0.1f, y * 0.1f, 2) * 8.0f;
                r = lipShade; g = lipShade - 5.0f; b = lipShade - 12.0f;
                h = 160.0f;
                
            } else if(isDiffuser) {
                // Plastic diffuser panel with prismatic pattern
                
                // Base plastic color - slightly yellowed white
                float plasticBase = 210.0f;
                
                // Prismatic grid pattern - light diffusing bumps
                float gridX = fmodf(x + perlin(y * 0.05f, 0, 2) * 2.0f, 12.0f);
                float gridY = fmodf(y + perlin(x * 0.05f, 0, 2) * 2.0f, 12.0f);
                float prismX = (gridX < 6.0f) ? gridX : 12.0f - gridX;
                float prismY = (gridY < 6.0f) ? gridY : 12.0f - gridY;
                float prismHeight = prismX * prismY * 0.15f;
                
                // Light scatter effect - brighter in center of each prism cell
                float scatter = prismHeight * 1.5f;
                
                // Plastic grain texture
                float grain = perlin(x * 0.2f, y * 0.2f, 3) * 5.0f;
                
                // Dust and dirt accumulation
                float dustNoise = perlin(x * 0.03f + 5.0f, y * 0.03f + 9.0f, 4);
                float dust = dustNoise * 8.0f;
                
                // Dead bugs (rare dark spots)
                float bugNoise = perlin(x * 0.025f + 11.0f, y * 0.025f + 7.0f, 3);
                float bugSpot = (bugNoise > 0.78f) ? (bugNoise - 0.78f) * 100.0f : 0.0f;
                
                // Yellowing from age and heat
                float yellowing = perlin(x * 0.02f, y * 0.02f, 3) * 6.0f;
                
                // Vignette - darker at edges
                float distFromCenter = sqrtf(nx * nx + ny * ny);
                float vignette = 1.0f - distFromCenter * 0.25f;
                if(vignette < 0.65f) vignette = 0.65f;
                
                r = (plasticBase + scatter + grain - dust - bugSpot + yellowing * 0.5f) * vignette + 25.0f;
                g = (plasticBase + 5.0f + scatter + grain - dust * 1.1f - bugSpot * 1.2f - yellowing * 0.3f) * vignette + 22.0f;
                b = (plasticBase - 12.0f + scatter * 0.8f + grain * 0.7f - dust * 0.8f - bugSpot - yellowing) * vignette + 15.0f;
                
                // Height for parallax - prismatic bumps
                h = 128.0f + prismHeight * 3.0f - dust * 0.2f;
                
            } else {
                // Border transition
                float border = 145.0f + perlin(x * 0.08f, y * 0.08f, 2) * 10.0f;
                r = border; g = border - 5.0f; b = border - 12.0f;
                h = 140.0f;
            }
            
        } else if(type==5) { // generic prop texture (wood/metal utility mix)
            float grains = perlin(x * 0.14f, y * 0.14f, 4) * 14.0f;
            float ridges = sinf(x * 0.22f + perlin(y * 0.07f, x * 0.05f, 2) * 2.5f) * 9.0f;
            float rust = perlin(x * 0.035f + 4.0f, y * 0.035f + 7.0f, 4);
            float rustMask = rust > 0.45f ? (rust - 0.45f) * 90.0f : 0.0f;
            float tapeLine = fmodf((float)x, 96.0f);
            float tape = (tapeLine > 42.0f && tapeLine < 54.0f) ? 18.0f : 0.0f;
            float panel = (fmodf((float)x, 128.0f) < 4.0f || fmodf((float)y, 128.0f) < 4.0f) ? 24.0f : 0.0f;
            float card = perlin(x * 0.09f + 6.0f, y * 0.09f + 2.0f, 3) * 10.0f;
            float woodBands = sinf((x + perlin(y * 0.03f, x * 0.02f, 2) * 22.0f) * 0.08f) * 16.0f;
            float knot = perlin(x * 0.045f + 2.0f, y * 0.045f + 11.0f, 4);
            float knotMask = knot > 0.55f ? (knot - 0.55f) * 52.0f : 0.0f;
            r = 122.0f + grains * 0.55f + ridges * 0.15f + woodBands + tape * 0.35f + panel * 0.08f + card - rustMask * 0.10f - knotMask * 0.4f;
            g = 90.0f + grains * 0.42f + ridges * 0.10f + woodBands * 0.7f + tape * 0.22f + panel * 0.06f + card * 0.75f - rustMask * 0.18f - knotMask * 0.35f;
            b = 58.0f + grains * 0.30f + ridges * 0.08f + woodBands * 0.35f + tape * 0.12f + panel * 0.04f + card * 0.45f - rustMask * 0.24f - knotMask * 0.28f;
            h = 124.0f + ridges * 0.45f + grains * 0.18f + woodBands * 0.24f + knotMask * 0.32f;
        } else if(type==6) { // handheld device texture (flashlight/scanner) - painted plastic/metal
            // Purpose: avoid using the generic prop wood texture for devices.
            // Palette: neutral matte gray plastic with subtle speckle + edge wear.
            float u = (float)x / (float)sz;
            float v = (float)y / (float)sz;

            // Base neutral gray
            float base = 124.0f;
            float speck = perlin(x * 0.35f, y * 0.35f, 3) * 10.0f;
            float micro = perlin(x * 1.2f + 3.0f, y * 1.2f + 7.0f, 2) * 4.0f;

            // Panel seams (a few vertical/horizontal grooves)
            float seamV = (fabsf(fmodf(u * 6.0f, 1.0f) - 0.5f) < 0.03f) ? 18.0f : 0.0f;
            float seamH = (fabsf(fmodf(v * 4.0f, 1.0f) - 0.5f) < 0.03f) ? 14.0f : 0.0f;
            float seams = (seamV + seamH);

            // Edge wear (brighter near borders)
            float edge = fminf(fminf(u, 1.0f - u), fminf(v, 1.0f - v));
            float wear = (edge < 0.08f) ? (0.08f - edge) * 180.0f : 0.0f;
            float wearNoise = perlin(x * 0.06f + 10.0f, y * 0.06f + 2.0f, 4) * 10.0f;
            wear = wear * (0.7f + 0.3f * (wearNoise / 10.0f));

            r = base + speck + micro + seams * 0.95f + wear * 0.90f;
            g = base + speck * 0.98f + micro + seams * 0.93f + wear * 0.90f;
            b = base + speck * 0.96f + micro * 0.92f + seams * 0.90f + wear * 0.88f;

            // Height: seams and edge wear slightly raised
            h = 128.0f + seams * 0.6f + wear * 0.25f + speck * 0.25f;
        } else if(type==7) { // plush toy texture (fabric/fur)
            float u = (float)x / (float)sz;
            float v = (float)y / (float)sz;
            float fur = perlin(x * 0.42f, y * 0.42f, 4) * 16.0f;
            float weave = perlin(x * 0.18f + 5.0f, y * 0.18f + 3.0f, 3) * 9.0f;
            float patch = perlin(x * 0.035f + 9.0f, y * 0.035f + 11.0f, 4);
            float patchMask = patch > 0.62f ? (patch - 0.62f) * 45.0f : 0.0f;
            float seamV = (fabsf(fmodf(u * 4.0f, 1.0f) - 0.5f) < 0.025f) ? 12.0f : 0.0f;
            float seamH = (fabsf(fmodf(v * 5.0f, 1.0f) - 0.5f) < 0.025f) ? 9.0f : 0.0f;
            float warmth = perlin(x * 0.06f + 1.0f, y * 0.06f + 4.0f, 3) * 10.0f;
            r = 156.0f + fur + weave * 0.8f + warmth - patchMask * 0.3f;
            g = 122.0f + fur * 0.86f + weave * 0.6f + warmth * 0.6f - patchMask * 0.2f;
            b = 96.0f + fur * 0.70f + weave * 0.45f + warmth * 0.35f - patchMask * 0.1f;
            h = 128.0f + fur * 0.4f + seamV + seamH;
        }
        
        // Clamp and store RGBA
        d[(y*sz+x)*4+0]=(unsigned char)(r<0?0:(r>255?255:(int)r));
        d[(y*sz+x)*4+1]=(unsigned char)(g<0?0:(g>255?255:(int)g));
        d[(y*sz+x)*4+2]=(unsigned char)(b<0?0:(b>255?255:(int)b));
        d[(y*sz+x)*4+3]=(unsigned char)(h<0?0:(h>255?255:(int)h)); // Height in alpha
    }
    GLuint tex; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,sz,sz,0,GL_RGBA,GL_UNSIGNED_BYTE,d);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] d; return tex;
}
