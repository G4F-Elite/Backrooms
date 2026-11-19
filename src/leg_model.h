#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include "leg_render.h"

// ============================================================================
// BODY MODEL
// The quiet look of the walker: plain trousers, plain shoes, nothing else. Every
// part is a loft (rings of points joined into a smooth skin), built once from the
// body dimensions, in metres. No texture detail and no bright colours, so the
// legs sit in the picture without pulling the eye.
//
// The debug look (blue and red prisms, drawBody in leg_render.h) is still there
// for reading the gait; the demo switches between the two.
// ============================================================================

inline constexpr int MODEL_SIDES = 20;
inline constexpr int MODEL_LEG_RINGS = 30;

typedef std::vector<Vec3> ModelRing;

// One section of a loft: where it is along the axis and up to three sizes.
struct ModelKey {
    float t, a, b, c;
};

struct BodyModel {
    BodyMesh shoe, toe, pelvis, torso, ball, skin;
    GLuint tex = 0;
    float scale = 1.0f, torsoLen = 1.0f, ankleR = 0.0f, headR = 0.0f;
    float hipSpan = 0.0f;              // half width of the seat of the trousers
    std::vector<ModelKey> legProfile;  // t = distance from the hip joint, a/b = radius to the side / to the front
    std::vector<float> scratch;
};

// Colours. The lamps hang straight overhead and leave upright faces dim, so these are
// brighter than they end up on screen.
inline const Vec3 MODEL_TROUSERS(0.42f, 0.44f, 0.50f);
inline const Vec3 MODEL_SHOE(0.30f, 0.28f, 0.27f);
inline const Vec3 MODEL_TOP(0.74f, 0.70f, 0.62f);
inline const Vec3 MODEL_HEAD(0.92f, 0.84f, 0.76f);

// The engine's main shader is written for walls: light falls off to nothing at a right
// angle to the lamp, and anything facing down is lit as a ceiling. On a round body under
// overhead lamps that gives black sides and hard bands. The body gets the same shader
// (same lamps, fog and grading) with those two lines swapped for a soft wrap-around falloff.
inline std::string modelShaderSource(const char* mainSource) {
    std::string src = mainSource;
    const char* swaps[2][2] = {
        {"bool isCeil = N.y < -0.5;", "bool isCeil = false;"},
        {"df = smoothstep(0.0, 0.5, NdotL);", "float wrap = dot(N, L) * 0.5 + 0.5; df = wrap * wrap;"},
    };
    for (const auto& swap : swaps) {
        size_t at = src.find(swap[0]);
        if (at != std::string::npos) src.replace(at, strlen(swap[0]), swap[1]);
    }
    return src;
}

inline Vec3 modelRingCentre(const ModelRing& r) {
    Vec3 c;
    for (const Vec3& p : r) c += p;
    return c / (float)r.size();
}

// A few hand-placed sections turned into a smooth run of them (Catmull-Rom through the keys).
inline std::vector<ModelKey> modelSmooth(const std::vector<ModelKey>& keys, int steps) {
    std::vector<ModelKey> out;
    int n = (int)keys.size();
    auto spline = [](float p0, float p1, float p2, float p3, float u) {
        return 0.5f * (2.0f * p1 + (p2 - p0) * u + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * u * u + (3.0f * p1 - p0 - 3.0f * p2 + p3) * u * u * u);
    };
    for (int i = 0; i + 1 < n; i++) {
        const ModelKey& k0 = keys[i > 0 ? i - 1 : 0];
        const ModelKey& k1 = keys[i];
        const ModelKey& k2 = keys[i + 1];
        const ModelKey& k3 = keys[i + 2 < n ? i + 2 : n - 1];
        for (int j = 0; j < steps; j++) {
            float u = (float)j / steps;
            ModelKey k;
            k.t = k1.t + (k2.t - k1.t) * u;
            k.a = spline(k0.a, k1.a, k2.a, k3.a, u);
            k.b = spline(k0.b, k1.b, k2.b, k3.b, u);
            k.c = spline(k0.c, k1.c, k2.c, k3.c, u);
            out.push_back(k);
        }
    }
    out.push_back(keys[n - 1]);
    return out;
}

