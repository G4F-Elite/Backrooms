#pragma once

// ============================================================================
// PROP THEMES
// A level (or a single chunk of it) is dressed with one or more of these.
// Every catalog row lists the themes its prop belongs to; props tagged with
// several themes are what makes neighbouring themes blend into each other.
//
// Adding a theme: new bit here (before THEME_ALL), name in propThemeName(),
// bump THEME_COUNT. tests/prop_library_tests.cpp then demands enough props
// for it (floor, wall, overhead and flat ones, signature and shared ones).
// ============================================================================

enum PropTheme : unsigned {
    THEME_OFFICE = 1u << 0,
    THEME_PARKING = 1u << 1,
    THEME_MAINT = 1u << 2,
    THEME_WAREHOUSE = 1u << 3,
    THEME_HOTEL = 1u << 4,
    THEME_POOL = 1u << 5,
    THEME_HOSPITAL = 1u << 6,
    THEME_SCHOOL = 1u << 7,     // classrooms, gym hall, corridors
    THEME_MALL = 1u << 8,       // shops and concourses
    THEME_PLAYROOM = 1u << 9,   // party rooms, soft play, arcade corners
    THEME_STATION = 1u << 10,   // metro concourse and platforms
    THEME_LAB = 1u << 11,       // laboratories and server rooms
    THEME_KITCHEN = 1u << 12,   // canteen, commercial kitchen, diner
    THEME_ALL = (1u << 13) - 1u
};

const int THEME_COUNT = 13;

inline const char* propThemeName(unsigned theme) {
    if (theme == THEME_OFFICE) return "office";
    if (theme == THEME_PARKING) return "parking";
    if (theme == THEME_MAINT) return "maintenance";
    if (theme == THEME_WAREHOUSE) return "warehouse";
    if (theme == THEME_HOTEL) return "hotel";
    if (theme == THEME_POOL) return "pool";
    if (theme == THEME_HOSPITAL) return "hospital";
    if (theme == THEME_SCHOOL) return "school";
    if (theme == THEME_MALL) return "mall";
    if (theme == THEME_PLAYROOM) return "playroom";
    if (theme == THEME_STATION) return "station";
    if (theme == THEME_LAB) return "lab";
    if (theme == THEME_KITCHEN) return "kitchen";
    return "mixed";
}

// Families of themes that share most of their everyday objects.
const unsigned THEME_INDUSTRIAL = THEME_PARKING | THEME_MAINT | THEME_WAREHOUSE;
const unsigned THEME_PUBLIC = THEME_OFFICE | THEME_HOTEL | THEME_HOSPITAL;
const unsigned THEME_CROWD = THEME_SCHOOL | THEME_MALL | THEME_STATION;   // halls built for many people
const unsigned THEME_STAFF = THEME_LAB | THEME_KITCHEN;                   // steel-and-tile work rooms

// Ready-made mixes for whole levels (see levelDecorThemes() in map_theme_spawn.h).
// Each chunk settles on one of the themes; the themes of a mix share enough props
// that walking from one block of rooms into the next does not feel like a cut.
const unsigned THEME_BLEND_SCHOOL = THEME_SCHOOL | THEME_KITCHEN | THEME_OFFICE;      // classrooms, canteen, staff rooms
const unsigned THEME_BLEND_MALL = THEME_MALL | THEME_PLAYROOM | THEME_KITCHEN;        // shops, play corner, food court
const unsigned THEME_BLEND_FUNHOUSE = THEME_PLAYROOM | THEME_POOL | THEME_SCHOOL;     // party rooms next to pools and gyms
const unsigned THEME_BLEND_METRO = THEME_STATION | THEME_MAINT | THEME_PARKING;       // concourse, service tunnels, car decks
const unsigned THEME_BLEND_FACILITY = THEME_LAB | THEME_MAINT | THEME_WAREHOUSE;      // labs, plant rooms, stores
const unsigned THEME_BLEND_CLINIC = THEME_HOSPITAL | THEME_LAB | THEME_KITCHEN;       // wards, labs, hospital kitchen
const unsigned THEME_BLEND_RESORT = THEME_HOTEL | THEME_POOL | THEME_MALL;            // rooms, spa, shopping arcade

const unsigned THEME_BLENDS[] = {
    THEME_BLEND_SCHOOL, THEME_BLEND_MALL, THEME_BLEND_FUNHOUSE, THEME_BLEND_METRO,
    THEME_BLEND_FACILITY, THEME_BLEND_CLINIC, THEME_BLEND_RESORT
};
