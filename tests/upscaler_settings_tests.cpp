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

int main() {
    testRenderScalePresetClamp();
    testRenderScalePresetValues();
    testRenderScalePresetStep();
    testEffectiveRenderScaleDependsOnMode();
    testUpscalerModeClampAndLabel();
    testFsrSharpnessClamp();
    testAaModeClampStepAndLabel();
    std::cout << "All upscaler settings tests passed.\n";
    return 0;
}
