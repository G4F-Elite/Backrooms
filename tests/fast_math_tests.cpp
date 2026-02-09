#include <cassert>
#include <cmath>
#include <iostream>
#include <chrono>

#include "../src/math.h"

// Tolerance levels
constexpr float TIGHT_TOL = 1e-6f;
constexpr float FAST_TOL = 0.06f;      // ~6% for fast trig
constexpr float VERY_FAST_TOL = 0.10f; // ~10% for very fast
constexpr float RSQRT_TOL = 0.02f;     // ~2% for inverse sqrt
constexpr float EXP_TOL = 0.05f;       // ~5% for exp/log

// ============================================================================
// BASIC TRIG TESTS
// ============================================================================

void testDefaultMatchesStdTrig() {
    g_fastMathEnabled = false;
    float a = 1.2345f;
    assert(std::fabs(mSin(a) - sinf(a)) < TIGHT_TOL);
    assert(std::fabs(mCos(a) - cosf(a)) < TIGHT_TOL);
    std::cout << "  [PASS] Standard trig functions match stdlib\n";
}

void testFastTrigApproximation() {
    g_fastMathEnabled = true;
    int tests = 0;
    float maxSinError = 0, maxCosError = 0;
    
    for (float a = -10.0f; a <= 10.0f; a += 0.1f) {
        float s = mSin(a);
        float c = mCos(a);
        float sinErr = std::fabs(s - sinf(a));
        float cosErr = std::fabs(c - cosf(a));
        
        maxSinError = std::max(maxSinError, sinErr);
        maxCosError = std::max(maxCosError, cosErr);
        
        assert(s >= -1.05f && s <= 1.05f);
        assert(c >= -1.05f && c <= 1.05f);
        assert(sinErr < FAST_TOL);
        assert(cosErr < FAST_TOL);
        tests++;
    }
    
    std::cout << "  [PASS] Fast sin/cos (tests=" << tests << ", maxSinErr=" << maxSinError << ", maxCosErr=" << maxCosError << ")\n";
}

void testVeryFastTrig() {
    float maxErr = 0;
    for (float a = -MATH_PI; a <= MATH_PI; a += 0.1f) {
        float err = std::fabs(veryFastSin(a) - sinf(a));
        maxErr = std::max(maxErr, err);
        assert(err < VERY_FAST_TOL);
    }
    std::cout << "  [PASS] Very fast sin (maxError=" << maxErr << ")\n";
}

// ============================================================================
// INVERSE SQRT TESTS (Quake III algorithm)
// ============================================================================

void testFastInvSqrt() {
    float maxError = 0;
    int tests = 0;
    
    for (float x = 0.01f; x <= 100.0f; x *= 1.3f) {
        float fast = fastInvSqrt(x);
        float std = 1.0f / sqrtf(x);
        float relErr = std::fabs(fast - std) / std;
        maxError = std::max(maxError, relErr);
        assert(relErr < RSQRT_TOL);
        tests++;
    }
    
    std::cout << "  [PASS] Fast inverse sqrt (tests=" << tests << ", maxRelError=" << maxError * 100.0f << "%)\n";
}

void testFastSqrt() {
    float maxError = 0;
    
    for (float x = 0.01f; x <= 1000.0f; x *= 1.5f) {
        float fast = fastSqrt(x);
        float std = sqrtf(x);
        float relErr = std::fabs(fast - std) / std;
        maxError = std::max(maxError, relErr);
        assert(relErr < RSQRT_TOL);
    }
    
    assert(fastSqrt(0.0f) == 0.0f);
    assert(fastSqrt(-1.0f) == 0.0f);
    
    std::cout << "  [PASS] Fast sqrt (maxRelError=" << maxError * 100.0f << "%)\n";
}

// ============================================================================
// ATAN2 TESTS
// ============================================================================

void testFastAtan2() {
    float maxError = 0;
    int tests = 0;
    
    for (float y = -5.0f; y <= 5.0f; y += 0.5f) {
        for (float x = -5.0f; x <= 5.0f; x += 0.5f) {
            if (x == 0.0f && y == 0.0f) continue;
            
            float fast = fastAtan2(y, x);
            float std = atan2f(y, x);
            float err = std::fabs(fast - std);
            maxError = std::max(maxError, err);
            assert(err < 0.02f); // ~1 degree
            tests++;
        }
    }
    
    std::cout << "  [PASS] Fast atan2 (tests=" << tests << ", maxError=" << maxError << " rad)\n";
}

