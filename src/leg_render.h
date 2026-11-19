#pragma once
#include <vector>
#include <glad/glad.h>
#include "render.h"
#include "gl_extra.h"
#include "leg_view.h"

// ============================================================================
// PSEUDO BODY
// Stand-in shapes for the simulated skeleton: tapered prisms for thigh and shank,
// boxes for pelvis, foot and toes, balls on the joints. Left leg is tinted blue,
// right leg red, so the gait can be read at a glance. This is the debug look now;
// the model the demo normally draws is in leg_model.h.
// ============================================================================

struct BodyMesh {
    GLuint vao = 0, vbo = 0;
    int count = 0;
};

struct BodyGfx {
    BodyMesh box, thigh, shank, ball;
    GLuint tex = 0;
};

inline void bodyPushVert(std::vector<float>& v, const Vec3& p, float tu, float tv, const Vec3& n) {
    float d[8] = {p.x, p.y, p.z, tu, tv, n.x, n.y, n.z};
    v.insert(v.end(), d, d + 8);
}

inline void bodyPushQuad(std::vector<float>& v, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& n) {
    bodyPushVert(v, a, 0, 0, n);
    bodyPushVert(v, b, 1, 0, n);
    bodyPushVert(v, c, 1, 1, n);
    bodyPushVert(v, a, 0, 0, n);
    bodyPushVert(v, c, 1, 1, n);
    bodyPushVert(v, d, 0, 1, n);
}

// Unit cube centred on the origin.
inline void bodyBuildBox(std::vector<float>& v) {
    const float h = 0.5f;
    bodyPushQuad(v, Vec3(-h, -h, h), Vec3(h, -h, h), Vec3(h, h, h), Vec3(-h, h, h), Vec3(0, 0, 1));
    bodyPushQuad(v, Vec3(h, -h, -h), Vec3(-h, -h, -h), Vec3(-h, h, -h), Vec3(h, h, -h), Vec3(0, 0, -1));
    bodyPushQuad(v, Vec3(h, -h, h), Vec3(h, -h, -h), Vec3(h, h, -h), Vec3(h, h, h), Vec3(1, 0, 0));
    bodyPushQuad(v, Vec3(-h, -h, -h), Vec3(-h, -h, h), Vec3(-h, h, h), Vec3(-h, h, -h), Vec3(-1, 0, 0));
    bodyPushQuad(v, Vec3(-h, h, h), Vec3(h, h, h), Vec3(h, h, -h), Vec3(-h, h, -h), Vec3(0, 1, 0));
    bodyPushQuad(v, Vec3(-h, -h, -h), Vec3(h, -h, -h), Vec3(h, -h, h), Vec3(-h, -h, h), Vec3(0, -1, 0));
}

// Eight-sided prism along +Y from 0 to 1: radius 1 at the start, `taper` at the end,
// with a bulge in between (muscle belly).
inline void bodyBuildSegment(std::vector<float>& v, float taper, float bulge, float bulgeAt) {
    const int SIDES = 8, RINGS = 4;
    float y[RINGS] = {0.0f, bulgeAt, 0.5f + bulgeAt * 0.5f, 1.0f};
    float r[RINGS] = {1.0f, bulge, 0.5f * (bulge + taper), taper};
    for (int k = 0; k + 1 < RINGS; k++) {
        for (int i = 0; i < SIDES; i++) {
            float a0 = MATH_TAU * i / SIDES, a1 = MATH_TAU * (i + 1) / SIDES;
            Vec3 n0(cosf(a0), 0, sinf(a0)), n1(cosf(a1), 0, sinf(a1));
            Vec3 p00 = n0 * r[k] + Vec3(0, y[k], 0), p01 = n1 * r[k] + Vec3(0, y[k], 0);
            Vec3 p10 = n0 * r[k + 1] + Vec3(0, y[k + 1], 0), p11 = n1 * r[k + 1] + Vec3(0, y[k + 1], 0);
            Vec3 n = ((n0 + n1) * 0.5f).norm();
            bodyPushQuad(v, p00, p01, p11, p10, n);
        }
    }
}

