#pragma once
// Included at the end of map_content.h.

// ============================================================================
// THEMED PROP SPAWNING
// Dresses a chunk with props picked from the catalog by theme. Levels 0 and 1
// keep their hand-tuned generators; a new level only has to return its themes
// from levelDecorThemes() to get furnished rooms.
// ============================================================================

// Themes used by a level, 0 = none (legacy props only). The two floors right under the
// garage keep their hand-tuned props; every floor the drill reaches after that is dressed
// as a different kind of place, cycling through the ready-made mixes of prop_themes.h.
inline unsigned levelDecorThemes(int level) {
    static const unsigned deep[] = {
        THEME_BLEND_METRO, THEME_BLEND_MALL, THEME_BLEND_SCHOOL, THEME_BLEND_CLINIC,
        THEME_BLEND_RESORT, THEME_BLEND_FACILITY, THEME_BLEND_FUNHOUSE
    };
    if (level < 2) return 0u;
    return deep[(level - 2) % 7];
}

// Each chunk gets ONE of the level's themes, so a block of rooms reads as one place
// while neighbouring blocks may differ. Cross-theme props keep the joins soft.
inline unsigned chunkDecorTheme(unsigned levelThemes, int cx, int cz) {
    int count = 0;
    for (int i = 0; i < THEME_COUNT; i++) if (levelThemes & (1u << i)) count++;
    if (count == 0) return 0u;
    int pick = (int)(hashCell(cx, cz, 0x7E3E5u) % (unsigned)count);
    for (int i = 0; i < THEME_COUNT; i++) {
        if ((levelThemes & (1u << i)) == 0) continue;
        if (pick-- == 0) return 1u << i;
    }
    return 0u;
}

// Ceiling lamps sit on the even/even cell lattice (see updateLightsAndPillars).
inline bool isLampLatticeCell(int wx, int wz) {
    return (wx & 1) == 0 && (wz & 1) == 0;
}

inline bool placeThemedProp(int wx, int wz, int type, std::mt19937& r) {
    if (propPlacement(type) == PROP_PLACE_CEILING && isLampLatticeCell(wx, wz)) return false;
    return placeProp(wx, wz, type, r);
}

// Props that usually come together: a bed with its nightstand, a table with a chair.
enum PropBuddyMode {
    PROP_BUDDY_BESIDE = 0,  // next to the host, backs in line
    PROP_BUDDY_FRONT = 1    // in front of the host, facing it
};

struct PropCompanion {
    int host;
    int buddy;
    int mode;
    int chance;  // percent
};

const PropCompanion PROP_COMPANIONS[] = {
    {MAP_PROP_BED, MAP_PROP_NIGHTSTAND, PROP_BUDDY_BESIDE, 80},
    {MAP_PROP_GURNEY, MAP_PROP_IV_STAND, PROP_BUDDY_BESIDE, 70},
    {MAP_PROP_MEETING_TABLE, MAP_PROP_CHAIR, PROP_BUDDY_FRONT, 85},
    {MAP_PROP_DESK, MAP_PROP_CHAIR, PROP_BUDDY_FRONT, 80},
    {MAP_PROP_WORKBENCH, MAP_PROP_TOOL_CHEST, PROP_BUDDY_BESIDE, 55},
    {MAP_PROP_LOUNGE_CHAIR, MAP_PROP_LOUNGE_CHAIR, PROP_BUDDY_BESIDE, 65},
    {MAP_PROP_WAITING_SEATS, MAP_PROP_TRASH_BIN, PROP_BUDDY_BESIDE, 50},
    {MAP_PROP_VENDING_MACHINE, MAP_PROP_TRASH_BIN, PROP_BUDDY_BESIDE, 50},
    {MAP_PROP_COPIER, MAP_PROP_PAPER_STACKS, PROP_BUDDY_BESIDE, 45},
    {MAP_PROP_PALLET_RACK, MAP_PROP_PALLET_JACK, PROP_BUDDY_FRONT, 40},
    {MAP_PROP_SHOWER_POST, MAP_PROP_FLOOR_DRAIN, PROP_BUDDY_FRONT, 60},
    {MAP_PROP_JERSEY_BARRIER, MAP_PROP_CONE_CLUSTER, PROP_BUDDY_FRONT, 40},

    // rows: desks, gates, racks and machines come in lines
    {MAP_PROP_SCHOOL_DESK, MAP_PROP_SCHOOL_DESK, PROP_BUDDY_BESIDE, 75},
    {MAP_PROP_TURNSTILE, MAP_PROP_TURNSTILE, PROP_BUDDY_BESIDE, 80},
    {MAP_PROP_SERVER_RACK, MAP_PROP_SERVER_RACK, PROP_BUDDY_BESIDE, 75},
    {MAP_PROP_TAPE_DRIVE, MAP_PROP_TAPE_DRIVE, PROP_BUDDY_BESIDE, 50},
    {MAP_PROP_ARCADE_CABINET, MAP_PROP_ARCADE_CABINET, PROP_BUDDY_BESIDE, 65},
    {MAP_PROP_TICKET_MACHINE, MAP_PROP_TICKET_MACHINE, PROP_BUDDY_BESIDE, 50},
    {MAP_PROP_PAYPHONE, MAP_PROP_PAYPHONE, PROP_BUDDY_BESIDE, 40},
    {MAP_PROP_FITTING_BOOTH, MAP_PROP_FITTING_BOOTH, PROP_BUDDY_BESIDE, 60},
    {MAP_PROP_DINER_BOOTH, MAP_PROP_DINER_BOOTH, PROP_BUDDY_BESIDE, 60},
    {MAP_PROP_STACKED_CHAIRS, MAP_PROP_STACKED_CHAIRS, PROP_BUDDY_BESIDE, 50},

    // pairs that belong together
    {MAP_PROP_BLACKBOARD, MAP_PROP_SCHOOL_DESK, PROP_BUDDY_FRONT, 60},
    {MAP_PROP_LAB_BENCH, MAP_PROP_CHAIR, PROP_BUDDY_FRONT, 40},
    {MAP_PROP_STOVE_RANGE, MAP_PROP_PREP_COUNTER, PROP_BUDDY_BESIDE, 60},
    {MAP_PROP_SERVING_LINE, MAP_PROP_TRAY_RACK, PROP_BUDDY_BESIDE, 50},
    {MAP_PROP_CLOTHES_RACK, MAP_PROP_MANNEQUIN, PROP_BUDDY_BESIDE, 45},
    {MAP_PROP_GONDOLA_SHELF, MAP_PROP_SHOPPING_CART, PROP_BUDDY_FRONT, 35},
    {MAP_PROP_DISPLAY_COUNTER, MAP_PROP_STANCHIONS, PROP_BUDDY_FRONT, 35},
    {MAP_PROP_TICKET_BOOTH, MAP_PROP_STANCHIONS, PROP_BUDDY_FRONT, 45},
    {MAP_PROP_PARTY_TABLE, MAP_PROP_BALLOON_BUNCH, PROP_BUDDY_BESIDE, 65},
    {MAP_PROP_PLAY_SLIDE, MAP_PROP_BALL_PIT, PROP_BUDDY_FRONT, 50},
    {MAP_PROP_JANITOR_CART, MAP_PROP_WET_FLOOR_SIGN, PROP_BUDDY_FRONT, 50}
};

