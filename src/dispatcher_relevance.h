#pragma once

#include "dispatcher_events.h"
#include <string_view>

inline bool dispatcherEventStillCurrent(std::string_view id, const DispatcherEventFrame& f) {
    struct FlagCheck {
        std::string_view event;
        bool DispatcherEventFrame::* field;
        bool expected;
    };
    static constexpr FlagCheck flags[] = {
        {"rig_fault", &DispatcherEventFrame::broken, true},
        {"rig_repaired", &DispatcherEventFrame::broken, false},
        {"low_flashlight", &DispatcherEventFrame::lampBlinking, true},
        {"long_walk", &DispatcherEventFrame::walking, true},
        {"pause_to_listen", &DispatcherEventFrame::footStill, true},
        {"far_from_rig", &DispatcherEventFrame::farFromRig, true},
        {"scanner_overheat", &DispatcherEventFrame::scannerHot, true},
        {"scanner_cooled", &DispatcherEventFrame::scannerHot, false},
        {"floor_blackout", &DispatcherEventFrame::blackout, true},
        {"lights_return", &DispatcherEventFrame::blackout, false},
        {"charger_ready", &DispatcherEventFrame::chargerReady, true},
        {"reserve_spent", &DispatcherEventFrame::reserveSpent, true}
    };
    for (const auto& check : flags)
        if (id == check.event) return (f.*check.field) == check.expected;
    if (id == "tutorial_garage") return f.stage == HEIST_TUT_GARAGE;
    if (id == "tutorial_haul") return f.stage == HEIST_TUT_HAUL;
    if (id == "tutorial_core") return f.stage == HEIST_TUT_CORE;
    if (id == "drill_started") return f.stage == HEIST_DRILLING;
    if (id == "accident") return f.stage == HEIST_ACCIDENT;
    for (unsigned i = 0; i < dispatcherMonsterIds.size(); ++i)
        if (id == dispatcherMonsterIds[i]) return (f.monsters & (1u << i)) != 0;
    if (id == "pickup_clock") return f.clocks > 0;
    if (id == "cabin_recovery") return f.inCabin && f.recovering;
    if (id == "first_load" || id == "cargo_full") return f.carried >= (id == "first_load" ? 1 : HEIST_CARRY_SLOTS);
    if (id == "quota_ready") return f.stage == HEIST_RUN && f.quota >= f.quotaNeed;
    if (id == "run_briefing") return f.stage == HEIST_RUN && f.quota < f.quotaNeed;
    if (id == "dark_walk") return f.walking && !f.lampOn;
    return true;
}