inline void bodyBuildBall(std::vector<float>& v) {
    const int LON = 8, LAT = 5;
    for (int j = 0; j < LAT; j++) {
        float t0 = MATH_PI * j / LAT, t1 = MATH_PI * (j + 1) / LAT;
        for (int i = 0; i < LON; i++) {
            float a0 = MATH_TAU * i / LON, a1 = MATH_TAU * (i + 1) / LON;
            Vec3 p00(sinf(t0) * cosf(a0), cosf(t0), sinf(t0) * sinf(a0));
            Vec3 p01(sinf(t0) * cosf(a1), cosf(t0), sinf(t0) * sinf(a1));
            Vec3 p10(sinf(t1) * cosf(a0), cosf(t1), sinf(t1) * sinf(a0));
            Vec3 p11(sinf(t1) * cosf(a1), cosf(t1), sinf(t1) * sinf(a1));
            Vec3 n = (p00 + p01 + p10 + p11).norm();
            bodyPushQuad(v, p00, p01, p11, p10, n);
        }
    }
}

inline void bodyUpload(BodyMesh& m, std::vector<float>& v) {
    m.count = (int)v.size() / 8;
    setupVAO(m.vao, m.vbo, v, true);
}

// Plain light grey with a faint grid, so rotation of the shapes stays readable.
inline GLuint bodyMakeTexture() {
    const int N = 64;
    unsigned char* d = new unsigned char[N * N * 4];
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            bool line = (x % 16) == 0 || (y % 16) == 0;
            unsigned char c = line ? 170 : 225;
            unsigned char* p = d + (y * N + x) * 4;
            p[0] = p[1] = p[2] = c;
            p[3] = 128;  // flat height: no parallax on the body
        }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] d;
    return tex;
}

inline void bodyGfxInit(BodyGfx& g) {
    std::vector<float> v;
    bodyBuildBox(v);
    bodyUpload(g.box, v);
    v.clear();
    bodyBuildSegment(v, 0.70f, 1.06f, 0.30f);
    bodyUpload(g.thigh, v);
    v.clear();
    bodyBuildSegment(v, 0.62f, 1.18f, 0.28f);
    bodyUpload(g.shank, v);
    v.clear();
    bodyBuildBall(v);
    bodyUpload(g.ball, v);
    g.tex = bodyMakeTexture();
}

inline Mat4 bodyAxesMatrix(const Vec3& x, const Vec3& y, const Vec3& z, const Vec3& pos) {
    Mat4 m;
    m.m[0] = x.x; m.m[1] = x.y; m.m[2] = x.z;
    m.m[4] = y.x; m.m[5] = y.y; m.m[6] = y.z;
    m.m[8] = z.x; m.m[9] = z.y; m.m[10] = z.z;
    m.m[12] = pos.x; m.m[13] = pos.y; m.m[14] = pos.z;
    return m;
}

struct BodyDraw {
    const BodyGfx* gfx;
    GLint locM, locTint;

    void mesh(const BodyMesh& m, const Mat4& model, const Vec3& tint) const {
        glUniformMatrix4fv(locM, 1, GL_FALSE, model.m);
        glUniform3f(locTint, tint.x, tint.y, tint.z);
        glBindVertexArray(m.vao);
        glDrawArrays(GL_TRIANGLES, 0, m.count);
    }
    // Prism from a to b; `front` says which way its local +Z faces, w/d are the start radii.
    void bone(const BodyMesh& m, const Vec3& a, const Vec3& b, const Vec3& front, float w, float d, const Vec3& tint) const {
        Vec3 y = b - a;
        Vec3 yn = y.norm();
        Vec3 z = (front - yn * front.dot(yn)).norm();
        Vec3 x = yn.cross(z);
        mesh(m, bodyAxesMatrix(x * w, y, z * d, a), tint);
    }
    void block(const Vec3& centre, const Vec3& right, const Vec3& up, const Vec3& fwd, float w, float h, float l, const Vec3& tint) const {
        mesh(gfx->box, bodyAxesMatrix(right * w, up * h, fwd * l, centre), tint);
    }
    void joint(const Vec3& p, float r, const Vec3& tint) const {
        mesh(gfx->ball, bodyAxesMatrix(Vec3(r, 0, 0), Vec3(0, r, 0), Vec3(0, 0, r), p), tint);
    }
};

