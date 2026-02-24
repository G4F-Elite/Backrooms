#pragma once

#include <cstdlib>

#ifndef BR_UI_COMPILE_ALLOW_LEGACY
#define BR_UI_COMPILE_ALLOW_LEGACY 1
#endif

#ifndef BR_UI_COMPILE_ALLOW_NEW
#define BR_UI_COMPILE_ALLOW_NEW 1
#endif

#ifndef BR_UI_COMPILE_DEFAULT_NEW
#define BR_UI_COMPILE_DEFAULT_NEW 1
#endif

#ifndef BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX
#define BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX 0
#endif

#if (BR_UI_COMPILE_ALLOW_LEGACY != 0) && (BR_UI_COMPILE_ALLOW_LEGACY != 1)
#error "BR_UI_COMPILE_ALLOW_LEGACY must be 0 or 1."
#endif

#if (BR_UI_COMPILE_ALLOW_NEW != 0) && (BR_UI_COMPILE_ALLOW_NEW != 1)
#error "BR_UI_COMPILE_ALLOW_NEW must be 0 or 1."
#endif

#if (BR_UI_COMPILE_DEFAULT_NEW != 0) && (BR_UI_COMPILE_DEFAULT_NEW != 1)
#error "BR_UI_COMPILE_DEFAULT_NEW must be 0 or 1."
#endif

#if (BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX != 0) && (BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX != 1)
#error "BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX must be 0 or 1."
#endif

#if !BR_UI_COMPILE_ALLOW_LEGACY && !BR_UI_COMPILE_ALLOW_NEW
#error "At least one UI migration path must be compiled (legacy or new)."
#endif

#if BR_UI_COMPILE_DEFAULT_NEW && !BR_UI_COMPILE_ALLOW_NEW
#error "BR_UI_COMPILE_DEFAULT_NEW=1 requires BR_UI_COMPILE_ALLOW_NEW=1."
#endif

enum UiMigrationPath {
    UI_MIGRATION_PATH_LEGACY = 0,
    UI_MIGRATION_PATH_NEW = 1
};

constexpr bool kUiCompileAllowLegacy = BR_UI_COMPILE_ALLOW_LEGACY != 0;
constexpr bool kUiCompileAllowNew = BR_UI_COMPILE_ALLOW_NEW != 0;
constexpr bool kUiCompileDefaultNew = BR_UI_COMPILE_DEFAULT_NEW != 0;
constexpr bool kUiCompileAllowT12pContextualFx = BR_UI_COMPILE_ALLOW_T12P_CONTEXTUAL_FX != 0;

inline char uiAsciiLower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

inline bool uiEqualsIgnoreCase(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (uiAsciiLower(*a) != uiAsciiLower(*b)) return false;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

inline UiMigrationPath uiDefaultMigrationPath() {
    if (kUiCompileDefaultNew && kUiCompileAllowNew) return UI_MIGRATION_PATH_NEW;
    if (kUiCompileAllowLegacy) return UI_MIGRATION_PATH_LEGACY;
    if (kUiCompileAllowNew) return UI_MIGRATION_PATH_NEW;
    return UI_MIGRATION_PATH_LEGACY;
}

inline UiMigrationPath resolveUiMigrationPath() {
#if BR_UI_COMPILE_ALLOW_LEGACY && BR_UI_COMPILE_ALLOW_NEW
    UiMigrationPath path = uiDefaultMigrationPath();
    const char* env = std::getenv("BR_UI_PATH");
    if (env) {
        if (uiEqualsIgnoreCase(env, "legacy")) path = UI_MIGRATION_PATH_LEGACY;
        else if (uiEqualsIgnoreCase(env, "new")) path = UI_MIGRATION_PATH_NEW;
        else if (uiEqualsIgnoreCase(env, "auto")) path = uiDefaultMigrationPath();
    }
    return path;
#elif BR_UI_COMPILE_ALLOW_NEW
    return UI_MIGRATION_PATH_NEW;
#else
    return UI_MIGRATION_PATH_LEGACY;
#endif
}

inline bool useNewUiMigrationPath() {
    return resolveUiMigrationPath() == UI_MIGRATION_PATH_NEW;
}

inline bool useT12pContextualFeedbackSpike() {
    if (!kUiCompileAllowT12pContextualFx) return false;
    if (!useNewUiMigrationPath()) return false;
    const char* env = std::getenv("BR_UI_T12P_CONTEXTUAL_FX");
    if (!env) return false;
    return uiEqualsIgnoreCase(env, "1") || uiEqualsIgnoreCase(env, "true") || uiEqualsIgnoreCase(env, "on");
}

inline const char* currentUiMigrationPathName() {
    return useNewUiMigrationPath() ? "new" : "legacy";
}