void testVeryFastAtan2() {
    float maxError = 0;
    
    for (float y = -5.0f; y <= 5.0f; y += 0.7f) {
        for (float x = -5.0f; x <= 5.0f; x += 0.7f) {
            if (x == 0.0f && y == 0.0f) continue;
            
            float fast = veryFastAtan2(y, x);
            float std = atan2f(y, x);
            float err = std::fabs(fast - std);
            maxError = std::max(maxError, err);
            assert(err < 0.05f); // ~3 degrees
        }
    }
    
    std::cout << "  [PASS] Very fast atan2 (maxError=" << maxError << " rad)\n";
}

// ============================================================================
// EXP / LOG TESTS
// ============================================================================

void testFastExp() {
    float maxError = 0;
    
    for (float x = -5.0f; x <= 5.0f; x += 0.2f) {
        float fast = fastExp(x);
        float std = expf(x);
        float relErr = std::fabs(fast - std) / std;
        maxError = std::max(maxError, relErr);
        assert(relErr < EXP_TOL);
    }
    
    // Edge cases
    assert(fastExp(-200.0f) == 0.0f);
    
    std::cout << "  [PASS] Fast exp (maxRelError=" << maxError * 100.0f << "%)\n";
}

void testFastLog() {
    float maxError = 0;
    
    for (float x = 0.01f; x <= 100.0f; x *= 1.5f) {
        float fast = fastLog(x);
        float std = logf(x);
        float relErr = std::fabs(fast - std) / std::fabs(std);
        maxError = std::max(maxError, relErr);
        assert(relErr < EXP_TOL);
    }
    
    // Edge case
    assert(fastLog(0.0f) < -1e20f);
    assert(fastLog(-1.0f) < -1e20f);
    
    std::cout << "  [PASS] Fast log (maxRelError=" << maxError * 100.0f << "%)\n";
}

void testFastPow() {
    float maxError = 0;
    
    // Test various bases and exponents
    float bases[] = {0.5f, 1.0f, 2.0f, 3.0f, 10.0f};
    float exps[] = {0.5f, 1.0f, 2.0f, 2.5f, 3.0f};
    
    for (float base : bases) {
        for (float exp : exps) {
            float fast = fastPow(base, exp);
            float std = powf(base, exp);
            float relErr = std::fabs(fast - std) / std;
            maxError = std::max(maxError, relErr);
            assert(relErr < 0.1f); // 10% tolerance for pow
        }
    }
    
    // Integer power should be exact
    assert(std::fabs(fastPowInt(2.0f, 10) - 1024.0f) < 0.001f);
    assert(std::fabs(fastPowInt(3.0f, 4) - 81.0f) < 0.001f);
    assert(std::fabs(fastPowInt(2.0f, -2) - 0.25f) < 0.001f);
    
    std::cout << "  [PASS] Fast pow (maxRelError=" << maxError * 100.0f << "%)\n";
}

// ============================================================================
// FLOOR / CEIL / ROUND TESTS
// ============================================================================

void testFastFloorCeilRound() {
    // Floor tests
    assert(fastFloor(2.7f) == 2);
    assert(fastFloor(-2.7f) == -3);
    assert(fastFloor(2.0f) == 2);
    assert(fastFloor(-2.0f) == -2);
    
    // Ceil tests
    assert(fastCeil(2.1f) == 3);
    assert(fastCeil(-2.1f) == -2);
    assert(fastCeil(2.0f) == 2);
    
    // Round tests
    assert(fastRound(2.4f) == 2);
    assert(fastRound(2.6f) == 3);
    assert(fastRound(-2.4f) == -2);
    assert(fastRound(-2.6f) == -3);
    
    std::cout << "  [PASS] Fast floor/ceil/round\n";
}

// ============================================================================
// UTILITY FUNCTION TESTS
// ============================================================================

void testFastAbs() {
    assert(fastAbs(5.0f) == 5.0f);
    assert(fastAbs(-5.0f) == 5.0f);
    assert(fastAbs(0.0f) == 0.0f);
    std::cout << "  [PASS] Fast abs\n";
}

void testFastClamp() {
    assert(fastClamp(5.0f, 0.0f, 10.0f) == 5.0f);
    assert(fastClamp(-5.0f, 0.0f, 10.0f) == 0.0f);
    assert(fastClamp(15.0f, 0.0f, 10.0f) == 10.0f);
    
    assert(fastClamp01(0.5f) == 0.5f);
    assert(fastClamp01(-0.5f) == 0.0f);
    assert(fastClamp01(1.5f) == 1.0f);
    
    std::cout << "  [PASS] Fast clamp\n";
}

