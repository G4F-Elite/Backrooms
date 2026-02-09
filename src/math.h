#pragma once
#include <cmath>
#include <cstdint>

// ============================================================================
// FAST MATH SYSTEM
// When enabled, trades some precision for significant performance gains
// Useful for: distance calculations, lighting, physics, animations
// ============================================================================

inline bool g_fastMathEnabled = false;

// ============================================================================
// CONSTANTS
// ============================================================================
inline constexpr float MATH_PI = 3.14159265358979323846f;
inline constexpr float MATH_TAU = 6.28318530717958647692f;
inline constexpr float MATH_HALF_PI = 1.57079632679489661923f;
inline constexpr float MATH_INV_PI = 0.31830988618379067154f;
inline constexpr float MATH_INV_TAU = 0.15915494309189533577f;

// ============================================================================
// FAST INVERSE SQUARE ROOT (Q_rsqrt - Quake III algorithm)
// Legendary algorithm ~4x faster than 1.0f/sqrtf(x)
// Error: ~1% maximum
// ============================================================================
inline float fastInvSqrt(float x) {
    float xhalf = 0.5f * x;
    int i = *(int*)&x;              // evil floating point bit hack
    i = 0x5f3759df - (i >> 1);      // what the fuck?
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x); // 1st iteration of Newton's method
    // x = x * (1.5f - xhalf * x * x); // 2nd iteration (optional, more precision)
    return x;
}

// Double precision version for when needed
inline double fastInvSqrtD(double x) {
    double xhalf = 0.5 * x;
    int64_t i = *(int64_t*)&x;
    i = 0x5fe6eb50c7b537a9 - (i >> 1);
    x = *(double*)&i;
    x = x * (1.5 - xhalf * x * x);
    return x;
}

// ============================================================================
// FAST SQUARE ROOT
// Uses inverse sqrt: sqrt(x) = x * rsqrt(x)
// ============================================================================
inline float fastSqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    return x * fastInvSqrt(x);
}

// ============================================================================
// FAST LENGTH / DISTANCE
// ============================================================================
inline float fastLength2D(float x, float y) {
    return fastSqrt(x * x + y * y);
}

inline float fastLength3D(float x, float y, float z) {
    return fastSqrt(x * x + y * y + z * z);
}

inline float fastDist2D(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return fastSqrt(dx * dx + dy * dy);
}

inline float fastDist3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    return fastSqrt(dx * dx + dy * dy + dz * dz);
}

// ============================================================================
// FAST TRIGONOMETRY
// ============================================================================
inline float wrapFastTrigAngle(float x) {
    while (x > MATH_PI) x -= MATH_TAU;
    while (x < -MATH_PI) x += MATH_TAU;
    return x;
}

// Fast sin using parabola approximation, error ~0.1%
inline float fastSinApprox(float x) {
    x = wrapFastTrigAngle(x);
    const float B = 1.2732395447f;   // 4/pi
    const float C = -0.4052847346f;  // -4/pi^2
    float y = B * x + C * x * fabsf(x);
    const float P = 0.225f;
    y = P * (y * fabsf(y) - y) + y;
    return y;
}

inline float fastCosApprox(float x) {
    return fastSinApprox(x + MATH_HALF_PI);
}

// Very fast sin/cos - less accurate but blazing fast (~5% error)
inline float veryFastSin(float x) {
    x = wrapFastTrigAngle(x);
    const float B = 1.2732395447f;
    const float C = -0.4052847346f;
    return B * x + C * x * fabsf(x);
}

inline float veryFastCos(float x) {
    return veryFastSin(x + MATH_HALF_PI);
}

// ============================================================================
// FAST ATAN2
// Approximation with ~0.3 degree max error
// ============================================================================
inline float fastAtan2(float y, float x) {
    const float PI_4 = 0.7853981633974483f;
    const float PI_3_4 = 2.356194490192345f;
    
    if (x == 0.0f && y == 0.0f) return 0.0f;
    
    float absY = fabsf(y) + 1e-10f; // Prevent division by zero
    float angle;
    
    if (x >= 0.0f) {
        float r = (x - absY) / (x + absY);
        angle = PI_4 - PI_4 * r;
    } else {
        float r = (x + absY) / (absY - x);
        angle = PI_3_4 - PI_4 * r;
    }
    
    return y < 0.0f ? -angle : angle;
}

