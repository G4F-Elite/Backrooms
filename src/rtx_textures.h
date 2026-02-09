#pragma once
#include <glad/glad.h>
#include <cmath>

// ============================================================================
// RTX TEXTURES - PBR material generation
// Normal maps and roughness/metallic maps for each surface type
// Generated procedurally to match existing textures
// ============================================================================

// Helper: generate normal from height differences (Sobel filter)
inline void heightToNormal(const float* heightMap, unsigned char* normalData, int sz, float strength) {
    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            int xp = (x + 1) % sz, xm = (x - 1 + sz) % sz;
            int yp = (y + 1) % sz, ym = (y - 1 + sz) % sz;
            
            // Sobel derivatives
            float dX = heightMap[y * sz + xp] - heightMap[y * sz + xm];
            float dY = heightMap[yp * sz + x] - heightMap[ym * sz + x];
            
            // Normal vector
            float nx = -dX * strength;
            float ny = -dY * strength;
            float nz = 1.0f;
            
            // Normalize
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0.001f) { nx /= len; ny /= len; nz /= len; }
            
            // Encode to [0,255]
            int idx = (y * sz + x) * 3;
            normalData[idx + 0] = (unsigned char)((nx * 0.5f + 0.5f) * 255.0f);
            normalData[idx + 1] = (unsigned char)((ny * 0.5f + 0.5f) * 255.0f);
            normalData[idx + 2] = (unsigned char)((nz * 0.5f + 0.5f) * 255.0f);
        }
    }
}

// Uses noise2d() and perlin() from textures.h (must be included before this header)

