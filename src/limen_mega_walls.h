#pragma once
// Part of limen_mega.h: slab mesh cache, thin partitions, narrow corridors, windows, moldings.

// ============================================================================
// PER-SLAB IMMUTABLE GPU MESH CACHE
// ============================================================================
struct LmCachedSlabMesh {
    LimenRect rect;
    int palette = 0, altPalette = 1;
    LmXenograft xeno = {};
    unsigned int builtSeed = 0, builtEpoch = 0;
    unsigned int storyEpoch = 0;       // lmStoryEpochOf() when it was built
    unsigned int siteSig = 0;          // which of the slab's own sites were triggered at build time
    unsigned long long vertHash = 0;   // bench self-check only
    int builtLevel = -999;
    int unstruct = 0;                  // lmUnstructKind(): 0 = architecture, 1..4 = mega-unstructure
    float uinfo[3] = {0.0f, 0.0f, 0.0f};   // centre (x, roof, z) of the slab, for the shader
    unsigned long long lastUsed = 0;
    GLuint megaVAO = 0, megaVBO = 0;           int megaVC = 0;
    GLuint megaAltVAO = 0, megaAltVBO = 0;     int megaAltVC = 0;
    GLuint skyVAO = 0, skyVBO = 0;             int skyVC = 0;   // sky curtain round an unstructure
    GLuint wallVAO = 0, wallVBO = 0;           int wallVC = 0;
    GLuint dadoVAO = 0, dadoVBO = 0;           int dadoVC = 0;
    GLuint decorVAO = 0, decorVBO = 0;         int decorVC = 0;
    GLuint vitFloorVAO = 0, vitFloorVBO = 0;   int vitFloorVC = 0;
    GLuint vitCeilVAO  = 0, vitCeilVBO  = 0;   int vitCeilVC  = 0;
    GLuint xenoWallVAO = 0, xenoWallVBO = 0;   int xenoWallVC = 0;
    GLuint xenoDadoVAO = 0, xenoDadoVBO = 0;   int xenoDadoVC = 0;
    GLuint xenoFloorVAO= 0, xenoFloorVBO= 0;   int xenoFloorVC= 0;
    GLuint xenoCeilVAO = 0, xenoCeilVBO = 0;   int xenoCeilVC = 0;
    std::vector<float> lv, lvOff;
    std::vector<LimenAabb2D> cols;

    void destroyGL() {
        auto del = [](GLuint& vao, GLuint& vbo, int& vc) {
            if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
            if (vbo) { glDeleteBuffers(1, &vbo);      vbo = 0; }
            vc = 0;
        };
        del(megaVAO, megaVBO, megaVC);
        del(megaAltVAO, megaAltVBO, megaAltVC);
        del(skyVAO, skyVBO, skyVC);
        del(wallVAO, wallVBO, wallVC);
        del(dadoVAO, dadoVBO, dadoVC);
        del(decorVAO, decorVBO, decorVC);
        del(vitFloorVAO, vitFloorVBO, vitFloorVC);
        del(vitCeilVAO, vitCeilVBO, vitCeilVC);
        del(xenoWallVAO, xenoWallVBO, xenoWallVC);
        del(xenoDadoVAO, xenoDadoVBO, xenoDadoVC);
        del(xenoFloorVAO, xenoFloorVBO, xenoFloorVC);
        del(xenoCeilVAO, xenoCeilVBO, xenoCeilVC);
    }
};

inline std::unordered_map<long long, LmCachedSlabMesh> gLmSlabMeshes;
inline std::vector<long long> gLmVisibleSlabKeys;
inline std::vector<long long> gLmPrewarmSlabKeys;   // one chunk beyond the view: meshed ahead of need
inline size_t gLmSlabMeshesWanted = 0;
inline unsigned long long gLmMeshUseClock = 0;

inline void lmEmitThinPartition(std::vector<float>& wv, std::vector<float>& dv,
                                float x0, float z0, bool alongZ, bool hasDoor, float cH, float yBot = 0.0f);
