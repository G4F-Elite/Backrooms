// Headless integration with the client entry points. Never creates a window or audio device.
#define main dispatcher_test_unused_client_main
#include "../src/game.cpp"
#undef main

#include <cassert>

static int renderPeak(int blocks) {
    int peak = 0;
    short pcm[BUF_LEN * AUDIO_CHANNELS];
    for (int block = 0; block < blocks; ++block) {
        fillAudio(pcm, BUF_LEN);
        for (short sample : pcm) peak = std::max(peak, std::abs(static_cast<int>(sample)));
    }
    return peak;
}

static bool captionFor(const char* caption, std::string_view event) {
    return caption && std::any_of(std::begin(dispatch::lines), std::end(dispatch::lines),
                                  [&](const auto& line) {
                                      return line.event == event && line.text == caption;
                                  });
}

static const char* pickClockAfterRespawn() {
    isPlayerDead = true;
    assert(!updateGameDispatcher());
    assert(!dispatcherVoiceBusy());
    carryClear(heistMyCarry());
    isPlayerDead = false;
    assert(!updateGameDispatcher()); // establish the empty inventory on this life
    assert(carryPush(heistMyCarry(), LOOT_CLOCK));
    return updateGameDispatcher();
}

int main() {
    dTime = 0.25f; // exactly representable so the 600-second boundary is unambiguous
    sndState.masterVol = sndState.voiceVol = 1.f;
    sndState.musicVol = sndState.ambienceVol = sndState.sfxVol = sndState.humVol = 0.f;
    heist.stage = HEIST_TUT_GARAGE;
    carryClear(heistMyCarry());
    gameState = STATE_GAME;
    const char* greeting = updateGameDispatcher();
    assert(greeting && *greeting);
    assert(dispatcherVoiceBusy());
    const int speechPeak = renderPeak(32);
    assert(speechPeak > 4000);

    gameState = STATE_PAUSE;
    assert(!updateGameDispatcher());
    const int pausePeak = renderPeak(16);
    assert(pausePeak < speechPeak / 10);
    gameState = STATE_SETTINGS_PAUSE;
    assert(!updateGameDispatcher());
    renderPeak(128);
    assert(dispatcherVoiceBusy());

    gameState = STATE_GAME;
    assert(updateGameDispatcher() == greeting);
    assert(renderPeak(32) > 4000);
    gameState = STATE_MENU;
    assert(!updateGameDispatcher());
    assert(!dispatcherVoiceBusy());
    assert(renderPeak(24) < speechPeak / 10);

    // A menu transition stops playback but preserves the garage greeting's cooldown.
    gameState = STATE_GAME;
    assert(!updateGameDispatcher());
    assert(!dispatcherVoiceBusy());
    heist.stage = HEIST_TUT_HAUL;
    assert(captionFor(updateGameDispatcher(), "tutorial_haul"));
    assert(dispatcherVoiceBusy());
    sndState.voiceVol = 0.f;
    renderPeak(4); // production slider smoothing
    assert(renderPeak(16) < 10);
    sndState.voiceVol = 1.f;
    sndState.masterVol = 0.f;
    renderPeak(4);
    assert(renderPeak(16) < 10);
    sndState.masterVol = 1.f;
    assert(renderPeak(32) > 4000);

    // The first clock is eligible. Returning quickly with another clock is silent,
    // including the generic first-load message that used to follow clock pickups.
    assert(captionFor(pickClockAfterRespawn(), "pickup_clock"));
    assert(dispatcherVoiceBusy());
    assert(!pickClockAfterRespawn());
    for (int i = 0; i < 4 * 598; ++i) {
        assert(!updateGameDispatcher());
        assert(!dispatcherVoiceBusy());
    }
    assert(!pickClockAfterRespawn()); // 599 seconds of gameplay since the first clock
    for (int i = 0; i < 2; ++i) assert(!updateGameDispatcher());
    assert(captionFor(pickClockAfterRespawn(), "pickup_clock")); // exactly 600 seconds

    // Exercise a blackout through the same live state, caption selector and audio
    // mixer used by the client. The next floor may begin in a blackout on join,
    // so establish a normally lit floor before switching the lights off.
    gameState = STATE_MENU;
    updateGameDispatcher();
    gameState = STATE_GAME;
    heist.stage = HEIST_RUN;
    heist.quotaNeed = 5;
    lightsOutTimer = 0;
    updateGameDispatcher();
    renderPeak(320);
    updateGameDispatcher();
    for (int i = 0; i < 10; ++i) updateGameDispatcher();
    lightsOutTimer = 8;
    const char* blackout = updateGameDispatcher();
    assert(blackout && dispatcherVoiceBusy());
    assert(captionFor(blackout, "floor_blackout"));
    assert(renderPeak(32) > 4000);
}