// Sizes of a smoothed profile at t.
inline ModelKey modelProfileAt(const std::vector<ModelKey>& p, float t) {
    int n = (int)p.size();
    if (t <= p[0].t) return p[0];
    for (int i = 0; i + 1 < n; i++) {
        if (t > p[i + 1].t) continue;
        float u = (t - p[i].t) / (p[i + 1].t - p[i].t);
        ModelKey k = {t, p[i].a + (p[i + 1].a - p[i].a) * u, p[i].b + (p[i + 1].b - p[i].b) * u, p[i].c + (p[i + 1].c - p[i].c) * u};
        return k;
    }
    return p[n - 1];
}

// The seat of the trousers and the tops of both legs are separate surfaces that pass
// through each other. Lit by their own normals they show a seam wherever they cross, so
// around the hips both take their normal from one shared field instead: that of a wide
// ellipsoid about the pelvis. `p` and the result are in pelvis axes (right, up, forward).
inline Vec3 modelHipNormal(const BodyModel& m, const Vec3& p) {
    float k = m.scale;
    float y = p.y - 0.03f * k;
    float tall = (y > 0.0f ? 0.20f : 0.36f) * k, deep = 0.10f * k;
    return Vec3(p.x / (m.hipSpan * m.hipSpan), y / (tall * tall), p.z / (deep * deep)).norm();
}

// Ellipse across the Y axis: rx to the side, rz to the front.
inline ModelRing modelRingY(float y, float rx, float rz) {
    ModelRing r;
    for (int i = 0; i < MODEL_SIDES; i++) {
        float a = MATH_TAU * i / MODEL_SIDES;
        r.push_back(Vec3(rx * cosf(a), y, rz * sinf(a)));
    }
    return r;
}

// Shoe section across the Z axis: flat sole on y = 0 with a chamfered edge, round upper.
inline ModelRing modelRingShoe(float z, float halfW, float top, float edge) {
    ModelRing r;
    for (int i = 0; i < MODEL_SIDES; i++) {
        float a = MATH_TAU * i / MODEL_SIDES;
        float s = sinf(a);
        float y = s >= 0.0f ? edge + (top - edge) * s : edge * (1.0f - gSat(-s * 2.5f));
        r.push_back(Vec3(halfW * cosf(a), y, z));
    }
    return r;
}

inline void modelPushTri(std::vector<float>& v, const Vec3& a, const Vec3& na, const Vec3& b, const Vec3& nb, const Vec3& c, const Vec3& nc) {
    bodyPushVert(v, a, 0.5f, 0.5f, na);
    bodyPushVert(v, b, 0.5f, 0.5f, nb);
    bodyPushVert(v, c, 0.5f, 0.5f, nc);
}

// Skin over the rings with smooth normals, closed with a flat cap at both ends.
inline void modelLoft(std::vector<float>& v, const std::vector<ModelRing>& rings) {
    int K = (int)rings.size(), N = MODEL_SIDES;
    std::vector<Vec3> centre(K);
    std::vector<ModelRing> normal(K);
    for (int k = 0; k < K; k++) centre[k] = modelRingCentre(rings[k]);
    for (int k = 0; k < K; k++) {
        int k0 = k > 0 ? k - 1 : k, k1 = k + 1 < K ? k + 1 : k;
        for (int i = 0; i < N; i++) {
            Vec3 around = rings[k][(i + 1) % N] - rings[k][(i + N - 1) % N];
            Vec3 along = rings[k1][i] - rings[k0][i];
            Vec3 n = around.cross(along).norm();
            if (n.dot(rings[k][i] - centre[k]) < 0.0f) n = -n;
            normal[k].push_back(n);
        }
    }
    for (int k = 0; k + 1 < K; k++) {
        for (int i = 0; i < N; i++) {
            int j = (i + 1) % N;
            modelPushTri(v, rings[k][i], normal[k][i], rings[k][j], normal[k][j], rings[k + 1][j], normal[k + 1][j]);
            modelPushTri(v, rings[k][i], normal[k][i], rings[k + 1][j], normal[k + 1][j], rings[k + 1][i], normal[k + 1][i]);
        }
    }
    for (int end = 0; end < 2; end++) {
        int k = end == 0 ? 0 : K - 1;
        Vec3 n = (end == 0 ? centre[0] - centre[1] : centre[K - 1] - centre[K - 2]).norm();
        for (int i = 0; i < N; i++) modelPushTri(v, centre[k], n, rings[k][i], n, rings[k][(i + 1) % N], n);
    }
}

