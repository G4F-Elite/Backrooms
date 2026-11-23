#pragma once
// LIMEN genome & Level DNA: neither slabs nor levels come from a list of archetypes.
// (seed, level) synthesizes LimenLevelDNA — continuous surface parameters across the
// full pigment wheel, weighted prop-theme distributions, structural scales, lighting/fog
// poles, per-gene field modulations, and continuous proclivities for liminal anomalies.
#include "limen_core.h"
#include "limen_beat.h"

enum LimenGene {
    LG_HEIGHT = 0,   // ceiling: cramped 2.65 m ... one storey ... 54 m vault
    LG_MASS,         // how much of the volume is solid (thin drywall vs thick masonry)
    LG_GRAIN,        // size at which subdivision stops (closet ... whole slab)
    LG_ORDER,        // rhythm and identical repetition vs drift and asymmetry
    LG_CELLS,        // tendency to break floor into rooms behind doors
    LG_LATTICE,      // tendency to repeat a motif on a grid
    LG_SINK,         // tendency to drop the floor away
    LG_ABYSS,        // how far a dropped floor falls: walkable dry basin ... bottomless
    LG_SHELL,        // tendency to nest a closed low/mezzanine building inside a space
    LG_SPAN,         // tendency to cross a drop or drive a long axial spine
    LG_EDGE,         // how open the slab is toward its neighbours
    LG_DECAY,        // rate of missing, severed and shifted elements
    LG_GRAFT,        // chance that a part is grown from a foreign genome
    LG_LIGHT,        // lamp density
    LG_HUE,          // surface material & atmospheric tint interpolation across level poles
    LG_USE,          // traces of function (drives weighted theme lookup)
    LG_COUNT
};

struct LimenGenome { unsigned char g[LG_COUNT]; };

const int LM_THEME_COUNT = 13;
const int LM_MEGA_POLES  = 3;

enum LmMegaForm {
    MF_CLASSIC   = 0,   // coffered/ribbed vault with sunken basin/chasm & skybridges
    MF_ZIGGURAT  = 1,   // stepped ziggurat, sunken terraced amphitheatre, or grand wall stair
    MF_CANYON    = 2,   // narrow 5..12 storey gorge with stacked balcony galleries & bridges
    MF_HYPOSTYLE = 3,   // hypostyle column forest: modulated pitch/size, broken & hanging shafts
    MF_SILO      = 4,   // octagonal vertical shaft / cooling tower with helical ramp & oculus
    MF_HANGING   = 5,   // inverted stepped ceiling pyramid (down to 2.65 m) & floating rooms
    MF_CELLBLOCK = 6,   // 6..12 storey cellular window/balcony apartment courtyard
    MF_PLATFORM  = 7,   // aimless transit platforms with sunken trackway into blank end walls
    // Mega-unstructures ("мегабезструктуры"): non-manmade monumental phenomena
    MF_MONOLITH  = 8,   // towering overhanging raw grey monolith slabs & narrow slits in milky fog
    MF_SPIRES    = 9,   // undulating pale ash/snow terraces with colossal dark forked basalt needles in blizzard mist
    MF_GROTTO    = 10,  // dark subterranean terraced cavern dome, stalagmites/stalactites, torn sky oculus & lone crimson tree
    MF_TENDRILS  = 11,  // 3D floating skeletal earth/root ribbons & dry-grass tufts over endless sky-cloud void
    LM_MEGA_FORM_COUNT = 12
};

const int LM_UNSTRUCT_FIRST = MF_MONOLITH;
const int LM_UNSTRUCT_COUNT = LM_MEGA_FORM_COUNT - LM_UNSTRUCT_FIRST;

inline bool lmIsMegaUnstruct(int mf) { return mf >= LM_UNSTRUCT_FIRST && mf < LM_MEGA_FORM_COUNT; }

inline const char* lmMegaFormName(int mf) {
    static const char* names[LM_MEGA_FORM_COUNT] = {
        "classic", "ziggurat", "canyon", "hypostyle", "silo", "hanging", "cellblock", "platform",
        "monolith", "spires", "grotto", "rootfall"
    };
    return (mf >= 0 && mf < LM_MEGA_FORM_COUNT) ? names[mf] : "1-storey";
}