inline void lmEmitNarrowCorridor(std::vector<float>& wv, int wx, int wz, float px, float pz,
                                 unsigned short flg, float cH, float yBot = 0.0f);
// A sealed vitrine room is solid from outside: wherever it touches walkable floor directly
// (no wall cell in between) that side gets an ordinary wall face.
inline bool lmSolidFromOutsideAt(int wx, int wz) {
    int t = lmTagAt(wx, wz);
    return t == LT_WALL || t == LT_VITRINE;
}

// Closes the free ends and outer corners of thin partitions around cell (wx, wz). At a lattice
// corner where partitions meet, each of the four quadrants is either bounded by a partition
// skin already or needs an L of two half-thickness faces; `topOf(cx, cz)` is the height the
// partitions owned by cell (cx, cz) are built to.
template <class TopFn>
inline void lmEmitThinJoints(std::vector<float>& wv, int wx, int wz, float yBot, TopFn&& topOf) {
    const float ht = LM_THIN_HALF;
    for (int qx = 0; qx < 2; qx++) {
        for (int qz = 0; qz < 2; qz++) {
            int vx = wx + qx, vz = wz + qz;
            unsigned short fHere = lmFlagAt(vx, vz);
            bool pPZ = (fHere & LF_THIN_W) != 0;                       // partition leaving toward +Z
            bool pPX = (fHere & LF_THIN_N) != 0;                       // toward +X
            bool pNZ = (lmFlagAt(vx, vz - 1) & LF_THIN_W) != 0;        // toward -Z
            bool pNX = (lmFlagAt(vx - 1, vz) & LF_THIN_N) != 0;        // toward -X
            if (!(pPZ || pPX || pNZ || pNX)) continue;
            int sx = qx ? -1 : 1, sz = qz ? -1 : 1;                    // this cell's quadrant
            if ((sx > 0 ? pPX : pNX) || (sz > 0 ? pPZ : pNZ)) continue;
            float top = 0.0f;
            if (pPZ || pPX) top = fmaxf(top, topOf(vx, vz));
            if (pNZ) top = fmaxf(top, topOf(vx, vz - 1));
            if (pNX) top = fmaxf(top, topOf(vx - 1, vz));
            float X = vx * CS, Z = vz * CS;
            if (sx * sz > 0) {
                lmPushWallSpan(wv, X + sx * ht, yBot, Z, 0.0f, sz * ht, top);
                lmPushWallSpan(wv, X + sx * ht, yBot, Z + sz * ht, -sx * ht, 0.0f, top);
            } else {
                lmPushWallSpan(wv, X + sx * ht, yBot, Z + sz * ht, 0.0f, -sz * ht, top);
                lmPushWallSpan(wv, X, yBot, Z + sz * ht, sx * ht, 0.0f, top);
            }
        }
    }
}

// Cuts a 3.10 m x 1.42 m glazed viewing window (sill 0.96 m .. lintel 2.38 m) in a CS wall face.
inline float lmDrunkVertexCeilOffset(int vx, int vz);
// A cell whose ceiling is pitched or sagging (any of its four lattice corners is displaced):
// flat things hung at the nominal ceiling height would cut through it.
inline bool lmCellCeilWarped(int wx, int wz) {
    return lmDrunkVertexCeilOffset(wx, wz) != 0.0f || lmDrunkVertexCeilOffset(wx + 1, wz) != 0.0f ||
           lmDrunkVertexCeilOffset(wx, wz + 1) != 0.0f || lmDrunkVertexCeilOffset(wx + 1, wz + 1) != 0.0f;
}
// How far the warped ceiling of a 1-storey cell is displaced at a point (same triangles as the mesh).
inline float lmCeilWarpAt(float x, float z) {
    int wx = (int)floorf(x / CS), wz = (int)floorf(z / CS);
    float u = x / CS - (float)wx, v = z / CS - (float)wz;
    float y00 = lmDrunkVertexCeilOffset(wx, wz), y01 = lmDrunkVertexCeilOffset(wx, wz + 1);
    float y11 = lmDrunkVertexCeilOffset(wx + 1, wz + 1), y10 = lmDrunkVertexCeilOffset(wx + 1, wz);
    return (v >= u) ? (y00 + (y01 - y00) * v + (y11 - y01) * u) : (y00 + (y10 - y00) * u + (y11 - y10) * v);
}
// The lowest the ceiling gets over a flat panel of the given size: the panel hangs clear of it.
inline float lmCeilWarpUnderPanel(float x, float z, float sx, float sz) {
    float m = lmCeilWarpAt(x, z);
    for (int q = 0; q < 4; q++) {
        float w = lmCeilWarpAt(x + ((q & 1) ? 0.5f : -0.5f) * sx, z + ((q & 2) ? 0.5f : -0.5f) * sz);
        if (w < m) m = w;
    }
    return m;
}