// Loft along the Y axis from smoothed keys: a to the side, b to the front.
inline void modelLoftY(std::vector<float>& v, const std::vector<ModelKey>& keys, int steps) {
    std::vector<ModelRing> rings;
    for (const ModelKey& k : modelSmooth(keys, steps)) rings.push_back(modelRingY(k.t, k.a, k.b));
    modelLoft(v, rings);
}

// Loft along the Z axis from smoothed keys: a half the width, b the height, c the sole chamfer.
inline void modelLoftShoe(std::vector<float>& v, const std::vector<ModelKey>& keys, int steps) {
    std::vector<ModelRing> rings;
    for (const ModelKey& k : modelSmooth(keys, steps)) rings.push_back(modelRingShoe(k.t, k.a, k.b, k.c));
    modelLoft(v, rings);
}

inline void modelInit(BodyModel& m, const BodyDims& b) {
    float k = b.stature / 1.75f;
    m.scale = k;
    m.torsoLen = 0.50f * k;
    m.ankleR = 0.044f * k;
    m.headR = 0.105f * k;
    m.hipSpan = b.hipHalf + 0.084f * k;
    std::vector<float> v;

    // Trouser leg, hip to hem. It stops short of the ankle: the hem hangs over the shoe.
    float knee = b.thigh, hem = b.thigh + b.shank * 0.94f;
    m.legProfile = modelSmooth({
        {0.0f, 0.080f * k, 0.088f * k, 0},
        {knee * 0.30f, 0.078f * k, 0.086f * k, 0},
        {knee * 0.65f, 0.067f * k, 0.072f * k, 0},
        {knee, 0.058f * k, 0.062f * k, 0},
        {knee + b.shank * 0.30f, 0.055f * k, 0.060f * k, 0},
        {knee + b.shank * 0.65f, 0.051f * k, 0.055f * k, 0},
        {hem, 0.050f * k, 0.053f * k, 0},
    }, 4);

    // Seat of the trousers. Origin between the hip joints, X to the right, Z forward.
    // Below the hip joints it stays between the thighs, so the outline there is theirs.
    float hw = m.hipSpan;
    modelLoftY(v, {
        {-0.070f * k, b.hipHalf * 0.60f, 0.050f * k, 0},
        {-0.035f * k, b.hipHalf * 1.25f, 0.078f * k, 0},
        {0.005f * k, hw * 0.95f, 0.090f * k, 0},
        {0.050f * k, hw, 0.094f * k, 0},
        {0.110f * k, hw * 0.96f, 0.093f * k, 0},
        {0.145f * k, hw * 0.92f, 0.090f * k, 0},
        {0.156f * k, hw * 0.84f, 0.082f * k, 0},
        {0.160f * k, hw * 0.55f, 0.054f * k, 0},
    }, 3);
    for (size_t i = 0; i + 7 < v.size(); i += 8) {
        Vec3 n = modelHipNormal(m, Vec3(v[i], v[i + 1], v[i + 2]));
        v[i + 5] = n.x;
        v[i + 6] = n.y;
        v[i + 7] = n.z;
    }
    bodyUpload(m.pelvis, v);

    // Trunk, waist (y = 0) to neck. No arms: the simulation has none.
    v.clear();
    float tl = m.torsoLen;
    modelLoftY(v, {
        {0.0f, 0.122f * k, 0.074f * k, 0},
        {tl * 0.30f, 0.146f * k, 0.094f * k, 0},
        {tl * 0.66f, 0.168f * k, 0.104f * k, 0},
        {tl * 0.86f, 0.166f * k, 0.098f * k, 0},
        {tl * 0.95f, 0.130f * k, 0.080f * k, 0},
        {tl, 0.062f * k, 0.054f * k, 0},
    }, 4);
    bodyUpload(m.torso, v);

    // Shoe from the heel to the ball of the foot. Origin on the sole under the ankle, Z forward.
    v.clear();
    float hb = b.heelBack, bf = b.ballFwd, w = b.footHalfW, h = b.ankleH, e = 0.012f * k;
    modelLoftShoe(v, {
        {-hb - 0.014f * k, w * 0.30f, h * 0.44f, e * 1.6f},
        {-hb - 0.007f * k, w * 0.66f, h * 0.84f, e},
        {-hb * 0.55f, w * 0.90f, h * 1.04f, e},
        {0.0f, w * 0.97f, h * 1.06f, e},
        {bf * 0.38f, w * 1.00f, h * 0.90f, e},
        {bf * 0.72f, w * 1.04f, h * 0.68f, e},
        {bf, w * 1.05f, h * 0.56f, e},
    }, 3);
    bodyUpload(m.shoe, v);

    // Toe cap, hinged at the ball. Origin on the sole under the ball, Z forward.
    v.clear();
    float toe = b.toeFwd - b.ballFwd;
    modelLoftShoe(v, {
        {0.0f, w * 1.05f, h * 0.56f, e},
        {toe * 0.45f, w * 1.00f, h * 0.52f, e},
        {toe * 0.80f, w * 0.82f, h * 0.44f, e},
        {toe * 0.95f, w * 0.56f, h * 0.34f, e},
        {toe, w * 0.26f, h * 0.24f, e * 1.4f},
    }, 3);
    bodyUpload(m.toe, v);

    // Unit ball, for the ankle and the head.
    v.clear();
    const int LAT = 14;
    std::vector<ModelRing> rings;
    for (int j = 0; j < LAT; j++) {
        float t = MATH_PI * (j + 0.5f) / LAT;
        rings.push_back(modelRingY(cosf(t), sinf(t), sinf(t)));
    }
    modelLoft(v, rings);
    bodyUpload(m.ball, v);

    // The legs are skinned anew every frame; this only sets the buffer up.
    v.assign(8, 0.0f);
    bodyUpload(m.skin, v);

    // One flat texel: the colour comes from the tint alone.
    const unsigned char white[4] = {255, 255, 255, 128};
    glGenTextures(1, &m.tex);
    glBindTexture(GL_TEXTURE_2D, m.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

// Pelvis axes in the world, to place the seat and to evaluate modelHipNormal for the legs.
struct ModelPelvis {
    Vec3 pos, right, up, fwd;
};

// Centre line of a leg at distance s from the hip: straight down the thigh and the shank,
// rounded through the knee so the trouser leg bends instead of breaking.
inline void modelLegCentre(const LegPose& leg, float thighLen, float bend, float s, Vec3& c, Vec3& tangent) {
    Vec3 t1 = (leg.knee - leg.hip).norm(), t2 = (leg.ankle - leg.knee).norm();
    if (s <= thighLen - bend) {
        c = leg.hip + t1 * s;
        tangent = t1;
    } else if (s >= thighLen + bend) {
        c = leg.knee + t2 * (s - thighLen);
        tangent = t2;
    } else {
        float u = (s - (thighLen - bend)) / (2.0f * bend);
        Vec3 p0 = leg.knee - t1 * bend, p2 = leg.knee + t2 * bend;
        c = p0 * ((1 - u) * (1 - u)) + leg.knee * (2 * u * (1 - u)) + p2 * (u * u);
        tangent = (t1 * (1 - u) + t2 * u).norm();
    }
}

// One trouser leg as a single skin from the hip to the hem, in world space.
inline void modelSkinLeg(std::vector<float>& v, const BodyModel& m, const ModelPelvis& pv, const LegPose& leg, const Vec3& front) {
    const int N = MODEL_SIDES, DOME = 4, R = DOME + MODEL_LEG_RINGS;
    float k = m.scale;
    float thighLen = (leg.knee - leg.hip).len();
    float shankLen = (leg.ankle - leg.knee).len();
    float end = thighLen + shankLen * 0.94f, profileEnd = m.legProfile.back().t;
    static std::vector<Vec3> pos, nrm;
    pos.resize(R * N);
    nrm.resize(R * N);

    for (int r = 0; r < R; r++) {
        // The first rings close the top of the thigh with a dome around the hip joint.
        float s = 0.0f, shrink = 1.0f, back = 0.0f;
        if (r < DOME) {
            float a = MATH_HALF_PI * (DOME - r) / (DOME + 0.5f);
            shrink = cosf(a);
            back = sinf(a);
        } else {
            s = end * (r - DOME) / (MODEL_LEG_RINGS - 1);
        }
        Vec3 c, t;
        modelLegCentre(leg, thighLen, 0.085f * k, s, c, t);
        ModelKey size = modelProfileAt(m.legProfile, s * profileEnd / end);
        c = c - t * (back * size.a);
        Vec3 x = t.cross(front).norm();
        Vec3 z = x.cross(t);
        float hipBlend = 1.0f - gSat((s - 0.04f * k) / (0.20f * k));
        hipBlend = hipBlend * hipBlend * (3.0f - 2.0f * hipBlend);
        for (int i = 0; i < N; i++) {
            float a = MATH_TAU * i / N;
            Vec3 p = c + x * (size.a * shrink * cosf(a)) + z * (size.b * shrink * sinf(a));
            Vec3 n = (x * (size.b * cosf(a)) + z * (size.a * sinf(a))).norm();
            if (hipBlend > 0.0f) {
                Vec3 d = p - pv.pos;
                Vec3 h = modelHipNormal(m, Vec3(d.dot(pv.right), d.dot(pv.up), d.dot(pv.fwd)));
                Vec3 shared = pv.right * h.x + pv.up * h.y + pv.fwd * h.z;
                n = (n * (1.0f - hipBlend) + shared * hipBlend).norm();
            }
            pos[r * N + i] = p;
            nrm[r * N + i] = n;
        }
    }
    for (int r = 0; r + 1 < R; r++) {
        for (int i = 0; i < N; i++) {
            int a = r * N + i, b = r * N + (i + 1) % N, c = a + N, d = b + N;
            modelPushTri(v, pos[a], nrm[a], pos[b], nrm[b], pos[d], nrm[d]);
            modelPushTri(v, pos[a], nrm[a], pos[d], nrm[d], pos[c], nrm[c]);
        }
    }
    // Close both ends: the top inside the seat, the hem around the ankle.
    for (int cap = 0; cap < 2; cap++) {
        int r = cap == 0 ? 0 : R - 1;
        Vec3 centre, n;
        for (int i = 0; i < N; i++) {
            centre += pos[r * N + i] / (float)N;
            n += nrm[r * N + i];
        }
        n = cap == 0 ? n.norm() : (leg.ankle - leg.knee).norm();
        for (int i = 0; i < N; i++) {
            int a = r * N + i, b = r * N + (i + 1) % N;
            modelPushTri(v, centre, n, pos[a], cap == 0 ? nrm[a] : n, pos[b], cap == 0 ? nrm[b] : n);
        }
    }
}

struct ModelDraw {
    BodyModel* model;
    BodyDraw d;

    void ball(const Vec3& p, float r, const Vec3& tint) const {
        d.mesh(model->ball, bodyAxesMatrix(Vec3(r, 0, 0), Vec3(0, r, 0), Vec3(0, 0, r), p), tint);
    }
};

inline void modelDrawFoot(const ModelDraw& md, const LegSim& s, int i) {
    const BodyModel& m = *md.model;
    const BodyDims& b = s.dims;
    const LegFoot& f = s.foot[i];
    LegPose leg = s.legPose(i);
    md.ball(leg.ankle, m.ankleR, MODEL_SHOE);

    Vec3 fw, up, rt;
    footAxes(f.pose, fw, up, rt);
    Vec3 sole = leg.ankle - up * b.ankleH;
    md.d.mesh(m.shoe, bodyAxesMatrix(rt, up, fw, sole), MODEL_SHOE);

    // The toe cap turns about the ball of the foot, on the sole, so the sole never opens.
    float tp = footToePitch(b, f.pose);
    Vec3 h = headingFwd(f.pose.yaw);
    Vec3 tf(h.x * cosf(tp), sinf(tp), h.z * cosf(tp));
    Vec3 tu(-h.x * sinf(tp), cosf(tp), -h.z * sinf(tp));
    md.d.mesh(m.toe, bodyAxesMatrix(rt, tu, tf, sole + fw * b.ballFwd), MODEL_SHOE);
}

// withTrunk: also draw the trunk and the head (third person). In first person the body
// ends at the waist, so looking down shows the legs with nothing in the way.
inline void drawModel(BodyModel& m, GLint locM, GLint locTint, const LegSim& s, const HeadView& head, float lookYaw, float lookPitch, bool withTrunk) {
    ModelDraw md = {&m, {nullptr, locM, locTint}};
    glBindTexture(GL_TEXTURE_2D, m.tex);
    glDisable(GL_CULL_FACE);

    Vec3 pr = headingRight(s.pelvisYaw);
    float c = cosf(s.list), sn = sinf(s.list);
    ModelPelvis pv = {s.pelvisPos, pr * c - Vec3(0, sn, 0), pr * sn + Vec3(0, c, 0), headingFwd(s.pelvisYaw)};
    md.d.mesh(m.pelvis, bodyAxesMatrix(pv.right, pv.up, pv.fwd, pv.pos), MODEL_TROUSERS);

    m.scratch.clear();
    for (int i = 0; i < 2; i++) {
        LegPose leg = s.legPose(i);
        Vec3 kneeFront = (leg.knee - (leg.hip + leg.ankle) * 0.5f).norm() + s.kneePole(i) * 0.5f;
        modelSkinLeg(m.scratch, m, pv, leg, kneeFront);
    }
    m.skin.count = (int)m.scratch.size() / 8;
    glBindBuffer(GL_ARRAY_BUFFER, m.skin.vbo);
    glBufferData(GL_ARRAY_BUFFER, m.scratch.size() * sizeof(float), m.scratch.data(), GL_DYNAMIC_DRAW);
    md.d.mesh(m.skin, Mat4(), MODEL_TROUSERS);
    modelDrawFoot(md, s, 0);
    modelDrawFoot(md, s, 1);

    if (withTrunk) {
        // The shoulders counter-rotate the pelvis, the trunk stays upright over it.
        float trunkYaw = s.bodyYaw - s.rot * 0.6f;
        Vec3 waist = s.pelvisPos + Vec3(0, 0.10f, 0);
        Vec3 spine = Vec3(head.eye.x, head.eye.y - 0.10f, head.eye.z) - waist;
        Vec3 sy = spine.norm();
        Vec3 tf = headingFwd(trunkYaw);
        Vec3 sz = (tf - sy * tf.dot(sy)).norm();
        md.d.mesh(m.torso, bodyAxesMatrix(sy.cross(sz), spine / m.torsoLen, sz, waist), MODEL_TOP);
        Vec3 look(sinf(lookYaw) * cosf(lookPitch), sinf(lookPitch), cosf(lookYaw) * cosf(lookPitch));
        md.ball(head.eye - look * 0.07f, m.headR, MODEL_HEAD);
    }
    glEnable(GL_CULL_FACE);
    Mat4 identity;
    glUniformMatrix4fv(locM, 1, GL_FALSE, identity.m);
    glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
}