// Continuous material surface coordinates (no discrete wall/floor/mega style enums).
struct LmSurfDNA {
    unsigned char rgb[3];       // base reflectance 0..255
    unsigned char seamFreqX;    // 0..255 -> continuous joint count along U
    unsigned char seamFreqY;    // 0..255 -> continuous joint count along V
    unsigned char seamDepth;    // 0..255 -> groove darkness & parallax indentation
    unsigned char stripe;       // 0..255 -> periodic striping / ribbing / wallpaper bands
    unsigned char grain;        // 0..255 -> fiber / aggregate micro-roughness
    unsigned char grime;        // 0..255 -> water stains, drips, wear
    unsigned char sheen;        // 0..255 -> ceramic/wet gloss & acoustic hardness
    // Pattern coordinates: what makes a wall read as wallpaper, brick or boards rather than
    // "the same tile in another colour". All 0 = the plain seam/stripe/grain surface above.
    unsigned char rgb2[3];      // second pigment (stripes, ornament, checker, chips)
    unsigned char checker;      // 0..255 -> alternate blocks take the second pigment
    unsigned char plank;        // 0..255 -> wood fibre along the long side of a block + board-to-board tone drift
    unsigned char speckle;      // 0..255 -> terrazzo chips / carpet flecks / acoustic-tile pits
    unsigned char motif;        // 0..255 -> repeating half-drop ornament in the second pigment
    unsigned char motifKind;    // 0..255 -> dots ... diamonds ... quatrefoils
    unsigned char bond;         // 0..255 -> running bond: odd rows shifted (>=128: half a block)
};

// Level poles. A level is not drawn from one continuous hash any more: along every axis the
// eye reads first (wall / floor / ceiling material, lamp spectrum, kind of space) it sits at
// one pole, and `lmLvlPick` deals the poles out so that two consecutive levels never share
// one and every run of N levels goes through all N.
const int LM_WALL_POLES  = 8;
const int LM_FLOOR_POLES = 8;
const int LM_CEIL_POLES  = 5;
const int LM_LIGHT_POLES = 7;
const int LM_SPACE_POLES = 8;

inline const char* lmWallPoleName(int p) {
    static const char* n[LM_WALL_POLES] = {"paint", "stripes", "ornament", "tile", "brick", "concrete", "boards", "ribbed"};
    return (p >= 0 && p < LM_WALL_POLES) ? n[p] : "?";
}
inline const char* lmFloorPoleName(int p) {
    static const char* n[LM_FLOOR_POLES] = {"carpet", "pattern", "checker", "mosaic", "planks", "concrete", "terrazzo", "lino"};
    return (p >= 0 && p < LM_FLOOR_POLES) ? n[p] : "?";
}
inline const char* lmCeilPoleName(int p) {
    static const char* n[LM_CEIL_POLES] = {"acoustic", "plaster", "slab", "slats", "coffer"};
    return (p >= 0 && p < LM_CEIL_POLES) ? n[p] : "?";
}
inline const char* lmLightPoleName(int p) {
    static const char* n[LM_LIGHT_POLES] = {"warm", "daylight", "green", "sodium", "mercury", "haze", "red"};
    return (p >= 0 && p < LM_LIGHT_POLES) ? n[p] : "?";
}
inline const char* lmSpacePoleName(int p) {
    static const char* n[LM_SPACE_POLES] = {"vaults", "warren", "halls", "pillars", "drops", "closets", "nested", "massive"};
    return (p >= 0 && p < LM_SPACE_POLES) ? n[p] : "-";
}

// Level quirks ("приколы"). Poles change what a level is made of; a quirk changes what it is:
// one rule of the world is different there. `lmLvlPick` deals them like the poles, so two
// consecutive levels never share a quirk and any 13 levels in a row show all of them. Level 0
// has none. Past the first deal a level may carry a second quirk on top (`quirk2`).
enum LmQuirk {
    LQ_NONE = 0,
    LQ_FLOOD,      // standing water over every floor; dry basins become pools you walk the bottom of
    LQ_SKY,        // no ceilings: the maze stands open under a sky (overcast, painted clouds, night, dusk, a second ceiling far above)
    LQ_PARTY,      // every lamp its own colour, pulsing; confetti, balloons, bunting, tables laid for nobody; music behind the walls
    LQ_BLACKOUT,   // lights out: a few lamps left, phosphorescent floor dashes to follow
    LQ_GREYBOX,    // nobody finished the level: developer grid on every surface and prop
    LQ_DRUNK,      // the delirium field covers the whole level; the horizon rolls, walls sway
    LQ_WARP,       // space is bent: floor curls up like a bowl, drops away like a globe, leans, or hangs at a dutch angle
    LQ_GIANT,      // furniture at the wrong scale: giants', dolls', or both in one room
    LQ_LAVA,       // the floor is the light: glowing cracks, toxic seep or a luminous panel floor; ceilings dark
    LQ_MONO,       // one pigment for everything and one object repeated through the whole level
    LQ_NEON,       // dark shell traced with glowing tubes and a luminous floor grid
    LQ_RETRO,      // a level from an old console: wobbling vertices, 15-bit colour, fat pixels
    LM_QUIRK_COUNT
};