// ============================================================================
// Generate normal map for a surface type
// ============================================================================
inline GLuint genNormalMap(int type) {
    const int sz = ((type == 3) || (type == 4)) ? 256 : 512;
    float* heightMap = new float[sz * sz];
    unsigned char* normalData = new unsigned char[sz * sz * 3];
    
    float normalStrength = 2.0f;
    
    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            float h = 0.5f;
            
            if (type == 0) { // Wall - wallpaper texture bumps
                float damaskX = sinf(x * 0.12f + y * 0.06f);
                float damaskY = sinf(y * 0.12f + x * 0.06f);
                float damask = (damaskX * damaskY + 1.0f) * 0.5f;
                
                float weave = (sinf(x * 0.6f) + sinf(y * 0.6f)) * 0.1f;
                float grain = perlin(x * 0.25f, y * 0.25f, 3) * 0.15f;
                float micro = perlin(x * 0.8f, y * 0.8f, 2) * 0.08f;
                
                // Peeling creates strong normal bumps
                float peel = perlin(x * 0.015f + 7.0f, y * 0.015f + 3.0f, 3);
                float peelBump = (peel > 0.75f) ? (peel - 0.75f) * 2.0f : 0.0f;
                
                h = 0.5f + damask * 0.15f + weave + grain + micro - peelBump * 0.3f;
                normalStrength = 2.5f;
                
            } else if (type == 1) { // Floor - carpet fiber bumps
                float fiberAngle = perlin(x * 0.3f + 5.0f, y * 0.3f + 8.0f, 2) * 3.14159f;
                float fiberX = cosf(fiberAngle);
                float fiberY = sinf(fiberAngle);
                float fiberH = perlin(x * 0.5f + fiberX * 2.0f, y * 0.5f + fiberY * 2.0f, 3) * 0.2f;
                
                float loopX = fmodf(x + perlin(y * 0.1f, 0, 2) * 3.0f, 8.0f);
                float loopY = fmodf(y + perlin(x * 0.1f, 0, 2) * 3.0f, 8.0f);
                float loopDist = sqrtf((loopX - 4.0f) * (loopX - 4.0f) + (loopY - 4.0f) * (loopY - 4.0f));
                float loopH = (loopDist < 3.0f) ? (3.0f - loopDist) / 3.0f * 0.12f : 0.0f;
                
                float fine = perlin(x * 0.7f, y * 0.7f, 2) * 0.06f;
                
                h = 0.5f + fiberH + loopH + fine;
                normalStrength = 1.8f;
                
            } else if (type == 2) { // Ceiling - acoustic tile texture
                int tileSize = 64;
                float lx = (float)(x % tileSize);
                float ly = (float)(y % tileSize);
                float dx = fminf(lx, tileSize - lx);
                float dy = fminf(ly, tileSize - ly);
                float edgeDist = fminf(dx, dy);
                bool isFrame = (dx < 2.0f || dy < 2.0f);
                
                if (isFrame) {
                    h = 0.8f + perlin(x * 0.15f, y * 0.15f, 2) * 0.05f;
                } else {
                    // Pore texture
                    float poreX = fmodf(lx * 1.7f, 6.0f);
                    float poreY = fmodf(ly * 1.7f, 6.0f);
                    float poreDist = sqrtf((poreX - 3.0f) * (poreX - 3.0f) + (poreY - 3.0f) * (poreY - 3.0f));
                    float poreH = (poreDist < 1.5f) ? -(1.5f - poreDist) / 1.5f * 0.2f : 0.0f;
                    
                    float fiber = (perlin(x * 0.3f, y * 0.15f, 3) + perlin(x * 0.15f, y * 0.3f, 3)) * 0.04f;
                    float ao = (edgeDist < 6.0f) ? -(1.0f - edgeDist / 6.0f) * 0.1f : 0.0f;
                    
                    h = 0.5f + poreH + fiber + ao;
                }
                normalStrength = 2.0f;
                
            } else if (type == 3) { // Light glow - smooth dome
                float cx = x - sz / 2, cy = y - sz / 2;
                float dd = sqrtf(cx * cx + cy * cy) / (float)sz;
                h = (1.0f - dd * 2.0f);
                if (h < 0) h = 0;
                normalStrength = 0.5f;
                
            } else if (type == 4) { // Ceiling lamp - prismatic diffuser
                float cx = x - sz * 0.5f;
                float cy = y - sz * 0.5f;
                float nx = cx / (sz * 0.5f);
                float ny = cy / (sz * 0.5f);
                float adx = fabsf(nx), ady = fabsf(ny);
                
                if (adx > 0.88f || ady > 0.88f) {
                    h = 0.9f; // Frame raised
                } else if (adx > 0.82f || ady > 0.82f) {
                    h = 0.75f;
                } else {
                    float gridX = fmodf(x + perlin(y * 0.05f, 0, 2) * 2.0f, 12.0f);
                    float gridY = fmodf(y + perlin(x * 0.05f, 0, 2) * 2.0f, 12.0f);
                    float prismX = (gridX < 6.0f) ? gridX : 12.0f - gridX;
                    float prismY = (gridY < 6.0f) ? gridY : 12.0f - gridY;
                    float prismH = prismX * prismY * 0.005f;
                    h = 0.5f + prismH;
                }
                normalStrength = 1.5f;
                
            } else if (type == 5) { // Props - metal/cardboard
                float ridges = sinf(x * 0.22f + perlin(y * 0.07f, x * 0.05f, 2) * 2.5f) * 0.12f;
                float grain = perlin(x * 0.14f, y * 0.14f, 4) * 0.1f;
                h = 0.5f + ridges + grain;
                normalStrength = 3.0f;
            }
            
            heightMap[y * sz + x] = h;
        }
    }
    
    heightToNormal(heightMap, normalData, sz, normalStrength);
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sz, sz, 0, GL_RGB, GL_UNSIGNED_BYTE, normalData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    delete[] heightMap;
    delete[] normalData;
    return tex;
}

