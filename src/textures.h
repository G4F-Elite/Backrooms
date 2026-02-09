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

// Generate RGBA texture with height in alpha for parallax
inline GLuint genTex(int type) {
    int sz = 512;
    if (type == 0 || type == 1) sz = 2048;
    else if (type == 2) sz = 1024;
    else if (type == 3 || type == 4) sz = 256;
    unsigned char* d=new unsigned char[sz*sz*4]; // RGBA for height map
    for(int y=0;y<sz;y++) for(int x=0;x<sz;x++) {
        float r=128,g=128,b=128,h=128; // h = height for parallax
        if(type==0) { // wall - detailed yellow wallpaper with fabric texture
            // Base wallpaper color - aged yellow
            float baseR = 198, baseG = 178, baseB = 88;
            
            // Large scale pattern - vintage wallpaper vertical stripes
            float stripePhase = sinf(x * 0.08f) * 0.5f + 0.5f;
            float stripePattern = stripePhase * 12.0f;
            
            // Diamond/damask pattern overlay
            float damaskX = sinf(x * 0.12f + y * 0.06f);
            float damaskY = sinf(y * 0.12f + x * 0.06f);
            float damask = (damaskX * damaskY + 1.0f) * 0.5f * 8.0f;
            
            // Fabric weave texture - horizontal and vertical threads
            float warpThread = sinf(x * 0.6f) * 3.0f;
            float weftThread = sinf(y * 0.6f) * 3.0f;
            float weave = (warpThread + weftThread) * 0.5f;
            
            // Fine grain noise - paper/fabric texture
            float fineGrain = perlin(x * 0.25f, y * 0.25f, 3) * 12.0f;
            float microDetail = perlin(x * 0.8f, y * 0.8f, 2) * 5.0f;
            
            // Larger noise for color variation
            float colorVar = perlin(x * 0.04f, y * 0.04f, 4) * 20.0f;
            
            // Age/dirt stains
            float stain = perlin(x * 0.02f, y * 0.025f, 5);
            float dirtMask = (stain > 0.5f) ? (stain - 0.5f) * 40.0f : 0.0f;
            
            // Water damage near bottom
            float waterDamage = 0.0f;
            if(y > sz * 0.7f) {
                float wetness = perlin(x * 0.08f, y * 0.04f, 4);
                waterDamage = wetness * (y - sz * 0.7f) * 0.5f;
            }
            
            // Combine all effects
            r = baseR + stripePattern + damask + weave + fineGrain + microDetail + colorVar - dirtMask - waterDamage * 0.7f;
            g = baseG + stripePattern * 0.9f + damask * 0.9f + weave + fineGrain + microDetail * 0.9f + colorVar * 0.9f - dirtMask * 1.2f - waterDamage;
            b = baseB + stripePattern * 0.4f + damask * 0.4f + weave * 0.5f + fineGrain * 0.5f + microDetail * 0.4f + colorVar * 0.4f - dirtMask * 0.8f - waterDamage * 0.6f;
            
            // Height map for parallax - based on pattern depth
            h = 128.0f + damask * 2.0f + stripePattern * 1.5f + fineGrain - dirtMask * 0.5f;
            
            // Peeling edges effect (rare)
            if(perlin(x * 0.015f + 7.0f, y * 0.015f + 3.0f, 3) > 0.75f) {
                float peelAmount = (perlin(x * 0.015f + 7.0f, y * 0.015f + 3.0f, 3) - 0.75f) * 80.0f;
                r -= peelAmount * 0.3f;
                g -= peelAmount * 0.4f;
                b -= peelAmount * 0.2f;
                h -= peelAmount * 0.8f; // Peeling creates depth
            }
            
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
            
        } else if(type==2) { // ceiling - acoustic drop ceiling tiles with detailed texture
            int tileSize = 64;
            float lx = (float)(x % tileSize);
            float ly = (float)(y % tileSize);
            
            // Distance to tile edges
            float dx = fminf(lx, tileSize - lx);
            float dy = fminf(ly, tileSize - ly);
            float edgeDist = fminf(dx, dy);
            
            // Metal grid frame between tiles
            bool isFrame = (dx < 2.0f || dy < 2.0f);
            
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
                float poreX = fmodf(lx * 1.7f, 6.0f);
                float poreY = fmodf(ly * 1.7f, 6.0f);
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
            
        } else if(type==5) { // generic prop texture (metal/cardboard mix)
            float grains = perlin(x * 0.14f, y * 0.14f, 4) * 14.0f;
            float ridges = sinf(x * 0.22f + perlin(y * 0.07f, x * 0.05f, 2) * 2.5f) * 9.0f;
            float rust = perlin(x * 0.035f + 4.0f, y * 0.035f + 7.0f, 4);
            float rustMask = rust > 0.45f ? (rust - 0.45f) * 90.0f : 0.0f;
            float tapeLine = fmodf((float)x, 96.0f);
            float tape = (tapeLine > 42.0f && tapeLine < 54.0f) ? 18.0f : 0.0f;
            float panel = (fmodf((float)x, 128.0f) < 4.0f || fmodf((float)y, 128.0f) < 4.0f) ? 24.0f : 0.0f;
            float card = perlin(x * 0.09f + 6.0f, y * 0.09f + 2.0f, 3) * 10.0f;
            r = 116.0f + grains * 0.7f + ridges * 0.2f + tape + panel * 0.2f + card - rustMask * 0.20f;
            g = 104.0f + grains * 0.6f + ridges * 0.15f + tape * 0.8f + panel * 0.35f + card * 0.9f - rustMask * 0.42f;
            b = 86.0f + grains * 0.5f + ridges * 0.1f + tape * 0.35f + panel * 0.50f + card * 0.5f - rustMask * 0.62f;
            h = 122.0f + ridges * 0.55f + grains * 0.24f + panel * 0.7f;
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
