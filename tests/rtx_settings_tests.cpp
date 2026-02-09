// RTX Settings Tests
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

// Inline the RTX settings (no OpenGL needed for unit tests)
#include "../src/rtx_settings.h"

static int passed = 0, failed = 0;
#define CHECK(cond, msg) do { if(cond){ passed++; } else { failed++; printf("FAIL: %s\n", msg); } } while(0)

int main() {
    printf("=== RTX Settings Tests ===\n");
    
    // clampRtxMode
    CHECK(clampRtxMode(-1) == RTX_MODE_OFF, "clamp negative to OFF");
    CHECK(clampRtxMode(0) == RTX_MODE_OFF, "clamp 0 = OFF");
    CHECK(clampRtxMode(1) == RTX_MODE_LOW, "clamp 1 = LOW");
    CHECK(clampRtxMode(2) == RTX_MODE_MEDIUM, "clamp 2 = MEDIUM");
    CHECK(clampRtxMode(3) == RTX_MODE_HIGH, "clamp 3 = HIGH");
    CHECK(clampRtxMode(4) == RTX_MODE_ULTRA, "clamp 4 = ULTRA");
    CHECK(clampRtxMode(5) == RTX_MODE_ULTRA, "clamp 5 -> ULTRA (max)");
    CHECK(clampRtxMode(100) == RTX_MODE_ULTRA, "clamp 100 -> ULTRA");
    
    // stepRtxMode
    CHECK(stepRtxMode(RTX_MODE_OFF, 1) == RTX_MODE_LOW, "step OFF+1 = LOW");
    CHECK(stepRtxMode(RTX_MODE_LOW, 1) == RTX_MODE_MEDIUM, "step LOW+1 = MED");
    CHECK(stepRtxMode(RTX_MODE_ULTRA, 1) == RTX_MODE_ULTRA, "step ULTRA+1 clamps");
    CHECK(stepRtxMode(RTX_MODE_OFF, -1) == RTX_MODE_OFF, "step OFF-1 clamps");
    CHECK(stepRtxMode(RTX_MODE_HIGH, -1) == RTX_MODE_MEDIUM, "step HIGH-1 = MED");
    CHECK(stepRtxMode(RTX_MODE_MEDIUM, -2) == RTX_MODE_OFF, "step MED-2 = OFF");
    
    // isRtxEnabled
    CHECK(!isRtxEnabled(RTX_MODE_OFF), "OFF not enabled");
    CHECK(isRtxEnabled(RTX_MODE_LOW), "LOW enabled");
    CHECK(isRtxEnabled(RTX_MODE_MEDIUM), "MED enabled");
    CHECK(isRtxEnabled(RTX_MODE_HIGH), "HIGH enabled");
    CHECK(isRtxEnabled(RTX_MODE_ULTRA), "ULTRA enabled");
    CHECK(!isRtxEnabled(-5), "negative not enabled");
    
    // rtxModeLabel
    CHECK(strcmp(rtxModeLabel(RTX_MODE_OFF), "OFF") == 0, "label OFF");
    CHECK(strcmp(rtxModeLabel(RTX_MODE_LOW), "LOW") == 0, "label LOW");
    CHECK(strcmp(rtxModeLabel(RTX_MODE_MEDIUM), "MEDIUM") == 0, "label MEDIUM");
    CHECK(strcmp(rtxModeLabel(RTX_MODE_HIGH), "HIGH") == 0, "label HIGH");
    CHECK(strcmp(rtxModeLabel(RTX_MODE_ULTRA), "ULTRA") == 0, "label ULTRA");
    CHECK(strcmp(rtxModeLabel(-1), "OFF") == 0, "label -1 = OFF");
    CHECK(strcmp(rtxModeLabel(99), "ULTRA") == 0, "label 99 = ULTRA");
    
    // rtxConfigForMode
    RtxConfig cfgOff = rtxConfigForMode(RTX_MODE_OFF);
    CHECK(!cfgOff.enabled, "OFF config disabled");
    CHECK(cfgOff.mode == RTX_MODE_OFF, "OFF config mode");
    
    RtxConfig cfgLow = rtxConfigForMode(RTX_MODE_LOW);
    CHECK(cfgLow.enabled, "LOW config enabled");
    CHECK(cfgLow.mode == RTX_MODE_LOW, "LOW config mode");
    CHECK(cfgLow.ssao.samples == 8, "LOW ssao 8 samples");
    CHECK(cfgLow.ssr.maxSteps == 24, "LOW ssr 24 steps");
    CHECK(cfgLow.volumetric.samples == 16, "LOW vol 16 samples");
    CHECK(cfgLow.pbrMaterials, "LOW pbr materials on");
    CHECK(!cfgLow.contactShadows, "LOW no contact shadows");
    
    RtxConfig cfgMed = rtxConfigForMode(RTX_MODE_MEDIUM);
    CHECK(cfgMed.ssao.samples == 16, "MED ssao 16 samples");
    CHECK(cfgMed.contactShadows, "MED contact shadows on");
    
    RtxConfig cfgHigh = rtxConfigForMode(RTX_MODE_HIGH);
    CHECK(cfgHigh.ssao.samples == 24, "HIGH ssao 24 samples");
    CHECK(cfgHigh.ssr.maxSteps == 64, "HIGH ssr 64 steps");
    CHECK(cfgHigh.volumetric.halfRes == 0, "HIGH vol full res");
    
    RtxConfig cfgUltra = rtxConfigForMode(RTX_MODE_ULTRA);
    CHECK(cfgUltra.ssao.samples == 32, "ULTRA ssao 32 samples");
    CHECK(cfgUltra.ssr.maxSteps == 96, "ULTRA ssr 96 steps");
    CHECK(cfgUltra.volumetric.samples == 48, "ULTRA vol 48 samples");
    CHECK(cfgUltra.giBounceMul > 0.35f, "ULTRA high GI bounce");
    
    // Quality escalation: each mode should have >= previous
    CHECK(cfgMed.ssao.samples >= cfgLow.ssao.samples, "MED >= LOW ssao samples");
    CHECK(cfgHigh.ssao.samples >= cfgMed.ssao.samples, "HIGH >= MED ssao samples");
    CHECK(cfgUltra.ssao.samples >= cfgHigh.ssao.samples, "ULTRA >= HIGH ssao samples");
    CHECK(cfgMed.ssr.maxSteps >= cfgLow.ssr.maxSteps, "MED >= LOW ssr steps");
    CHECK(cfgHigh.ssr.maxSteps >= cfgMed.ssr.maxSteps, "HIGH >= MED ssr steps");
    CHECK(cfgUltra.ssr.maxSteps >= cfgHigh.ssr.maxSteps, "ULTRA >= HIGH ssr steps");
    
    // VRAM estimate
    int vramOff = rtxVramEstimateMB(RTX_MODE_OFF, 1920, 1080);
    CHECK(vramOff == 0, "OFF vram = 0");
    
    int vramLow = rtxVramEstimateMB(RTX_MODE_LOW, 1920, 1080);
    CHECK(vramLow > 0, "LOW vram > 0");
    CHECK(vramLow < 200, "LOW vram reasonable (<200MB)");
    
    int vram4k = rtxVramEstimateMB(RTX_MODE_ULTRA, 3840, 2160);
    CHECK(vram4k > vramLow, "4K vram > 1080p vram");
    CHECK(vram4k < 1000, "4K ULTRA vram < 1000MB");
    
    // Roundtrip mode constants
    CHECK(RTX_MODE_OFF == 0, "OFF = 0");
    CHECK(RTX_MODE_LOW == 1, "LOW = 1");
    CHECK(RTX_MODE_MEDIUM == 2, "MEDIUM = 2");
    CHECK(RTX_MODE_HIGH == 3, "HIGH = 3");
    CHECK(RTX_MODE_ULTRA == 4, "ULTRA = 4");
    CHECK(RTX_MODE_COUNT == 5, "COUNT = 5");
    
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