inline void drawLeg(const BodyDraw& d, const LegSim& s, int i) {
    const BodyDims& b = s.dims;
    const LegFoot& f = s.foot[i];
    Vec3 tint = i == 0 ? Vec3(0.85f, 1.25f, 1.90f) : Vec3(1.95f, 1.05f, 0.80f);
    Vec3 dark = tint * 0.55f;

    Vec3 pole = s.kneePole(i);
    LegPose leg = s.legPose(i);
    Vec3 kneeFront = (leg.knee - (leg.hip + leg.ankle) * 0.5f).norm() + pole * 0.5f;

    d.joint(leg.hip, 0.088f, dark);
    d.bone(d.gfx->thigh, leg.hip, leg.knee, kneeFront, 0.082f, 0.092f, tint);
    d.joint(leg.knee, 0.060f, dark);
    d.bone(d.gfx->shank, leg.knee, leg.ankle, kneeFront, 0.054f, 0.062f, tint);
    d.joint(leg.ankle, 0.043f, dark);

    // Foot: a block from heel to ball hanging under the ankle, toes hinged at the ball.
    Vec3 fw, up, rt;
    footAxes(f.pose, fw, up, rt);
    float soleLen = b.heelBack + b.ballFwd;
    float footH = b.ankleH * 0.85f;
    Vec3 mid = leg.ankle + fw * ((b.ballFwd - b.heelBack) * 0.5f) - up * (b.ankleH - footH * 0.5f);
    d.block(mid, rt, up, fw, b.footHalfW * 2.0f, footH, soleLen, tint * 0.85f);

    float tp = footToePitch(b, f.pose);
    Vec3 h = headingFwd(f.pose.yaw);
    Vec3 tf(h.x * cosf(tp), sinf(tp), h.z * cosf(tp));
    Vec3 tu(-h.x * sinf(tp), cosf(tp), -h.z * sinf(tp));
    float toeLen = b.toeFwd - b.ballFwd, toeH = footH * 0.55f;
    Vec3 ballPt = leg.ankle + fw * b.ballFwd - up * b.ankleH;
    d.block(ballPt + tf * (toeLen * 0.5f) + tu * (toeH * 0.5f), rt, tu, tf, b.footHalfW * 1.9f, toeH, toeLen, dark);
}

// withTrunk: also draw pelvis, torso and head (third person). In first person only the
// legs are drawn, so looking down shows them without a body in the way.
inline void drawBody(const BodyGfx& g, GLint locM, GLint locTint, const LegSim& s, const HeadView& head, float lookYaw, float lookPitch, bool withTrunk) {
    BodyDraw d = {&g, locM, locTint};
    glBindTexture(GL_TEXTURE_2D, g.tex);
    glDisable(GL_CULL_FACE);

    Vec3 pf = headingFwd(s.pelvisYaw), pr = headingRight(s.pelvisYaw);
    float c = cosf(s.list), sn = sinf(s.list);
    Vec3 right = pr * c - Vec3(0, sn, 0);
    Vec3 up = pr * sn + Vec3(0, c, 0);
    // Tints above 1 on purpose: the lamps hang straight overhead and leave upright faces dim.
    Vec3 grey(1.45f, 1.45f, 1.40f);
    if (withTrunk) d.block(s.pelvisPos + up * 0.055f, right, up, pf, s.dims.hipHalf * 2.0f + 0.13f, 0.17f, 0.20f, grey);
    drawLeg(d, s, 0);
    drawLeg(d, s, 1);

    if (withTrunk) {
        // The shoulders counter-rotate the pelvis, the trunk stays upright over it.
        float trunkYaw = s.bodyYaw - s.rot * 0.6f;
        Vec3 tf = headingFwd(trunkYaw), tr = headingRight(trunkYaw);
        Vec3 waist = s.pelvisPos + Vec3(0, 0.14f, 0);
        Vec3 neck = Vec3(head.eye.x, head.eye.y - 0.17f, head.eye.z);
        Vec3 spine = neck - waist;
        Vec3 sy = spine.norm();
        Vec3 sz = (tf - sy * tf.dot(sy)).norm();
        d.block(waist + spine * 0.5f, tr, sy, sz, 0.37f, spine.len(), 0.21f, grey * 0.92f);
        Vec3 look(sinf(lookYaw) * cosf(lookPitch), sinf(lookPitch), cosf(lookYaw) * cosf(lookPitch));
        Vec3 skull = head.eye - look * 0.07f;
        d.joint(skull, 0.105f, grey);
        d.block(skull + look * 0.11f, headingRight(lookYaw), Vec3(0, 1, 0), look, 0.03f, 0.03f, 0.06f, grey * 0.5f);
    }
    glEnable(GL_CULL_FACE);
    Mat4 identity;
    glUniformMatrix4fv(locM, 1, GL_FALSE, identity.m);
    glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
}
