#include <cassert>
#include <iostream>
#include <string>

#include "../src/upscaler_settings.h"

void testRenderScalePresetClamp() {
    assert(clampRenderScalePreset(-1) == 0);
    assert(clampRenderScalePreset(0) == 0);
    assert(clampRenderScalePreset(RENDER_SCALE_PRESET_COUNT - 1) == RENDER_SCALE_PRESET_COUNT - 1);
    assert(clampRenderScalePreset(99) == RENDER_SCALE_PRESET_COUNT - 1);
}

void testRenderScalePresetValues() {
    assert(renderScaleFromPreset(0) == 0.50f);
    assert(renderScaleFromPreset(RENDER_SCALE_PRESET_DEFAULT) == 0.75f);
    assert(renderScaleFromPreset(RENDER_SCALE_PRESET_COUNT - 1) == 1.00f);
}

void testRenderScalePresetStep() {
    assert(stepRenderScalePreset(RENDER_SCALE_PRESET_DEFAULT, -1) == RENDER_SCALE_PRESET_DEFAULT - 1);
    assert(stepRenderScalePreset(0, -1) == 0);
    assert(stepRenderScalePreset(RENDER_SCALE_PRESET_COUNT - 1, 1) == RENDER_SCALE_PRESET_COUNT - 1);
}

void testEffectiveRenderScaleDependsOnMode() {
    float offScale = effectiveRenderScale(UPSCALER_MODE_OFF, 0);
    float fsrScale = effectiveRenderScale(UPSCALER_MODE_FSR10, 0);
    assert(offScale == 1.0f);
    assert(fsrScale == 0.50f);
}

void testUpscalerModeClampAndLabel() {
    assert(clampUpscalerMode(-5) == UPSCALER_MODE_OFF);
    assert(clampUpscalerMode(UPSCALER_MODE_FSR10) == UPSCALER_MODE_FSR10);
    assert(clampUpscalerMode(99) == UPSCALER_MODE_FSR10);
    assert(std::string(upscalerModeLabel(UPSCALER_MODE_OFF)) == "OFF");
    assert(std::string(upscalerModeLabel(UPSCALER_MODE_FSR10)) == "FSR 1.0");
}

void testFsrSharpnessClamp() {
    assert(clampFsrSharpness(-0.2f) == 0.0f);
    assert(clampFsrSharpness(0.5f) == 0.5f);
    assert(clampFsrSharpness(1.2f) == 1.0f);
}

void testAaModeClampStepAndLabel() {
    assert(clampAaMode(-3) == AA_MODE_OFF);
    assert(clampAaMode(AA_MODE_OFF) == AA_MODE_OFF);
    assert(clampAaMode(AA_MODE_FXAA) == AA_MODE_FXAA);
    assert(clampAaMode(AA_MODE_TAA) == AA_MODE_TAA);
    assert(clampAaMode(999) == AA_MODE_TAA);

    assert(stepAaMode(AA_MODE_OFF, -1) == AA_MODE_OFF);
    assert(stepAaMode(AA_MODE_OFF, 1) == AA_MODE_FXAA);
    assert(stepAaMode(AA_MODE_FXAA, 1) == AA_MODE_TAA);
    assert(stepAaMode(AA_MODE_TAA, 1) == AA_MODE_TAA);

    assert(std::string(aaModeLabel(AA_MODE_OFF)) == "OFF");
    assert(std::string(aaModeLabel(AA_MODE_FXAA)) == "FXAA");
    assert(std::string(aaModeLabel(AA_MODE_TAA)) == "TAA");
}

void testFrameGenModeStepAndLabels() {
    assert(clampFrameGenMode(-10) == FRAME_GEN_MODE_OFF);
    assert(clampFrameGenMode(FRAME_GEN_MODE_150) == FRAME_GEN_MODE_150);
    assert(clampFrameGenMode(99) == FRAME_GEN_MODE_200);
    assert(stepFrameGenMode(FRAME_GEN_MODE_OFF, 1) == FRAME_GEN_MODE_150);
    assert(stepFrameGenMode(FRAME_GEN_MODE_150, 1) == FRAME_GEN_MODE_200);
    assert(stepFrameGenMode(FRAME_GEN_MODE_200, 1) == FRAME_GEN_MODE_200);
    assert(std::string(frameGenModeLabel(FRAME_GEN_MODE_OFF)) == "OFF");
    assert(std::string(frameGenModeLabel(FRAME_GEN_MODE_150)) == "1.5X");
    assert(std::string(frameGenModeLabel(FRAME_GEN_MODE_200)) == "2.0X");
}

void testFrameGenBlendAndMultiplier() {
    assert(frameGenMultiplier(FRAME_GEN_MODE_OFF) == 1.0f);
    assert(frameGenMultiplier(FRAME_GEN_MODE_150) == 1.5f);
    assert(frameGenMultiplier(FRAME_GEN_MODE_200) == 2.0f);
    assert(frameGenBlendStrength(FRAME_GEN_MODE_OFF) == 0.0f);
    assert(frameGenBlendStrength(FRAME_GEN_MODE_150) > 0.0f);
    assert(frameGenBlendStrength(FRAME_GEN_MODE_200) > frameGenBlendStrength(FRAME_GEN_MODE_150));
}

void testFrameGenBaseFpsCap() {
    assert(frameGenBaseFpsCap(180, FRAME_GEN_MODE_200, true) == 90);
    assert(frameGenBaseFpsCap(180, FRAME_GEN_MODE_150, true) == 120);
    assert(frameGenBaseFpsCap(165, FRAME_GEN_MODE_200, true) == 83);
    assert(frameGenBaseFpsCap(180, FRAME_GEN_MODE_OFF, true) == 0);
    assert(frameGenBaseFpsCap(180, FRAME_GEN_MODE_200, false) == 0);
}

int main() {
    testRenderScalePresetClamp();
    testRenderScalePresetValues();
    testRenderScalePresetStep();
    testEffectiveRenderScaleDependsOnMode();
    testUpscalerModeClampAndLabel();
    testFsrSharpnessClamp();
    testAaModeClampStepAndLabel();
    testFrameGenModeStepAndLabels();
    testFrameGenBlendAndMultiplier();
    testFrameGenBaseFpsCap();
    std::cout << "All upscaler settings tests passed.\n";
    return 0;
}