void testFastLerpSmoothstep() {
    assert(std::fabs(fastLerp(0.0f, 10.0f, 0.5f) - 5.0f) < TIGHT_TOL);
    assert(std::fabs(fastLerp(0.0f, 10.0f, 0.0f) - 0.0f) < TIGHT_TOL);
    assert(std::fabs(fastLerp(0.0f, 10.0f, 1.0f) - 10.0f) < TIGHT_TOL);
    
    // Smoothstep at edges
    assert(std::fabs(fastSmoothstep(0.0f, 1.0f, 0.0f) - 0.0f) < TIGHT_TOL);
    assert(std::fabs(fastSmoothstep(0.0f, 1.0f, 1.0f) - 1.0f) < TIGHT_TOL);
    assert(std::fabs(fastSmoothstep(0.0f, 1.0f, 0.5f) - 0.5f) < TIGHT_TOL);
    
    std::cout << "  [PASS] Fast lerp/smoothstep\n";
}

void testFastNormalizeAngle() {
    assert(std::fabs(fastNormalizeAngle(0.0f) - 0.0f) < TIGHT_TOL);
    assert(std::fabs(fastNormalizeAngle(MATH_PI) - MATH_PI) < 0.01f || std::fabs(fastNormalizeAngle(MATH_PI) + MATH_PI) < 0.01f);
    assert(std::fabs(fastNormalizeAngle(MATH_TAU + 0.5f) - 0.5f) < TIGHT_TOL);
    assert(std::fabs(fastNormalizeAngle(-MATH_TAU - 0.5f) + 0.5f) < TIGHT_TOL);
    
    std::cout << "  [PASS] Fast normalize angle\n";
}

// ============================================================================
// VEC3 TESTS WITH FAST MATH
// ============================================================================

void testVec3FastMath() {
    g_fastMathEnabled = true;
    
    Vec3 a(3, 4, 0);
    Vec3 b(1, 2, 3);
    
    // Length using fast sqrt
    float len = a.len();
    assert(std::fabs(len - 5.0f) < 0.1f);
    
    // Normalize using fast inv sqrt
    Vec3 n = a.norm();
    float normLen = n.len();
    assert(std::fabs(normLen - 1.0f) < 0.02f);
    
    // Distance
    float dist = a.distTo(b);
    float expectedDist = sqrtf(4 + 4 + 9);
    assert(std::fabs(dist - expectedDist) < 0.1f);
    
    // In-place normalize
    Vec3 c(3, 4, 0);
    c.normalize();
    assert(std::fabs(c.len() - 1.0f) < 0.02f);
    
    std::cout << "  [PASS] Vec3 fast math operations\n";
    g_fastMathEnabled = false;
}

void testVec3Lerp() {
    Vec3 a(0, 0, 0);
    Vec3 b(10, 20, 30);
    
    Vec3 mid = Vec3::lerp(a, b, 0.5f);
    assert(std::fabs(mid.x - 5.0f) < TIGHT_TOL);
    assert(std::fabs(mid.y - 10.0f) < TIGHT_TOL);
    assert(std::fabs(mid.z - 15.0f) < TIGHT_TOL);
    
    std::cout << "  [PASS] Vec3 lerp\n";
}

void testVec3Reflect() {
    Vec3 v(1, -1, 0);
    Vec3 n(0, 1, 0);
    Vec3 r = v.reflect(n);
    
    assert(std::fabs(r.x - 1.0f) < TIGHT_TOL);
    assert(std::fabs(r.y - 1.0f) < TIGHT_TOL);
    assert(std::fabs(r.z - 0.0f) < TIGHT_TOL);
    
    std::cout << "  [PASS] Vec3 reflect\n";
}

// ============================================================================
// MAT4 TESTS WITH FAST MATH
// ============================================================================

void testMat4Rotation() {
    g_fastMathEnabled = true;
    
    // Rotate 90 degrees around Y
    Mat4 rotY = Mat4::rotateY(MATH_HALF_PI);
    Vec3 point(1, 0, 0);
    Vec3 rotated = rotY.transformPoint(point);
    
    // After 90 degree Y rotation, (1,0,0) should be approximately (0,0,-1)
    assert(std::fabs(rotated.x) < 0.1f);
    assert(std::fabs(rotated.y) < 0.1f);
    assert(std::fabs(rotated.z + 1.0f) < 0.1f);
    
    std::cout << "  [PASS] Mat4 rotation with fast math\n";
    g_fastMathEnabled = false;
}

// ============================================================================
// UTILITY FUNCTION TESTS
// ============================================================================

