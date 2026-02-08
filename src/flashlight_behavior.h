#pragma once

inline float flashlightPreShutdownBatteryThreshold() {
    return 2.0f;
}

inline float flashlightShutdownBlinkDuration() {
    return 0.55f;
}

inline bool shouldStartFlashlightShutdownBlink(float battery) {
    return battery <= flashlightPreShutdownBatteryThreshold();
}

inline bool isFlashlightShutdownBlinkFinished(float blinkTimer) {
    return blinkTimer >= flashlightShutdownBlinkDuration();
}

inline bool isFlashlightOnDuringShutdownBlink(float blinkTimer) {
    if (blinkTimer < 0.0f) return true;
    if (blinkTimer < 0.08f) return true;
    if (blinkTimer < 0.14f) return false;
    if (blinkTimer < 0.24f) return true;
    if (blinkTimer < 0.30f) return false;
    if (blinkTimer < 0.42f) return true;
    if (blinkTimer < 0.49f) return false;
    return true;
}