inline void lmEmitWindowedWall(std::vector<float>& wv, float x, float z, float wdx, float wdz, float topH, float sillY = 0.96f, float yBot = 0.0f) {
    const float winTop = 2.38f, jFrac = 0.19f; // 0.19 * 5m = 0.95m jambs -> 3.10m opening
    lmPushWallSpan(wv, x, yBot, z, wdx, wdz, sillY);
    if (topH > winTop + 0.01f) lmPushWallSpan(wv, x, winTop, z, wdx, wdz, topH);
    lmPushWallSpan(wv, x, sillY, z, wdx * jFrac, wdz * jFrac, winTop);
    lmPushWallSpan(wv, x + wdx * (1.0f - jFrac), sillY, z + wdz * (1.0f - jFrac), wdx * jFrac, wdz * jFrac, winTop);
}

inline bool lmIsVitrineWindowWorld(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return false;
    const LimenSlab& s = lmSlab(r.j, r.i);
    return lmSlabHasSite(s, wx - r.x0 - 1, wz - r.z0 - 1, LS_VITRINE_WIN);
}

// ============================================================================
// PROCEDURAL ARCHITECTURAL SURFACE DETAIL (DNA + SLAB GENES, PER-SLAB VAO)
// Baseboards, two-tone dado/wainscoting + chair rails, opening architraves,
// cornices, Armstrong T-bar ceiling grids, floor expansion strips, and rhythmic
// outlets / switches / HVAC louver grilles — with LG_DECAY missing/fallen parts.
// ============================================================================
struct LmSlabArchSpec {
    float plinthH, plinthD;
    bool  hasDado;
    float dadoH;
    float corniceH;
    bool  hasCeilGrid;
    int   ceilSub;
    float pilasterW;
    int   fixPitch;
    int   plinthMat, trimMat, gridMat, plateMat;
    bool  verticalBattens;
};