inline const char* lmQuirkName(int q) {
    static const char* n[LM_QUIRK_COUNT] = {
        "-", "flood", "sky", "party", "blackout", "greybox", "drunk", "warp", "giant", "lava", "mono", "neon", "retro"
    };
    return (q >= 0 && q < LM_QUIRK_COUNT) ? n[q] : "?";
}

// Debug only (F10 menu / LM_QUIRK env): >= 0 forces the main quirk of every level built afterwards.
inline int gLmQuirkForce = -1;
inline int gLmQuirkVarForce = -1;   // debug: with gLmQuirkForce, also pins the variant (quirkVar)

struct LimenLevelDNA {
    int rowPitch;                        // 20..40 cells (strictly bounded for LM_MAX=64)
    int colPitch;                        // 20..40 cells
    int originCalmRings;                 // 2 calm rings around the origin on level 0, none elsewhere
    int vaultThreshold;                  // 48..224 threshold on LG_HEIGHT for multi-storey vaults
    int maxStoreys;                      // 2..12 vault storey ceiling cap
    int transitPermille;                 // 140..860 permille chance of 300 m transit avenues
    int throughEdgeThresh;               // 210..420 sum of LG_EDGE for open colonnade seams
    int pillarLatticeThresh;             // 52..180 threshold on LG_LATTICE for hall columns

    signed char   geneBias[LG_COUNT];    // -90..+90 per-gene shift (3..5 extreme genes per level)
    unsigned char geneGain[LG_COUNT];    // 72..224 contrast gain around 128
    unsigned char genePeriod[LG_COUNT];  // 1..7 slabs bilinear noise period
    unsigned char smoothMix[LG_COUNT];   // 1..4 smooth-vs-slab weight out of 5

    unsigned char lightDensity;          // 0..255 level-wide lamp density drive
    unsigned char darkSlabChance;        // 0..55% probability of solitary-lamp dark slabs
    unsigned char lightRGB[3];           // 0..255 lamp & scatter tint
    unsigned char fogRGB[3];             // 0..255 base corridor fog tint
    unsigned char fogDensity;            // 0..255 corridor fog density parameter
    unsigned char megaFogDensity;        // 0..255 vault fog density parameter
    signed char   reverbBias;            // -35..+35 reverb tail offset
    unsigned char humGain;               // 15..220 120 Hz ballast hum strength
    unsigned char ambientGain;           // 60..210 ambient fill (128 = neutral): murk ... overlit
    unsigned char lampGain;              // 100..160 lamp output (128 = neutral)

    signed char wallPole, floorPole, ceilPole, lightPole, spacePole; // see LM_*_POLES (spacePole -1 on level 0)

    LmSurfDNA wallSurf;
    LmSurfDNA floorSurf;
    LmSurfDNA ceilSurf;
    LmSurfDNA dadoSurf;
    LmSurfDNA megaPoles[LM_MEGA_POLES];  // 3 continuous poles derived from DNA; LG_HUE blends them

    unsigned char themeWeight[LM_THEME_COUNT]; // 1..255 continuous weights (2..3 sharp peaks)
    unsigned char graftWeight[LM_THEME_COUNT]; // 1..255 contrasting alien weights for LG_GRAFT
    unsigned      domThemeMask;                // bitmask of primary theme peaks
    unsigned      graftThemeMask;              // bitmask of foreign graft peaks
    unsigned char propDensity;                 // 28..240 overall furniture budget
    unsigned char roomFurnish;                 // 45..245 fullness of room suites
    unsigned char poiChance;                   // 12..85% POI wall-set spawn rate

    // Continuous level proclivities for Stage-2 liminal excision & absurdity operators (0..255)
    unsigned char lackTendency;          // ruleMissingHost: amputated centerpiece
    unsigned char faceWallTendency;      // ruleFaceWall: furniture facing walls/corners
    unsigned char twinMutTendency;       // ruleUncannyTwin: 1 wrong detail in a clone row
    unsigned char monoFloodTendency;     // ruleMonoFlood: absurd grid of 1 repeated prop
    unsigned char waitVoidTendency;      // ruleWaitingVoid: rows facing blank wall or abyss
    unsigned char xenograftTendency;     // opXenograft: sharp rectangle leaked from another level
    unsigned char deadThreshTendency;    // opDeadThresholds: wall doors, ceiling stairs, sealed vitrines
    unsigned char phaseShiftTendency;    // sunken/elevated phase-shifted objects & corridor loops