// Even faster atan2 using polynomial (~1 degree error)
inline float veryFastAtan2(float y, float x) {
    float absX = fabsf(x), absY = fabsf(y);
    float a = (absX < absY) ? absX / (absY + 1e-10f) : absY / (absX + 1e-10f);
    float s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (absY > absX) r = MATH_HALF_PI - r;
    if (x < 0.0f) r = MATH_PI - r;
    if (y < 0.0f) r = -r;
    return r;
}

// ============================================================================
// FAST EXPONENTIAL / LOGARITHM
// ============================================================================

// Fast exp2 approximation (2^x)
inline float fastExp2(float x) {
    // Clamp to prevent overflow
    if (x < -126.0f) return 0.0f;
    if (x > 128.0f) return 3.402823466e+38f;
    
    int xi = (int)x;
    float xf = x - (float)xi;
    
    // IEEE 754 bit manipulation
    union { float f; int32_t i; } u;
    u.i = (xi + 127) << 23;
    
    // Polynomial approximation for fractional part
    float k = 1.0f + xf * (0.6931471805599453f + xf * (0.2402265069591007f + 
              xf * (0.0555041086648216f + xf * 0.009618129107628477f)));
    
    return u.f * k;
}

// Fast exp(x) = exp2(x * log2(e))
inline float fastExp(float x) {
    return fastExp2(x * 1.4426950408889634f);
}

// Fast log2 approximation
inline float fastLog2(float x) {
    if (x <= 0.0f) return -1e30f;
    
    union { float f; int32_t i; } u;
    u.f = x;
    
    int exp = ((u.i >> 23) & 0xFF) - 127;
    u.i = (u.i & 0x007FFFFF) | (127 << 23);
    
    // Polynomial approximation for mantissa
    float m = u.f;
    float log_m = -1.7417939f + m * (2.8212026f + m * (-1.4699568f + m * 0.44717955f));
    
    return (float)exp + log_m;
}

// Fast natural log: ln(x) = log2(x) / log2(e)
inline float fastLog(float x) {
    return fastLog2(x) * 0.6931471805599453f;
}

// Fast log10: log10(x) = log2(x) / log2(10)
inline float fastLog10(float x) {
    return fastLog2(x) * 0.3010299956639812f;
}

// ============================================================================
// FAST POWER FUNCTIONS
// ============================================================================

// Fast pow using exp/log: x^y = exp(y * ln(x))
inline float fastPow(float x, float y) {
    if (x <= 0.0f) return 0.0f;
    return fastExp2(y * fastLog2(x));
}

// Fast integer power (exact)
inline float fastPowInt(float x, int n) {
    if (n == 0) return 1.0f;
    if (n < 0) { x = 1.0f / x; n = -n; }
    float result = 1.0f;
    while (n > 0) {
        if (n & 1) result *= x;
        x *= x;
        n >>= 1;
    }
    return result;
}

// ============================================================================
// FAST FLOOR / CEIL / ROUND
// ============================================================================

inline int fastFloor(float x) {
    int i = (int)x;
    return i - (x < (float)i);
}

inline int fastCeil(float x) {
    int i = (int)x;
    return i + (x > (float)i);
}

inline int fastRound(float x) {
    return (int)(x + 0.5f - (x < 0.0f));
}

// ============================================================================
// FAST ABS / SIGN / CLAMP
// ============================================================================

inline float fastAbs(float x) {
    int i = *(int*)&x;
    i &= 0x7FFFFFFF;
    return *(float*)&i;
}

inline float fastSign(float x) {
    return (x > 0.0f) - (x < 0.0f);
}