inline const PropCompanion* propCompanionFor(int host) {
    for (const PropCompanion& c : PROP_COMPANIONS) {
        if (c.host == host) return &c;
    }
    return nullptr;
}

inline bool placePropCompanion(const MapProp& host, const PropCompanion& rule, std::mt19937& r) {
    float hhx = 0.0f, hhz = 0.0f, bhx = 0.0f, bhz = 0.0f;
    mapPropBaseExtents(host.type, hhx, hhz);
    mapPropBaseExtents(rule.buddy, bhx, bhz);
    Vec3 right(cosf(host.yaw), 0.0f, -sinf(host.yaw));
    Vec3 front(sinf(host.yaw), 0.0f, cosf(host.yaw));

    if (rule.mode == PROP_BUDDY_FRONT) {
        float reach = hhz + bhz + 0.12f + rand01(r) * 0.20f;
        return addMapProp(host.pos + front * reach, rule.buddy, 1.0f, host.yaw + 3.1415927f);
    }
    // both against the wall: keep the buddy's back on the wall too
    float back = (isMapPropWallType(host.type) && isMapPropWallType(rule.buddy)) ? (bhz - hhz) : 0.0f;
    float side = (r() & 1u) ? 1.0f : -1.0f;
    for (int attempt = 0; attempt < 2; attempt++) {
        Vec3 pos = host.pos + right * (side * (hhx + bhx + 0.10f)) + front * back;
        if (addMapProp(pos, rule.buddy, 1.0f, host.yaw)) return true;
        side = -side;
    }
    return false;
}

// Places up to `wantLow` floor / wall / flat props and `wantOverhead` pipes, ducts and
// signs of the given theme(s). Returns how many props were added (companions included).
inline int spawnThemedProps(const Chunk& c, std::mt19937& r, unsigned themes, int wantLow, int wantOverhead) {
    const unsigned lowMask = (1u << PROP_PLACE_FLOOR) | (1u << PROP_PLACE_WALL) | (1u << PROP_PLACE_FLAT);
    const unsigned highMask = (1u << PROP_PLACE_WALL_HIGH) | (1u << PROP_PLACE_CEILING);
    int before = (int)mapProps.size();
    for (int pass = 0; pass < 2; pass++) {
        int want = (pass == 0) ? wantLow : wantOverhead;
        unsigned mask = (pass == 0) ? lowMask : highMask;
        int placed = 0;
        for (int attempt = 0; attempt < want * 12 && placed < want; attempt++) {
            int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
            int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
            int type = pickThemedPropType(themes, r(), mask);
            if (type < 0) break;
            if (!isMapPropCellValid(c, lx, lz)) continue;
            if (!placeThemedProp(c.cx * CHUNK_SIZE + lx, c.cz * CHUNK_SIZE + lz, type, r)) continue;
            placed++;
            const PropCompanion* rule = propCompanionFor(type);
            if (!rule || (PROP_CATALOG[rule->buddy].themes & themes) == 0) continue;
            if ((int)(r() % 100u) >= rule->chance) continue;
            const MapProp host = mapProps.back();
            placePropCompanion(host, *rule, r);
        }
    }
    return (int)mapProps.size() - before;
}

inline void spawnThemedChunkProps(const Chunk& c, std::mt19937& r) {
    unsigned theme = chunkDecorTheme(levelDecorThemes(gCurrentLevel), c.cx, c.cz);
    if (theme == 0u) return;
    spawnThemedProps(c, r, theme, 7 + (int)(r() % 4u), 2 + (int)(r() % 2u));
}
