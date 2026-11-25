#include "../src/dispatcher_radio.h"

#include "../src/dispatcher_relevance.h"

#include <cassert>
#include <set>
#include <string>

struct Receiver {
    dispatch::Radio radio{42};
    std::vector<std::string> heard;
    std::string stale;
    void tick(bool busy = false, bool danger = false) {
        radio.transmit(busy, danger, [&](std::string_view id) { return id != stale; },
                       [&](std::string_view, const char* id) { heard.emplace_back(id); return true; });
    }
    void expect(const char* prefix) {
        assert(!heard.empty() && heard.back().rfind(prefix, 0) == 0);
    }
};

void testCatalog() {
    std::set<std::string_view> scripted, policies;
    for (auto line : dispatch::lines) scripted.insert(line.event);
    for (auto s : dispatch::situations) assert(policies.insert(s.id).second);
    assert(scripted == policies);
}

void testSubtitlesFollowSuccessfulPlayback() {
    Receiver r;
    assert(!r.radio.subtitle());
    auto heardCaption = [&] {
        for (const auto& line : dispatch::lines)
            if (r.heard.back() == line.clip) return line.text;
        throw std::logic_error("Test received an unknown recording");
    };
    r.radio.offer("long_walk");
    r.tick();
    assert(r.radio.subtitle() == heardCaption());
    r.radio.offer("first_shadow");
    r.radio.transmit(true, false, [](auto) { return true; }, [](auto, auto) { return false; });
    assert(r.radio.subtitle() == heardCaption());
    r.radio.offer("first_shadow");
    r.tick(true);
    assert(r.heard.size() == 2 && r.radio.subtitle() == heardCaption());
    r.tick();
    assert(!r.radio.subtitle());
    r.radio.advance(2);
    r.radio.offer("first_load");
    r.tick();
    assert(r.radio.subtitle());
    r.radio.clearPending();
    assert(!r.radio.subtitle());
}

void testPriorityAndInterruption() {
    Receiver r;
    r.radio.offer("long_walk");
    r.radio.offer("tutorial_haul");
    r.tick();
    r.expect("tutorial_haul_");
    r.radio.offer("first_stalker");
    r.tick(true);
    r.expect("first_stalker_");
    r.radio.offer("first_shadow");
    r.tick(true);
    assert(r.heard.size() == 2);
    r.radio.advance(2);
    r.tick();
    r.expect("first_shadow_");
}

void testExpiryAndRelevance() {
    Receiver r;
    r.radio.offer("long_walk");
    r.tick(false, true);
    r.tick();
    r.radio.offer("low_flashlight");
    r.stale = "low_flashlight";
    r.tick();
    r.stale.clear();
    r.tick();
    r.radio.offer("rig_fault");
    r.radio.advance(8);
    r.tick();
    assert(r.heard.empty());
}

void testVarietyAndCooldown() {
    Receiver r;
    const auto variants = std::count_if(std::begin(dispatch::lines), std::end(dispatch::lines),
                                       [](auto line) { return line.event == "long_walk"; });
    for (int i = 0; i <= variants; ++i) {
        r.radio.offer("long_walk");
        r.tick();
        assert(r.heard.size() == static_cast<std::size_t>(i + 1));
        r.radio.advance(1);
        r.tick();
        r.radio.offer("long_walk");
        r.radio.advance(2);
        r.tick();
        assert(r.heard.size() == static_cast<std::size_t>(i + 1));
        r.radio.advance(80);
    }
    assert(std::set<std::string>(r.heard.begin(), r.heard.end() - 1).size() == static_cast<std::size_t>(variants));
    assert(r.heard.back() != r.heard[r.heard.size() - 2]);
}

void testCooldownAcrossRuns() {
    Receiver r;
    r.radio.offer("first_load");
    r.radio.clearPending();
    r.tick();
    assert(r.heard.empty());
    r.radio.offer("pickup_clock");
    r.tick();
    assert(r.heard.size() == 1);
    r.radio.clearPending();
    r.radio.advance(599);
    r.radio.offer("pickup_clock");
    r.tick();
    assert(r.heard.size() == 1 && !r.radio.subtitle());
    r.radio.advance(1);
    r.tick(); // A suppressed pickup is never queued for later.
    assert(r.heard.size() == 1);
    r.radio.offer("pickup_clock");
    r.tick();
    assert(r.heard.size() == 2 && r.heard[0] != r.heard[1]);
}

void testExplorationAndEquipmentRelevance() {
    DispatcherEventFrame f;
    f.walking = true;
    f.lampOn = false;
    assert(dispatcherEventStillCurrent("dark_walk", f));
    f.lampOn = true;
    assert(!dispatcherEventStillCurrent("dark_walk", f));
    f.walking = false;
    f.lampOn = false;
    assert(!dispatcherEventStillCurrent("dark_walk", f));
    f.scannerHot = true;
    assert(dispatcherEventStillCurrent("scanner_overheat", f));
    assert(!dispatcherEventStillCurrent("scanner_cooled", f));
    f.scannerHot = false;
    assert(!dispatcherEventStillCurrent("scanner_overheat", f));
    assert(dispatcherEventStillCurrent("scanner_cooled", f));
    f.blackout = true;
    assert(dispatcherEventStillCurrent("floor_blackout", f));
    assert(!dispatcherEventStillCurrent("lights_return", f));
    f.blackout = false;
    assert(!dispatcherEventStillCurrent("floor_blackout", f));
    assert(dispatcherEventStillCurrent("lights_return", f));
    assert(!dispatcherEventStillCurrent("charger_ready", f));
    assert(!dispatcherEventStillCurrent("pause_to_listen", f));
    assert(!dispatcherEventStillCurrent("reserve_spent", f));
    assert(!dispatcherEventStillCurrent("far_from_rig", f));
    f.footStill = f.chargerReady = f.reserveSpent = f.farFromRig = true;
    assert(dispatcherEventStillCurrent("charger_ready", f));
    assert(dispatcherEventStillCurrent("pause_to_listen", f));
    assert(dispatcherEventStillCurrent("reserve_spent", f));
    assert(dispatcherEventStillCurrent("far_from_rig", f));
}

int main() {
    DispatcherEventFrame frame;
    assert(dispatcherEventStillCurrent("tutorial_garage", frame));
    assert(!dispatcherEventStillCurrent("tutorial_core", frame));
    assert(!dispatcherEventStillCurrent("first_shadow", frame));
    frame.monsters = 4;
    assert(dispatcherEventStillCurrent("first_shadow", frame));
    assert(!dispatcherEventStillCurrent("first_crawler", frame));
    assert(!dispatcherEventStillCurrent("pickup_clock", frame));
    frame.clocks = 1;
    assert(dispatcherEventStillCurrent("pickup_clock", frame));
    testCatalog();
    testSubtitlesFollowSuccessfulPlayback();
    testPriorityAndInterruption();
    testExpiryAndRelevance();
    testVarietyAndCooldown();
    testCooldownAcrossRuns();
    testExplorationAndEquipmentRelevance();
}