inline float fastClamp(float x, float lo, float hi) {
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

inline float fastClamp01(float x) {
    return (x < 0.0f) ? 0.0f : ((x > 1.0f) ? 1.0f : x);
}

// ============================================================================
// FAST LERP / SMOOTHSTEP
// ============================================================================

inline float fastLerp(float a, float b, float t) {
    return a + t * (b - a);  // Faster than a*(1-t) + b*t
}

inline float fastSmoothstep(float edge0, float edge1, float x) {
    float t = fastClamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

inline float fastSmootherStep(float edge0, float edge1, float x) {
    float t = fastClamp01((x - edge0) / (edge1 - edge0));
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// ============================================================================
// FAST NORMALIZE ANGLE
// ============================================================================

inline float fastNormalizeAngle(float a) {
    a = fmodf(a + MATH_PI, MATH_TAU);
    if (a < 0.0f) a += MATH_TAU;
    return a - MATH_PI;
}

inline float fastAngleDiff(float from, float to) {
    float diff = fmodf(to - from + MATH_PI, MATH_TAU);
    if (diff < 0.0f) diff += MATH_TAU;
    return diff - MATH_PI;
}

// ============================================================================
// WRAPPER FUNCTIONS (respects g_fastMathEnabled)
// ============================================================================

inline float mSin(float x) {
    return g_fastMathEnabled ? fastSinApprox(x) : sinf(x);
}

inline float mCos(float x) {
    return g_fastMathEnabled ? fastCosApprox(x) : cosf(x);
}

inline float mTan(float x) {
    if (g_fastMathEnabled) {
        float c = fastCosApprox(x);
        return (fabsf(c) > 1e-6f) ? fastSinApprox(x) / c : 0.0f;
    }
    return tanf(x);
}

inline float mAtan2(float y, float x) {
    return g_fastMathEnabled ? fastAtan2(y, x) : atan2f(y, x);
}

inline float mSqrt(float x) {
    return g_fastMathEnabled ? fastSqrt(x) : sqrtf(x);
}

inline float mInvSqrt(float x) {
    return g_fastMathEnabled ? fastInvSqrt(x) : (1.0f / sqrtf(x));
}

inline float mExp(float x) {
    return g_fastMathEnabled ? fastExp(x) : expf(x);
}

inline float mLog(float x) {
    return g_fastMathEnabled ? fastLog(x) : logf(x);
}

inline float mPow(float x, float y) {
    return g_fastMathEnabled ? fastPow(x, y) : powf(x, y);
}

// ============================================================================
// VEC3 STRUCT (uses fast math when enabled)
// ============================================================================

struct Vec3 {
    float x, y, z;
    
    Vec3(float a = 0, float b = 0, float c = 0) : x(a), y(b), z(c) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { float inv = 1.0f / s; return Vec3(x * inv, y * inv, z * inv); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    
    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const { return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    
    float lenSq() const { return x * x + y * y + z * z; }
    
    float len() const {
        float sq = x * x + y * y + z * z;
        return g_fastMathEnabled ? fastSqrt(sq) : sqrtf(sq);
    }
    
    // Fast normalization using inverse sqrt
    Vec3 norm() const {
        float sq = x * x + y * y + z * z;
        if (sq <= 0.0f) return Vec3();
        float inv = g_fastMathEnabled ? fastInvSqrt(sq) : (1.0f / sqrtf(sq));
        return Vec3(x * inv, y * inv, z * inv);
    }
    
    // Normalize in place
    void normalize() {
        float sq = x * x + y * y + z * z;
        if (sq > 0.0f) {
            float inv = g_fastMathEnabled ? fastInvSqrt(sq) : (1.0f / sqrtf(sq));
            x *= inv; y *= inv; z *= inv;
        }
    }
    
    // Fast distance to another point
    float distTo(const Vec3& v) const {
        return (*this - v).len();
    }
    
    float distSqTo(const Vec3& v) const {
        return (*this - v).lenSq();
    }
    
    // Reflect vector off surface with normal n
    Vec3 reflect(const Vec3& n) const {
        return *this - n * (2.0f * dot(n));
    }
    
    // Lerp between vectors
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return Vec3(
            a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z)
        );
    }
};

// ============================================================================
// MAT4 STRUCT (4x4 transformation matrix)
// ============================================================================

struct Mat4 {
    float m[16];
    
    Mat4() {
        for (int i = 0; i < 16; i++) m[i] = 0;
        m[0] = m[5] = m[10] = m[15] = 1;
    }
    
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 16; i++) r.m[i] = 0;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i * 4 + j] += m[i * 4 + k] * o.m[k * 4 + j];
        return r;
    }
    
    Vec3 transformPoint(const Vec3& v) const {
        float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
        if (fabsf(w) < 1e-6f) w = 1.0f;
        float invW = g_fastMathEnabled ? fastInvSqrt(w * w) * w : 1.0f / w;
        return Vec3(
            (m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12]) * invW,
            (m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13]) * invW,
            (m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]) * invW
        );
    }
    
    Vec3 transformDir(const Vec3& v) const {
        return Vec3(
            m[0] * v.x + m[4] * v.y + m[8] * v.z,
            m[1] * v.x + m[5] * v.y + m[9] * v.z,
            m[2] * v.x + m[6] * v.y + m[10] * v.z
        );
    }
    
    static Mat4 translate(float x, float y, float z) {
        Mat4 r;
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }
    
    static Mat4 scale(float x, float y, float z) {
        Mat4 r;
        r.m[0] = x; r.m[5] = y; r.m[10] = z;
        return r;
    }
    
    static Mat4 rotateX(float angle) {
        Mat4 r;
        float c = mCos(angle), s = mSin(angle);
        r.m[5] = c; r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }
    
    static Mat4 rotateY(float angle) {
        Mat4 r;
        float c = mCos(angle), s = mSin(angle);
        r.m[0] = c; r.m[2] = -s;
        r.m[8] = s; r.m[10] = c;
        return r;
    }
    
    static Mat4 rotateZ(float angle) {
        Mat4 r;
        float c = mCos(angle), s = mSin(angle);
        r.m[0] = c; r.m[1] = s;
        r.m[4] = -s; r.m[5] = c;
        return r;
    }
    
    static Mat4 persp(float fov, float asp, float n, float f) {
        Mat4 r;
        for (int i = 0; i < 16; i++) r.m[i] = 0;
        float t = g_fastMathEnabled ? (1.0f / fastSinApprox(fov * 0.5f) * fastCosApprox(fov * 0.5f)) : (1.0f / tanf(fov * 0.5f));
        r.m[0] = t / asp;
        r.m[5] = t;
        r.m[10] = -(f + n) / (f - n);
        r.m[11] = -1;
        r.m[14] = -(2 * f * n) / (f - n);
        return r;
    }
    
    static Mat4 look(Vec3 e, Vec3 c, Vec3 u) {
        Vec3 f = (c - e).norm();
        Vec3 s = f.cross(u).norm();
        Vec3 up = s.cross(f);
        Mat4 r;
        r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
        r.m[1] = up.x; r.m[5] = up.y; r.m[9] = up.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -s.dot(e); r.m[13] = -up.dot(e); r.m[14] = f.dot(e);
        return r;
    }
};

