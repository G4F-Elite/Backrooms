#pragma once

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace dispatch {
enum Priority { scenery, advice, objective, warning };
struct Situation {
    std::string_view id;
    Priority priority;
    float expires, repeatAfter;
};

inline constexpr Situation situations[] = {
    {"tutorial_garage", objective, 12, 60}, {"tutorial_haul", objective, 10, 60},
    {"tutorial_core", objective, 10, 60}, {"run_briefing", objective, 12, 60},
    {"accident", warning, 8, 60}, {"first_stalker", warning, 6, 60},
    {"first_crawler", warning, 6, 60}, {"first_shadow", warning, 6, 60},
    {"pickup_clock", advice, 5, 600}, {"first_load", advice, 7, 90},
    {"cargo_full", advice, 5, 100}, {"first_deposit", advice, 7, 90},
    {"quota_ready", objective, 8, 60}, {"drill_started", objective, 6, 60},
    {"rig_fault", warning, 7, 20}, {"rig_repaired", objective, 6, 20},
    {"low_flashlight", advice, 4, 120}, {"cabin_recovery", advice, 6, 120},
    {"wall_broken", advice, 5, 80}, {"study_complete", advice, 6, 60},
    {"first_wall_door", scenery, 4, 120}, {"first_ceiling_stair", scenery, 4, 120},
    {"first_echo_hall", scenery, 4, 120}, {"first_chasm", scenery, 4, 120},
    {"first_twin", scenery, 4, 120}, {"long_walk", scenery, 3, 80},
    {"first_facewall", scenery, 4, 120}, {"first_vitrine", scenery, 4, 120},
    {"first_plugged_hall", scenery, 4, 120}, {"first_nested_rooms", scenery, 4, 120},
    {"first_spiral", scenery, 4, 120}, {"first_skywell", scenery, 4, 120},
    {"first_basin", scenery, 4, 120}, {"first_dark_wing", scenery, 4, 120},
    {"first_xenograft", scenery, 4, 120}, {"route_return", scenery, 4, 100},
    {"pause_to_listen", scenery, 3, 100}, {"dark_walk", scenery, 4, 120},
    {"far_from_rig", scenery, 5, 120}, {"scanner_overheat", advice, 4, 60},
    {"scanner_cooled", advice, 3, 60}, {"floor_blackout", advice, 5, 75},
    {"lights_return", scenery, 4, 75}, {"charger_ready", advice, 5, 60},
    {"reserve_spent", advice, 5, 60}, {"floor_descent", objective, 8, 0}
};
inline constexpr std::size_t count = sizeof(situations) / sizeof(*situations);

struct Line { std::string_view event; const char* clip; const char* text; };
inline constexpr Line lines[] = {
#define DISPATCH_LINE(event, clip, text) {#event, #clip, text},
#include "dispatcher_lines.inc"
#undef DISPATCH_LINE
};

struct Slot {
    float expiry = -1, nextAllowed = 0;
    std::vector<const Line*> bag;
    const Line* last = nullptr;
};

class Radio {
    std::array<Slot, count> slots;
    std::mt19937 random;
    float clock = 0, silenceUntil = 0;
    int speaking = -1;

    const Line* pick(std::size_t index) {
        Slot& slot = slots[index];
        if (slot.bag.empty()) {
            for (const Line& line : lines)
                if (line.event == situations[index].id) slot.bag.push_back(&line);
            if (slot.bag.empty()) throw std::logic_error("No recordings for dispatcher event");
            std::shuffle(slot.bag.begin(), slot.bag.end(), random);
            if (slot.bag.size() > 1 && slot.bag.back() == slot.last)
                std::swap(slot.bag.front(), slot.bag.back());
        }
        return slot.bag.back();
    }
public:
    explicit Radio(unsigned seed) : random(seed) {}

    const char* subtitle() const {
        return speaking < 0 ? nullptr : slots[speaking].last->text;
    }

    // On death or a floor change only pending speech belongs to the old world.
    // Playback history and cooldowns use the same session-wide gameplay clock.
    void clearPending() {
        speaking = -1;
        silenceUntil = clock;
        for (Slot& slot : slots) slot.expiry = -1;
    }
    void advance(float dt) { clock += dt; }
    void offer(std::string_view id) {
        for (std::size_t i = 0; i < count; ++i) {
            if (situations[i].id != id) continue;
            if (clock >= slots[i].nextAllowed) slots[i].expiry = clock + situations[i].expires;
            return;
        }
        throw std::logic_error("Unknown dispatcher event");
    }

    template<class Current, class Play>
    void transmit(bool busy, bool danger, Current current, Play play) {
        if (!busy && speaking >= 0) {
            speaking = -1;
            silenceUntil = clock + 1.8f;
        }
        int best = -1;
        for (std::size_t i = 0; i < count; ++i) {
            Slot& slot = slots[i];
            if (slot.expiry < clock) continue;
            const Situation& s = situations[i];
            if (!current(s.id) || (danger && s.priority == scenery)) {
                slot.expiry = -1;
                continue;
            }
            if (busy && (speaking < 0 || s.priority != warning ||
                         situations[speaking].priority == warning)) continue;
            if (clock < silenceUntil && s.priority < warning) continue;
            if (best < 0 || s.priority > situations[best].priority) best = static_cast<int>(i);
        }
        if (best < 0) return;
        Slot& slot = slots[best];
        slot.expiry = -1;
        const Line* line = pick(best);
        if (!play(line->event, line->clip)) return;
        speaking = best;
        slot.last = line;
        slot.bag.pop_back();
        slot.nextAllowed = clock + situations[best].repeatAfter;
    }
};
} // namespace dispatch
