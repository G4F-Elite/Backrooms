#include "../src/render_history.h"
#include <array>
#include <cassert>

namespace {
using Vector = std::array<double, 4>;

Vector gpuMultiply(const Mat4& matrix, const Vector& point) {
    Vector result{};
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            result[row] += matrix.m[column*4+row] * point[column];
    return result;
}

void near(double actual, double expected, double tolerance = 2e-5) {
    assert(std::abs(actual - expected) <= tolerance * std::max(1.0, std::abs(expected)));
}

void checkJitter() {
    const int w=733, h=511;
    const Mat4 projection = Mat4::persp(1.3f, float(w)/h, .15f, 220.f);
    std::array<Vector, 8> phases{};
    for (int i=0; i<16; ++i) {
        Mat4 shifted = projection;
        jitterSceneProjection(shifted, i, w, h);
        Vector ndcOffset{};
        for (double z : {-1.0, -53.0}) {
            const Vector point{.3,-.2,z,1};
            const auto before=gpuMultiply(projection,point), after=gpuMultiply(shifted,point);
            const double dx=(after[0]-before[0])/before[3]*w*.5;
            const double dy=(after[1]-before[1])/before[3]*h*.5;
            near(after[2],before[2]); near(after[3],before[3]);
            assert(std::abs(dx)<.5 && std::abs(dy)<.5);
            if(z == -1.0) ndcOffset={dx,dy,0,0};
            else { near(dx,ndcOffset[0]); near(dy,ndcOffset[1]); }
        }
        if (i < 8) {
            for (int j=0; j<i; ++j) assert(phases[j] != ndcOffset);
            phases[i] = ndcOffset;
        } else {
            assert(phases[i-8] == ndcOffset);
        }
    }
}

void checkReprojection() {
    Mat4 p0 = Mat4::persp(1.2f,1.7f,.1f,320.f), p1 = Mat4::persp(.95f,1.5f,.2f,130.f);
    jitterSceneProjection(p0,2,480,270);
    jitterSceneProjection(p1,5,960,540);
    const Mat4 v0 = Mat4::look({1,2,3},{-.5f,1,0},{0,1,0});
    const Mat4 v1 = Mat4::look({-2,1,4},{0,.5f,0},{0,1,0});
    const Mat4 current = v0*p0, previous = v1*p1;
    const Mat4 reprojection = previousClipFromCurrent(current, previous);
    for (double z : {-.35, -3., -37., -80.}) {
        const Vector world{.5,1.25,z,1};
        const Vector cur = gpuMultiply(p0,gpuMultiply(v0,world));
        const Vector prev = gpuMultiply(p1,gpuMultiply(v1,world));
        const Vector mapped = gpuMultiply(reprojection,cur);
        const Vector reverse = gpuMultiply(previousClipFromCurrent(previous,current),mapped);
        for (int i=0;i<4;++i) {near(mapped[i],prev[i],8e-5); near(reverse[i],cur[i],8e-5);}
    }
    bool rejected=false;
    try { previousClipFromCurrent(Mat4::scale(1,0,1), previous); }
    catch(const std::logic_error&) {rejected=true;}
    assert(rejected);
}
}

int main() {
    for(int up=0;up<UPSCALER_MODE_COUNT;++up) for(int aa=0;aa<AA_MODE_COUNT;++aa)
        assert(needsRenderHistory(up,aa) == (aa==AA_MODE_TAA || up==UPSCALER_MODE_FSR20));
    checkJitter();
    checkReprojection();
}
