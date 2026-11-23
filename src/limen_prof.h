#pragma once
// Tiny scoped profiler for the walk benchmark (`limen_shots.exe bench`). Compiles to nothing
// in the game build.
#ifdef LIMEN_SHOTS
#include <chrono>
#include <cstdio>
#include <cstring>
struct LmProfSlot { const char* name; double ms; long long calls; double worst; };
inline LmProfSlot gLmProf[64];
inline int gLmProfCount = 0;
struct LmProfScope {
    int idx;
    std::chrono::high_resolution_clock::time_point t0;
    explicit LmProfScope(const char* name) {
        idx = -1;
        for (int k = 0; k < gLmProfCount; k++) if (gLmProf[k].name == name || strcmp(gLmProf[k].name, name) == 0) { idx = k; break; }
        if (idx < 0 && gLmProfCount < 64) { idx = gLmProfCount++; gLmProf[idx] = {name, 0.0, 0, 0.0}; }
        t0 = std::chrono::high_resolution_clock::now();
    }
    ~LmProfScope() {
        if (idx < 0) return;
        double t = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();
        gLmProf[idx].ms += t;
        if (t > gLmProf[idx].worst) gLmProf[idx].worst = t;
        gLmProf[idx].calls++;
    }
};
inline void lmProfReset() { gLmProfCount = 0; }
inline void lmProfPrint(double perDiv) {
    for (int k = 0; k < gLmProfCount; k++)
        printf("  %-28s %9.2f ms/crossing  (%lld calls, worst %.2f ms)\n", gLmProf[k].name, gLmProf[k].ms / perDiv, gLmProf[k].calls, gLmProf[k].worst);
}
#define LM_PROF_CAT2(a, b) a##b
#define LM_PROF_CAT(a, b) LM_PROF_CAT2(a, b)
#define LM_PROF(name) LmProfScope LM_PROF_CAT(lmProfScope_, __LINE__)(name)
#else
#define LM_PROF(name) ((void)0)
#endif