    // Continuous architectural trim, paneling & surface-rhythm DNA (modulated per slab by genes)
    unsigned char dadoHeight;            // 0..255 -> lower-third wainscot/dado height (<38 none, else 0.78..1.62 m)
    signed char   dadoToneDelta;         // -88..+72 -> dado panel luminance offset vs upper wall
    unsigned char dadoHueShift;          // 0..255 -> continuous pigment/saturation shift on dado & trim
    unsigned char baseboardSize;         // 40..245 -> plinth height & relief (0.09..0.24 m)
    unsigned char corniceSize;           // 0..255 -> crown/frieze rail size (<34 none, else 0.08..0.24 m)
    unsigned char ceilGridStrength;      // 0..255 -> Armstrong suspended ceiling T-bar grid prominence
    unsigned char ceilGridStep;          // 0..255 -> ceiling grid module (1.25 / 1.67 / 2.50 m)
    unsigned char floorSeamStrength;     // 0..250 -> floor joint groove & expansion divider strength (0: seamless floor)
    unsigned char trimPresence;          // 45..250 -> doorway/portal architrave & pilaster relief
    unsigned char fixtureRhythm;         // 35..245 -> rhythmic outlet / switch / HVAC louver cadence

    // Continuous Level DNA proclivities across megastructure spatial forms (2..3 dominant peaks)
    unsigned char megaFormWeight[LM_MEGA_FORM_COUNT];
    unsigned      domMegaMask;           // bitmask of primary mega-form peaks

    // Continuous architectural delirium ("пьянка") field parameters: mostly sober, surging in
    // low-frequency world pockets whose frequency grows smoothly with |level|.
    unsigned char deliriumDrive;         // 0..255 pocket intensity & frequency drive
    unsigned char deliriumThresh;        // 105..198 noise threshold below which world is sober
    unsigned char deliriumPeriod;        // 13..25 cells (~65..125 m pocket wavelength)

    // Level quirks (see LmQuirk). `quirkVar` picks the variant and strength inside a quirk;
    // the colours are whatever the quirk needs (water body, sky zenith/horizon, neon pair,
    // phosphor, pigment, glow).
    signed char   quirk, quirk2;
    unsigned char quirkVar, quirkVar2;
    unsigned char quirkColA[3], quirkColB[3];
    unsigned char quirk2ColA[3], quirk2ColB[3];
};

inline bool lmHasQuirk(const LimenLevelDNA& d, int q) { return q != LQ_NONE && (d.quirk == q || d.quirk2 == q); }
inline int lmQuirkVarOf(const LimenLevelDNA& d, int q) { return d.quirk == q ? d.quirkVar : (d.quirk2 == q ? d.quirkVar2 : 0); }
inline const unsigned char* lmQuirkColAOf(const LimenLevelDNA& d, int q) { return d.quirk == q ? d.quirkColA : d.quirk2ColA; }
inline const unsigned char* lmQuirkColBOf(const LimenLevelDNA& d, int q) { return d.quirk == q ? d.quirkColB : d.quirk2ColB; }

// Variants inside a quirk (all read from quirkVar, so the renderer, the props and the sound agree).
enum { LM_SKY_OVERCAST = 0, LM_SKY_CLOUDS, LM_SKY_NIGHT, LM_SKY_DUSK, LM_SKY_UPPER, LM_SKY_KINDS };
enum { LM_WARP_BOWL = 0, LM_WARP_GLOBE, LM_WARP_LEAN, LM_WARP_DUTCH, LM_WARP_KINDS };
enum { LM_GIANT_BIG = 0, LM_GIANT_DOLL, LM_GIANT_ALICE, LM_GIANT_KINDS };
enum { LM_LAVA_MAGMA = 0, LM_LAVA_TOXIC, LM_LAVA_PANEL, LM_LAVA_KINDS };
inline int lmSkyKind(const LimenLevelDNA& d)   { return lmQuirkVarOf(d, LQ_SKY) % LM_SKY_KINDS; }
inline int lmWarpKind(const LimenLevelDNA& d)  { return lmQuirkVarOf(d, LQ_WARP) % LM_WARP_KINDS; }
inline int lmGiantKind(const LimenLevelDNA& d) { return lmQuirkVarOf(d, LQ_GIANT) % LM_GIANT_KINDS; }
inline int lmLavaKind(const LimenLevelDNA& d)  { return lmQuirkVarOf(d, LQ_LAVA) % LM_LAVA_KINDS; }
inline bool lmSkyIsDay(const LimenLevelDNA& d) { int k = lmSkyKind(d); return k == LM_SKY_OVERCAST || k == LM_SKY_CLOUDS; }
// Water surface height above the floor datum, metres: ankle (0.10) ... knee (0.52).
inline float lmFloodLevelY(const LimenLevelDNA& d) {
    return lmHasQuirk(d, LQ_FLOOD) ? 0.10f + (float)(lmQuirkVarOf(d, LQ_FLOOD) >> 2) * 0.42f / 63.0f : -1000.0f;
}

