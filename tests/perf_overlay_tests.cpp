#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "../src/perf_overlay.h"

void testFrameTimeHistoryPushAndClamp() {
    float hist[8] = {};
    int head = 7;
    initFrameTimeHistory(hist, 8, 16.6f);
    pushFrameTimeSample(hist, 8, head, 200.0f);
    assert(head == 0);
    assert(hist[0] == 120.0f);
    pushFrameTimeSample(hist, 8, head, 0.01f);
    assert(head == 1);
    assert(hist[1] == 0.1f);
}

void testFrameTimeStats() {
    float hist[5] = {10.0f, 11.0f, 12.0f, 20.0f, 30.0f};
    float avg = averageFrameTimeMs(hist, 5);
    float p95 = percentileFrameTimeMs(hist, 5, 0.95f);
    assert(avg > 16.0f && avg < 17.0f);
    assert(p95 == 30.0f);
}

void testGraphBuild() {
    float hist[8] = {8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 22.0f};
    char graph[16] = {};
    buildFrameTimeGraph(hist, 8, 7, 8, graph, 16);
    assert((int)strlen(graph) == 8);
    for (int i = 0; i < 8; i++) {
        assert(graph[i] != '\0');
    }
}

void testPipelineFormatting() {
    char fg[80] = {};
    formatFrameGenPipeline(fg, 80, 180, 90, FRAME_GEN_MODE_200, true);
    assert(std::string(fg).find("90 -> 180") != std::string::npos);

    char upA[96] = {};
    formatUpscalePipeline(upA, 96, UPSCALER_MODE_OFF, 1280, 720, 1280, 720);
    assert(std::string(upA).find("NATIVE 1280x720") != std::string::npos);

    char upB[96] = {};
    formatUpscalePipeline(upB, 96, UPSCALER_MODE_FSR10, 960, 540, 1280, 720);
    assert(std::string(upB).find("960x540 -> 1280x720") != std::string::npos);
}

int main() {
    testFrameTimeHistoryPushAndClamp();
    testFrameTimeStats();
    testGraphBuild();
    testPipelineFormatting();
    std::cout << "All perf overlay tests passed.\n";
    return 0;
}