void testIsWithinDistance() {
    Vec3 a(0, 0, 0);
    Vec3 b(3, 4, 0);
    
    assert(isWithinDistance(a, b, 5.1f) == true);
    assert(isWithinDistance(a, b, 4.9f) == false);
    
    std::cout << "  [PASS] isWithinDistance\n";
}

void testFastAngleBetween() {
    g_fastMathEnabled = true;
    
    Vec3 a(1, 0, 0);
    Vec3 b(0, 1, 0);
    
    float angle = fastAngleBetween(a, b);
    assert(std::fabs(angle - MATH_HALF_PI) < 0.2f); // ~11 degrees tolerance
    
    // Parallel vectors = 0 angle
    Vec3 c(2, 0, 0);
    float angle2 = fastAngleBetween(a, c);
    assert(std::fabs(angle2) < 0.1f);
    
    std::cout << "  [PASS] Fast angle between vectors\n";
    g_fastMathEnabled = false;
}

void testBarycentricCoords() {
    Vec3 a(0, 0, 0);
    Vec3 b(1, 0, 0);
    Vec3 c(0, 1, 0);
    
    float u, v, w;
    
    // Center of triangle
    Vec3 center(0.333f, 0.333f, 0);
    barycentricCoords(center, a, b, c, u, v, w);
    assert(std::fabs(u + v + w - 1.0f) < 0.01f);
    
    // At vertex a
    barycentricCoords(a, a, b, c, u, v, w);
    assert(std::fabs(u - 1.0f) < 0.01f);
    
    std::cout << "  [PASS] Barycentric coordinates\n";
}

// ============================================================================
// PERFORMANCE BENCHMARK
// ============================================================================

void benchmarkFastMath() {
    const int iterations = 100000;
    
    // Benchmark inverse sqrt
    auto start = std::chrono::high_resolution_clock::now();
    float sum = 0;
    for (int i = 1; i <= iterations; i++) {
        sum += 1.0f / sqrtf((float)i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto stdTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    float sum2 = 0;
    for (int i = 1; i <= iterations; i++) {
        sum2 += fastInvSqrt((float)i);
    }
    end = std::chrono::high_resolution_clock::now();
    auto fastTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    std::cout << "  [BENCH] Inverse sqrt: std=" << stdTime << "us, fast=" << fastTime << "us";
    if (fastTime > 0 && stdTime > fastTime) {
        std::cout << " (speedup: " << (float)stdTime / fastTime << "x)";
    }
    std::cout << "\n";
    
    // Benchmark sin
    start = std::chrono::high_resolution_clock::now();
    sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += sinf((float)i * 0.01f);
    }
    end = std::chrono::high_resolution_clock::now();
    stdTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    sum2 = 0;
    for (int i = 0; i < iterations; i++) {
        sum2 += fastSinApprox((float)i * 0.01f);
    }
    end = std::chrono::high_resolution_clock::now();
    fastTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    std::cout << "  [BENCH] Sin: std=" << stdTime << "us, fast=" << fastTime << "us";
    if (fastTime > 0 && stdTime > fastTime) {
        std::cout << " (speedup: " << (float)stdTime / fastTime << "x)";
    }
    std::cout << "\n";
    
    // Prevent optimization
    if (sum == sum2) std::cout << "";
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "=== Fast Math Tests ===\n\n";
    
    std::cout << "Trigonometry:\n";
    testDefaultMatchesStdTrig();
    testFastTrigApproximation();
    testVeryFastTrig();
    
    std::cout << "\nInverse Sqrt (Quake III):\n";
    testFastInvSqrt();
    testFastSqrt();
    
    std::cout << "\nAtan2:\n";
    testFastAtan2();
    testVeryFastAtan2();
    
    std::cout << "\nExponential/Logarithm:\n";
    testFastExp();
    testFastLog();
    testFastPow();
    
    std::cout << "\nFloor/Ceil/Round:\n";
    testFastFloorCeilRound();
    
    std::cout << "\nUtility Functions:\n";
    testFastAbs();
    testFastClamp();
    testFastLerpSmoothstep();
    testFastNormalizeAngle();
    
    std::cout << "\nVec3 Operations:\n";
    testVec3FastMath();
    testVec3Lerp();
    testVec3Reflect();
    
    std::cout << "\nMat4 Operations:\n";
    testMat4Rotation();
    
    std::cout << "\nGeometry Utilities:\n";
    testIsWithinDistance();
    testFastAngleBetween();
    testBarycentricCoords();
    
    std::cout << "\nPerformance Benchmarks:\n";
    benchmarkFastMath();
    
    std::cout << "\n=== All " << 24 << " fast math tests passed! ===\n";
    return 0;
}