#pragma once

inline void presentGameScene() {
    const bool gameplay = gameState == STATE_GAME || gameState == STATE_PAUSE ||
                          gameState == STATE_SETTINGS_PAUSE || gameState == STATE_INTRO;
    float intensity = 0.f;
    const float deathPulse = 0.55f + 0.45f * sinf(vhsTime * 2.9f);
    if (gameplay) {
        const float loss = std::clamp(1.f - playerSanity / 100.f, 0.f, 1.f);
        const float danger = entityMgr.dangerLevel;
        const float stress = std::clamp(danger * 0.65f + loss * 0.35f, 0.f, 1.f);
        intensity = settings.vhsIntensity * (1.f + 0.35f * stress);
        static float deathEnvelope = 0.f;
        deathEnvelope = std::clamp(deathEnvelope + (isPlayerDead ? 0.95f : -1.4f) * dTime, 0.f, 1.f);
        if (isPlayerDead) intensity += settings.vhsIntensity * deathEnvelope * (0.52f + 0.44f * sinf(vhsTime * 5.8f) * 0.45f + 0.44f * 0.55f);
    }
    float death = isPlayerDead ? 1.f : 0.f;
    if (gameState == STATE_GAME || gameState == STATE_PAUSE)
        death = isPlayerDead ? deathPulse : std::clamp(gSanityCollapseTimer / 2.3f, 0.f, 1.f);
    presentScene(0, std::min(intensity, 1.35f), death);
}