inline const char* lmGeneName(int k) {
    static const char* names[LG_COUNT] = {
        "height", "mass", "grain", "order", "cells", "lattice", "sink", "abyss",
        "shell", "span", "edge", "decay", "graft", "light", "hue", "use"
    };
    return (k >= 0 && k < LG_COUNT) ? names[k] : "?";
}

inline const char* lmThemeShortName(int t) {
    static const char* names[LM_THEME_COUNT] = {
        "office", "parking", "maint", "warehs", "hotel", "pool", "hospit",
        "school", "mall", "playrm", "station", "lab", "kitchen"
    };
    return (t >= 0 && t < LM_THEME_COUNT) ? names[t] : "?";
}

inline int lmClampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int lmClamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

// Continuous everyday architectural pigment curve: maps [0..1000) across 360 deg while
// strongly favoring real institutional/interior materials (faded wallpaper cream/ochre,
// particleboard/wood/linoleum browns, slate-blue & sage-green oil paint, aged olive carpet,
// dusty rose/maroon tile) and keeping violet/magenta strictly muted.
inline int lmArchHueFromPermille(int pm) {
    pm = ((pm % 1000) + 1000) % 1000;
    if (pm < 290) return 34 + (pm * 22) / 290;                   // 34..56  buff plaster, cream, yellowed wallpaper
    if (pm < 485) return 16 + ((pm - 290) * 18) / 195;           // 16..34  particleboard, wood veneer, cork, tan linoleum
    if (pm < 660) return 192 + ((pm - 485) * 32) / 175;          // 192..224 institutional slate-blue, cool tile, grey-blue
    if (pm < 820) return 140 + ((pm - 660) * 52) / 160;          // 140..192 clinic sage, seafoam oil paint, oxidized green
    if (pm < 915) return 60 + ((pm - 820) * 80) / 95;            // 60..140  aged olive carpet, institutional drab
    return (338 + ((pm - 915) * 38) / 85) % 360;                 // 338..16  dusty rose ceramic, faded maroon/terracotta
}

// Heavy-tailed saturation curve (0..255 HSV S):
//   ~84% dusty / faded / muted everyday materials (S ~ 0.06..0.34)
//   ~13.5% moderately saturated oil paint / carpet / tile (S ~ 0.34..0.50)
//   ~2.5% rare vivid single-surface accent (S ~ 0.50..0.68)
inline int lmHeavyTailSat255(u32 roll) {
    int u = (int)(roll & 0xFFFFu);
    if (((roll >> 16) % 100u) < 10u) return 14 + (int)((roll >> 24) % 16u); // 0.05..0.11 concrete/plaster
    if (u < 55050) {
        long long t = (long long)u * 1000LL / 55050LL;
        long long shaped = (t * 500LL + (t * t * 500LL) / 1000LL) / 1000LL;
        return 21 + (int)((shaped * 65LL) / 1000LL);   // 21..86  (S = 0.08..0.34)
    }
    if (u < 63900) {
        long long t = (long long)(u - 55050) * 1000LL / 8850LL;
        long long shaped = (t * 420LL + (t * t * 580LL) / 1000LL) / 1000LL;
        return 86 + (int)((shaped * 42LL) / 1000LL);   // 86..128 (S = 0.34..0.50)
    }
    long long t = (long long)(u - 63900) * 1000LL / 1636LL;
    return 128 + (int)((t * t * 46LL) / 1000000LL);    // 128..174 (S = 0.50..0.68)
}

