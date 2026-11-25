#pragma once

#include "dispatcher_radio.h"
#include "dispatcher_game_events.h"

#include "dispatcher_relevance.h"

inline const char* updateGameDispatcher() {
    static dispatch::Radio radio(std::random_device{}());
    static DispatcherEventObserver observer;
    static bool active = false;
    const bool playing = gameState == STATE_GAME && !isPlayerDead && !playerEscaped && !gCoopDead;
    const bool paused = gameState == STATE_PAUSE || gameState == STATE_SETTINGS_PAUSE ||
                        gameState == STATE_NOTE || (gameState == STATE_GUIDE && guideReturnToPause);
    if (!playing) {
        if (paused && !isPlayerDead && !playerEscaped && !gCoopDead) {
            if (active) dispatcherPauseVoice(true);
        } else if (observer.initialized) {
            dispatcherStopVoice();
            radio.clearPending();
            observer = {};
        }
        active = false;
        return nullptr;
    }
    if (!active) dispatcherPauseVoice(false);
    active = true;
    if (observer.initialized && (observer.previous.seed != worldSeed || observer.previous.level != gCurrentLevel)) {
        dispatcherStopVoice();
        radio.clearPending();
    }
    radio.advance(dTime);
    pollDispatcherGameEvents(observer, [&](const char* id) { radio.offer(id); });
    radio.transmit(dispatcherVoiceBusy(), playerDowned || sndState.monsterMenace > 0.35f,
                   [&](std::string_view id) { return dispatcherEventStillCurrent(id, observer.previous); },
                   [&](std::string_view id, const char* clip) {
                       if (!dispatcherPlayVoice(clip)) return false;
                       observer.markSpoken(id);
                       return true;
                   });
    return radio.subtitle();
}