// ============================================================================
// ADDITIONAL FAST UTILITIES
// ============================================================================

// Fast distance check (avoids sqrt when possible)
inline bool isWithinDistance(const Vec3& a, const Vec3& b, float maxDist) {
    return a.distSqTo(b) <= maxDist * maxDist;
}

// Fast angle between two vectors
inline float fastAngleBetween(const Vec3& a, const Vec3& b) {
    float dot = a.dot(b);
    float lenProd = a.lenSq() * b.lenSq();
    if (lenProd <= 0.0f) return 0.0f;
    float invLen = g_fastMathEnabled ? fastInvSqrt(lenProd) : (1.0f / sqrtf(lenProd));
    float cosAngle = fastClamp(dot * invLen, -1.0f, 1.0f);
    // Fast acos approximation
    if (g_fastMathEnabled) {
        float x = cosAngle;
        return MATH_HALF_PI - (x + x * x * x * 0.166666667f);
    }
    return acosf(cosAngle);
}

// Barycentric coordinates (for triangle intersection)
inline void barycentricCoords(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c, float& u, float& v, float& w) {
    Vec3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = v0.dot(v0);
    float d01 = v0.dot(v1);
    float d11 = v1.dot(v1);
    float d20 = v2.dot(v0);
    float d21 = v2.dot(v1);
    float denom = d00 * d11 - d01 * d01;
    if (fabsf(denom) < 1e-10f) { u = v = w = 0.33333f; return; }
    float invDenom = 1.0f / denom;
    v = (d11 * d20 - d01 * d21) * invDenom;
    w = (d00 * d21 - d01 * d20) * invDenom;
    u = 1.0f - v - w;
}