inline void lmHueSatValToRGB(int hue, int sat, int val, unsigned char out[3]) {
    hue = ((hue % 360) + 360) % 360;
    if (hue >= 245 && hue <= 334 && sat > 58) sat = 58; // never emit debug-magenta chroma
    sat = lmClamp255(sat);
    val = lmClamp255(val);
    int sec = hue / 60;
    int rem = hue % 60;
    int p = (val * (255 - sat)) / 255;
    int q = (val * (255 - (sat * rem) / 60)) / 255;
    int t = (val * (255 - (sat * (60 - rem)) / 60)) / 255;
    int r = val, g = val, b = val;
    if (sec == 0)      { r = val; g = t;   b = p;   }
    else if (sec == 1) { r = q;   g = val; b = p;   }
    else if (sec == 2) { r = p;   g = val; b = t;   }
    else if (sec == 3) { r = p;   g = q;   b = val; }
    else if (sec == 4) { r = t;   g = p;   b = val; }
    else               { r = val; g = p;   b = q;   }
    out[0] = (unsigned char)lmClamp255(r);
    out[1] = (unsigned char)lmClamp255(g);
    out[2] = (unsigned char)lmClamp255(b);
}

inline void lmRGBToHSV01(const unsigned char rgb[3], float& outH, float& outS, float& outV) {
    float r = (float)rgb[0] / 255.0f, g = (float)rgb[1] / 255.0f, b = (float)rgb[2] / 255.0f;
    float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    float d = mx - mn;
    outV = mx;
    outS = (mx > 0.0001f) ? (d / mx) : 0.0f;
    if (d < 0.0001f) { outH = 0.0f; return; }
    if (mx == r)      outH = 60.0f * ((g - b) / d + (g < b ? 6.0f : 0.0f));
    else if (mx == g) outH = 60.0f * ((b - r) / d + 2.0f);
    else              outH = 60.0f * ((r - g) / d + 4.0f);
}

inline LmSurfDNA lmBuildSurfFromHSV(u32 seed, int level, u32 chBase, int hueDeg, int sat255, int val255,
                                    int orderBias, int decayBias, int classicBlend, int cR, int cG, int cB) {
    u32 h1 = lmLvlHash(seed, level, chBase + 1u);
    u32 h2 = lmLvlHash(seed, level, chBase + 2u);
    LmSurfDNA s{};
    lmHueSatValToRGB(hueDeg, sat255, val255, s.rgb);
    if (classicBlend > 0) {
        s.rgb[0] = (unsigned char)((s.rgb[0] * (100 - classicBlend) + cR * classicBlend) / 100);
        s.rgb[1] = (unsigned char)((s.rgb[1] * (100 - classicBlend) + cG * classicBlend) / 100);
        s.rgb[2] = (unsigned char)((s.rgb[2] * (100 - classicBlend) + cB * classicBlend) / 100);
    }
    s.seamFreqX = (unsigned char)lmClamp255((int)(h1 & 255u));
    s.seamFreqY = (unsigned char)lmClamp255(((h1 >> 24) & 1u) ? s.seamFreqX : (int)((h1 >> 8) & 255u));
    s.seamDepth = (unsigned char)lmClamp255((int)((h1 >> 16) & 255u) + orderBias / 3);
    s.stripe    = (unsigned char)lmClamp255((int)(h2 & 255u) + orderBias / 4);
    s.grain     = (unsigned char)lmClamp255(28 + (int)((h2 >> 8) % 190u));
    s.grime     = (unsigned char)lmClamp255(24 + (int)((h2 >> 16) % 145u) + decayBias * 2 / 5);
    s.sheen     = (unsigned char)lmClamp255((int)((h2 >> 24) & 255u));
    return s;
}

// Levels are dealt in blocks of `n`: block b is a seeded shuffle of 0..n-1, patched so that it
// does not open with the option the previous block closed on. Hence any two consecutive
// levels differ and every block shows all n options. `classic` pins level 0 (and keeps it
// away from level -1). Needs 4 <= n <= 16.
inline void lmLvlDealBlock(u32 seed, int block, u32 ch, int n, int classic, int* perm) {
    for (int k = 0; k < n; k++) perm[k] = k;
    for (int k = n - 1; k > 0; k--) {
        int sw = (int)(lmLvlHash(seed, block, ch + (u32)k * 7919u) % (u32)(k + 1));
        int tmp = perm[k]; perm[k] = perm[sw]; perm[sw] = tmp;
    }
    if (classic < 0) return;
    if (block == 0) {
        for (int k = 1; k < n; k++) if (perm[k] == classic) { perm[k] = perm[0]; perm[0] = classic; }
    } else if (block == -1 && perm[n - 1] == classic) {
        perm[n - 1] = perm[n - 2]; perm[n - 2] = classic;
    }
}