inline LmSlabArchSpec lmMakeSlabArchSpec(const LimenLevelDNA& dna, const LimenGenome& g, bool isVault) {
    LmSlabArchSpec a{};
    int bbRaw = (int)dna.baseboardSize + ((int)g.g[LG_MASS] - 128) / 4 + ((int)g.g[LG_ORDER] - 128) / 6;
    a.plinthH = (isVault ? 0.14f : 0.09f) + (float)lmClamp255(bbRaw) * (isVault ? 0.18f : 0.14f) / 255.0f;
    a.plinthD = 0.032f + a.plinthH * 0.15f;

    int dadoScore = (int)dna.dadoHeight + ((int)g.g[LG_USE] - 108) / 3 +
                    ((int)g.g[LG_ORDER] - 112) / 5 + ((int)g.g[LG_HUE] - 128) / 8;
    a.hasDado = (dna.dadoHeight >= 38 && dadoScore >= 40);
    a.dadoH   = 0.82f + (float)lmClampInt(dadoScore - 40, 0, 215) * 0.80f / 215.0f;

    int cornScore = (int)dna.corniceSize + ((int)g.g[LG_ORDER] - 108) / 3 + ((int)g.g[LG_MASS] - 128) / 5;
    a.corniceH = (cornScore >= 36) ? (0.08f + (float)lmClampInt(cornScore - 36, 0, 219) * 0.16f / 219.0f) : 0.0f;

    int cgScore = (int)dna.ceilGridStrength + ((int)g.g[LG_USE] - 96) / 4 + ((int)g.g[LG_ORDER] - 112) / 5;
    a.hasCeilGrid = (dna.ceilGridStrength >= 24 && cgScore >= 50);
    a.ceilSub = (dna.ceilGridStep < 88) ? 4 : ((dna.ceilGridStep < 180) ? 3 : 2);

    a.pilasterW = 0.14f + (float)lmClamp255((int)dna.trimPresence + ((int)g.g[LG_MASS] - 128) / 4) * 0.14f / 255.0f;
    a.fixPitch  = 5 - lmClampInt(((int)dna.fixtureRhythm + (int)g.g[LG_USE]) / 160, 0, 2);

    bool metallic = (dna.wallSurf.sheen > 152);
    a.plinthMat = metallic ? DECOR_MAT_DARK_METAL : DECOR_MAT_TRIM;
    a.trimMat   = DECOR_MAT_TRIM;
    a.gridMat   = (dna.ceilSurf.sheen > 136 || metallic) ? DECOR_MAT_METAL : DECOR_MAT_WHITE;
    a.plateMat  = metallic ? DECOR_MAT_METAL : DECOR_MAT_WHITE;
    a.verticalBattens = (dna.wallSurf.stripe > 105 || g.g[LG_ORDER] > 125);
    return a;
}

inline void lmPushDecorQuadRaw(std::vector<float>& dv, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, int mat) {
    DecorUv t = decorTileUv(mat);
    float uSpan = (b - a).len() / DECOR_TEXEL_METERS;
    float vSpan = (d - a).len() / DECOR_TEXEL_METERS;
    if (uSpan > 1.0f) uSpan = 1.0f; else if (uSpan < 0.12f) uSpan = 0.12f;
    if (vSpan > 1.0f) vSpan = 1.0f; else if (vSpan < 0.12f) vSpan = 0.12f;
    float vv[] = {
        a.x,a.y,a.z, t.u0,             t.v0,             n.x,n.y,n.z,
        b.x,b.y,b.z, t.u0+uSpan*t.du,  t.v0,             n.x,n.y,n.z,
        c.x,c.y,c.z, t.u0+uSpan*t.du,  t.v0+vSpan*t.dv,  n.x,n.y,n.z,
        a.x,a.y,a.z, t.u0,             t.v0,             n.x,n.y,n.z,
        c.x,c.y,c.z, t.u0+uSpan*t.du,  t.v0+vSpan*t.dv,  n.x,n.y,n.z,
        d.x,d.y,d.z, t.u0,             t.v0+vSpan*t.dv,  n.x,n.y,n.z
    };
    dv.insert(dv.end(), vv, vv + 48);
}