// ============================================================================
// Generate material map (R=roughness, G=metallic)
// ============================================================================
inline GLuint genMaterialMap(int type) {
    const int sz = ((type == 3) || (type == 4)) ? 256 : 512;
    unsigned char* data = new unsigned char[sz * sz * 2]; // RG
    
    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            float roughness = 0.5f;
            float metallic = 0.0f;
            
            if (type == 0) { // Wall - varying roughness from wallpaper wear
                roughness = 0.65f + perlin(x * 0.08f, y * 0.08f, 3) * 0.15f;
                
                // Water damage areas are smoother (wet)
                if (y > sz * 0.7f) {
                    float wetness = perlin(x * 0.08f, y * 0.04f, 4);
                    roughness -= wetness * 0.15f;
                }
                
                // Peeling areas expose rougher substrate
                float peel = perlin(x * 0.015f + 7.0f, y * 0.015f + 3.0f, 3);
                if (peel > 0.75f) {
                    roughness += 0.1f;
                }
                metallic = 0.0f;
                
            } else if (type == 1) { // Floor - carpet is rough, worn paths smoother
                roughness = 0.82f + perlin(x * 0.04f, y * 0.04f, 3) * 0.1f;
                
                // Worn/wet areas - slightly reflective
                float wear = perlin(x * 0.025f, y * 0.025f, 5);
                if (wear > 0.5f) {
                    roughness -= (wear - 0.5f) * 0.25f;
                }
                
                // Stains can be glossy
                float stain = perlin(x * 0.02f + 3.0f, y * 0.02f + 7.0f, 4);
                if (stain > 0.65f) {
                    roughness -= 0.08f;
                }
                metallic = 0.0f;
                
            } else if (type == 2) { // Ceiling tiles - mostly matte
                int tileSize = 64;
                float dx = fminf((float)(x % tileSize), tileSize - (float)(x % tileSize));
                float dy = fminf((float)(y % tileSize), tileSize - (float)(y % tileSize));
                bool isFrame = (dx < 2.0f || dy < 2.0f);
                
                if (isFrame) {
                    // Metal frame - somewhat reflective
                    roughness = 0.45f + perlin(x * 0.1f, y * 0.1f, 2) * 0.1f;
                    metallic = 0.6f;
                } else {
                    // Acoustic tile - very matte
                    roughness = 0.9f + perlin(x * 0.08f, y * 0.08f, 3) * 0.05f;
                    metallic = 0.0f;
                }
                
            } else if (type == 3) { // Light glow - emissive, smooth
                roughness = 0.2f;
                metallic = 0.0f;
                
            } else if (type == 4) { // Ceiling lamp
                float cx = x - sz * 0.5f;
                float cy = y - sz * 0.5f;
                float nx = cx / (sz * 0.5f);
                float ny = cy / (sz * 0.5f);
                float adx = fabsf(nx), ady = fabsf(ny);
                
                if (adx > 0.82f || ady > 0.82f) {
                    // Metal frame
                    roughness = 0.35f + perlin(x * 0.12f, y * 0.12f, 2) * 0.1f;
                    metallic = 0.7f;
                } else {
                    // Plastic diffuser - semi-glossy
                    roughness = 0.55f + perlin(x * 0.05f, y * 0.05f, 3) * 0.1f;
                    metallic = 0.0f;
                }
                
            } else if (type == 5) { // Props - mixed metal/cardboard
                float rust = perlin(x * 0.035f + 4.0f, y * 0.035f + 7.0f, 4);
                if (rust > 0.45f) {
                    roughness = 0.7f + (rust - 0.45f) * 0.4f;
                    metallic = 0.3f;
                } else {
                    roughness = 0.75f + perlin(x * 0.1f, y * 0.1f, 3) * 0.1f;
                    metallic = 0.05f;
                }
            }
            
            // Clamp
            if (roughness < 0.05f) roughness = 0.05f;
            if (roughness > 1.0f) roughness = 1.0f;
            if (metallic < 0.0f) metallic = 0.0f;
            if (metallic > 1.0f) metallic = 1.0f;
            
            data[(y * sz + x) * 2 + 0] = (unsigned char)(roughness * 255.0f);
            data[(y * sz + x) * 2 + 1] = (unsigned char)(metallic * 255.0f);
        }
    }
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, sz, sz, 0, GL_RG, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    delete[] data;
    return tex;
}

// ============================================================================
// PBR texture set for each surface type
// ============================================================================
struct PbrTexSet {
    GLuint normalMap;
    GLuint materialMap; // R=roughness, G=metallic
};

inline PbrTexSet genPbrTexSet(int type) {
    PbrTexSet set;
    set.normalMap = genNormalMap(type);
    set.materialMap = genMaterialMap(type);
    return set;
}