inline int lmLvlPick(u32 seed, int level, u32 ch, int n, int classic = -1) {
    int b = lmFloorDiv(level, n), k = lmMod(level, n);
    int perm[16] = {0}, prev[16] = {0};
    lmLvlDealBlock(seed, b, ch, n, classic, perm);
    if (!(b == 0 && classic >= 0)) {
        lmLvlDealBlock(seed, b - 1, ch, n, classic, prev);
        if (perm[0] == prev[n - 1]) { int tmp = perm[0]; perm[0] = perm[1]; perm[1] = tmp; }
    }
    return perm[k];
}

struct LmPoleRng {
    u32 seed; int level; u32 ch;
    int operator()(int lo, int hi) {
        return lo + (int)(lmLvlHash(seed, level, ch++) % (u32)(hi - lo + 1));
    }
};

inline unsigned char lmSeamFreqForRepeats(int reps) { return (unsigned char)((lmClampInt(reps, 1, 16) - 1) * 17); }

inline void lmSurfSecondPigment(LmSurfDNA& s, int hue, int sat, int val) {
    lmHueSatValToRGB(hue, lmClampInt(sat, 0, 200), lmClampInt(val, 28, 240), s.rgb2);
}

// Moves a freshly hashed surface to its material pole. (hue, sat, val) is the pigment the
// surface was built with; the second pigment is derived from it.
inline void lmApplyWallPole(LmSurfDNA& s, int pole, u32 seed, int level, int hue, int sat, int val) {
    LmPoleRng R{seed, level, 940u};
    s.motifKind = (unsigned char)R(0, 255);
    int tone = R(0, 2) ? -R(20, 40) : R(14, 26);
    lmSurfSecondPigment(s, hue, sat, val + tone);
    switch (pole) {
    case 0: // painted plaster: nothing but the paint
        s.seamDepth = (unsigned char)R(0, 18); s.stripe = (unsigned char)R(0, 70);
        s.grain = (unsigned char)R(40, 110);   s.sheen = (unsigned char)R(30, 170);
        break;
    case 1: // striped wallpaper
        s.seamDepth = (unsigned char)R(0, 14); s.stripe = (unsigned char)R(195, 255);
        s.grain = (unsigned char)R(30, 85);    s.sheen = (unsigned char)R(8, 60);
        lmSurfSecondPigment(s, hue, sat + R(0, 18), val + tone);
        break;
    case 2: // ornament wallpaper
        s.seamDepth = (unsigned char)R(0, 14); s.stripe = (unsigned char)R(0, 80);
        s.motif = (unsigned char)R(160, 255);  s.grain = (unsigned char)R(30, 85);
        s.sheen = (unsigned char)R(8, 60);
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(R(3, 7));
        lmSurfSecondPigment(s, hue + (R(0, 3) ? 0 : R(-30, 30)), sat + R(4, 30), val + (tone < 0 ? tone - 8 : tone + 6));
        break;
    case 3: { // glazed tile, square or subway
        int r = R(7, 13);
        bool subway = R(0, 2) == 0;
        s.seamFreqX = lmSeamFreqForRepeats(subway ? r / 2 + 1 : r);
        s.seamFreqY = lmSeamFreqForRepeats(r);
        s.bond = subway ? 255 : 0;
        s.seamDepth = (unsigned char)R(150, 230); s.stripe = (unsigned char)R(0, 60);
        s.grain = (unsigned char)R(18, 60);       s.sheen = (unsigned char)R(175, 245);
        if (!subway && R(0, 3) == 0) s.checker = (unsigned char)R(70, 150);
        break;
    }
    case 4: // brick
        s.seamFreqX = lmSeamFreqForRepeats(R(5, 7)); s.seamFreqY = lmSeamFreqForRepeats(R(12, 16));
        s.bond = 255; s.plank = (unsigned char)R(60, 110);
        s.seamDepth = (unsigned char)R(175, 235); s.stripe = (unsigned char)R(0, 50);
        s.grain = (unsigned char)R(150, 220);     s.sheen = (unsigned char)R(10, 70);
        break;
    case 5: // formed concrete panels with tie holes
        s.seamFreqX = lmSeamFreqForRepeats(R(1, 2)); s.seamFreqY = lmSeamFreqForRepeats(R(1, 2));
        s.seamDepth = (unsigned char)R(115, 180); s.stripe = (unsigned char)R(0, 60);
        s.grain = (unsigned char)R(170, 230);     s.sheen = (unsigned char)R(10, 80);
        s.grime = (unsigned char)lmClamp255((int)s.grime + R(40, 90));
        break;
    case 6: // vertical board panelling
        s.seamFreqX = lmSeamFreqForRepeats(R(8, 12)); s.seamFreqY = 0;
        s.plank = (unsigned char)R(185, 255);
        s.seamDepth = (unsigned char)R(95, 150); s.stripe = (unsigned char)R(0, 50);
        s.grain = (unsigned char)R(60, 120);     s.sheen = (unsigned char)R(60, 140);
        break;
    default: // ribbed sheet metal
        s.seamFreqX = lmSeamFreqForRepeats(R(2, 3)); s.seamFreqY = 0;
        s.seamDepth = (unsigned char)R(100, 160); s.stripe = (unsigned char)R(200, 255);
        s.grain = (unsigned char)R(18, 50);       s.sheen = (unsigned char)R(180, 250);
        lmSurfSecondPigment(s, hue, sat, val - R(26, 44));
        break;
    }
}