// Lean 1-or-2-quad architectural molding strip along T=(tx,0,tz), protruding along N=(nx,0,nz).
inline void lmEmitWallMolding(std::vector<float>& dv, float bx, float bz, float tx, float tz,
                              float nx, float nz, float u0, float u1, float y0, float y1,
                              float proj, int mat, bool capTop = false, bool capBot = false) {
    if (u1 <= u0 + 0.02f || y1 <= y0 + 0.005f) return;
    Vec3 T(tx, 0.0f, tz), N(nx, 0.0f, nz);
    Vec3 p0 = Vec3(bx, 0.0f, bz) + T * u0;
    Vec3 p1 = Vec3(bx, 0.0f, bz) + T * u1;
    if (!capTop && !capBot && proj >= 0.025f) {
        // Bevelled single-quad plinth/cove profile (6 verts instead of 18, catches overhead light!)
        bool isCove = (y0 > 1.8f);
        Vec3 bot0 = isCove ? (p0 + N * 0.008f) : (p0 + N * proj);
        Vec3 bot1 = isCove ? (p1 + N * 0.008f) : (p1 + N * proj);
        Vec3 top0 = isCove ? (p0 + N * proj)   : (p0 + N * 0.008f);
        Vec3 top1 = isCove ? (p1 + N * proj)   : (p1 + N * 0.008f);
        Vec3 nrm  = (N * 0.88f + Vec3(0.0f, isCove ? -0.47f : 0.47f, 0.0f)).norm();
        lmPushDecorQuadRaw(dv, Vec3(bot0.x, y0, bot0.z), Vec3(bot1.x, y0, bot1.z),
                               Vec3(top1.x, y1, top1.z), Vec3(top0.x, y1, top0.z), nrm, mat);
        return;
    }
    Vec3 f0 = p0 + N * proj, f1 = p1 + N * proj;
    lmPushDecorQuadRaw(dv, Vec3(f0.x, y0, f0.z), Vec3(f1.x, y0, f1.z),
                           Vec3(f1.x, y1, f1.z), Vec3(f0.x, y1, f0.z), N, mat);
    if (capTop) {
        lmPushDecorQuadRaw(dv, Vec3(p0.x, y1, p0.z), Vec3(f0.x, y1, f0.z),
                               Vec3(f1.x, y1, f1.z), Vec3(p1.x, y1, p1.z), Vec3(0, 1, 0), mat);
    }
    if (capBot) {
        lmPushDecorQuadRaw(dv, Vec3(f0.x, y0, f0.z), Vec3(p0.x, y0, p0.z),
                               Vec3(p1.x, y0, p1.z), Vec3(f1.x, y0, f1.z), Vec3(0, -1, 0), mat);
    }
}

// Single-quad ceiling T-bar flange (6 verts; merged across contiguous slab runs!).
inline void lmEmitCeilRunner(std::vector<float>& dv, float x0, float z0, float x1, float z1,
                             float cH, float lipH, int mat) {
    if (x1 <= x0 + 0.01f || z1 <= z0 + 0.01f) return;
    float yB = cH - lipH;
    lmPushDecorQuadRaw(dv, Vec3(x0, yB, z0), Vec3(x1, yB, z0), Vec3(x1, yB, z1), Vec3(x0, yB, z1), Vec3(0, -1, 0), mat);
}

inline int lmPureDoorAxisAt(int wx, int wz) {
    int t = lmTagAt(wx, wz);
    if (!lmIsWalkableTag(t) || t == LT_HALL || t == LT_BRIDGE) return 0;
    unsigned short f = lmFlagAt(wx, wz);
    if (f & (LF_NARROW_X | LF_NARROW_Z)) return 0;
    if (lmZoneAt(wx, wz) & (LZ_PLUG_CORR | LZ_XENO)) return 0;
    bool wL = (lmTagAt(wx - 1, wz) == LT_WALL), wR = (lmTagAt(wx + 1, wz) == LT_WALL);
    bool wN = (lmTagAt(wx, wz - 1) == LT_WALL), wS = (lmTagAt(wx, wz + 1) == LT_WALL);
    if (wL && wR && !wN && !wS) return 1;
    if (wN && wS && !wL && !wR) return 2;
    return 0;
}

inline bool lmWallFaceHasCenteredDoor(const LimenSlab& s, int slx, int slz, int nslx, int nslz) {
    if (slx >= 0 && slz >= 0 && slx < s.w && slz < s.h) {
        if (lmSlabHasSite(s, slx, slz, LS_DOOR_GAP)) return true;
        if ((s.zone[slx][slz] & LZ_PLUG_CORR) != 0) return true;
    }
    if (nslx >= 0 && nslz >= 0 && nslx < s.w && nslz < s.h) {
        if (lmSlabHasSite(s, nslx, nslz, LS_WALL_DOOR)) return true;
    }
    return false;
}