// Returns the level's floorSeamStrength: 0 where world-space joints would cut a seamless floor.
inline int lmApplyFloorPole(LmSurfDNA& s, int pole, u32 seed, int level, int hue, int sat, int val) {
    LmPoleRng R{seed, level, 960u};
    s.motifKind = (unsigned char)R(0, 255);
    lmSurfSecondPigment(s, hue, sat, val - R(16, 30));
    switch (pole) {
    case 0: // carpet
        s.seamDepth = 0; s.stripe = (unsigned char)R(0, 60);
        s.grain = (unsigned char)R(190, 240); s.speckle = (unsigned char)R(40, 110);
        s.sheen = (unsigned char)R(0, 30);
        return 0;
    case 1: // patterned carpet
        s.seamDepth = 0; s.stripe = (unsigned char)R(0, 60);
        s.grain = (unsigned char)R(170, 230); s.motif = (unsigned char)R(150, 235);
        s.sheen = (unsigned char)R(0, 30);
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(R(3, 6));
        lmSurfSecondPigment(s, hue + (R(0, 2) ? 0 : R(-40, 40)), sat + R(8, 36), val + (R(0, 1) ? -R(26, 44) : R(22, 40)));
        return 0;
    case 2: // two-tone checker tile
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(2 * R(3, 5));
        s.checker = (unsigned char)R(160, 255);
        s.seamDepth = (unsigned char)R(80, 140); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(20, 70);      s.sheen = (unsigned char)R(170, 240);
        lmSurfSecondPigment(s, hue, sat / 3, val < 118 ? val + R(60, 85) : val - R(55, 80));
        return 0;
    case 3: // small mosaic tile
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(2 * R(7, 8));
        s.checker = (unsigned char)R(0, 70);
        s.seamDepth = (unsigned char)R(160, 220); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(20, 70);       s.sheen = (unsigned char)R(140, 220);
        return R(60, 130);
    case 4: // boards
        s.seamFreqX = lmSeamFreqForRepeats(R(2, 3)); s.seamFreqY = lmSeamFreqForRepeats(R(13, 16));
        s.bond = 255; s.plank = (unsigned char)R(190, 255);
        s.seamDepth = (unsigned char)R(70, 120); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(60, 120);     s.sheen = (unsigned char)R(80, 160);
        return 0;
    case 5: // bare concrete, 5 m pour joints
        s.seamFreqX = s.seamFreqY = 0;
        s.seamDepth = (unsigned char)R(30, 70); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(170, 230);   s.sheen = (unsigned char)R(20, 110);
        s.grime = (unsigned char)lmClamp255((int)s.grime + R(50, 100));
        return R(175, 250);
    case 6: // terrazzo
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(R(1, 2));
        s.speckle = (unsigned char)R(175, 255);
        s.seamDepth = (unsigned char)R(60, 110); s.stripe = (unsigned char)R(0, 40);
        s.grain = (unsigned char)R(30, 80);      s.sheen = (unsigned char)R(170, 240);
        lmSurfSecondPigment(s, hue + R(-50, 50), sat / 2 + R(0, 30), val + R(40, 70));
        return R(90, 160);
    default: // linoleum tile
        s.seamFreqX = s.seamFreqY = lmSeamFreqForRepeats(R(5, 8));
        s.checker = (unsigned char)R(0, 40);
        s.seamDepth = (unsigned char)R(60, 120); s.stripe = (unsigned char)R(0, 70);
        s.grain = (unsigned char)R(50, 100);     s.sheen = (unsigned char)R(90, 170);
        return R(60, 120);
    }
}
