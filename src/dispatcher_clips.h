#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

struct DispatcherClipBytes {
    const unsigned char* data;
    std::size_t size;
};

// The Windows client ships as one executable. Keep the bank in read-only storage;
// the assembler reports a missing recording at build time.
#define DISPATCH_LINE(event, id, words) \
    extern "C" const unsigned char dispatcher_start_##id[], dispatcher_end_##id[]; \
    asm(".section .rdata, \"dr\"\n" \
        ".balign 2\n.globl dispatcher_start_" #id "\ndispatcher_start_" #id ":\n" \
        ".incbin \"assets/dispatcher/" #id ".opus\"\n" \
        ".globl dispatcher_end_" #id "\ndispatcher_end_" #id ":\n.text\n");
#include "dispatcher_lines.inc"
#undef DISPATCH_LINE

inline DispatcherClipBytes dispatcherEmbeddedClip(const char* clipId) {
    struct Record {
        const char* id;
        const unsigned char* begin;
        const unsigned char* end;
    };
    static const Record records[] = {
#define DISPATCH_LINE(event, id, words) {#id, dispatcher_start_##id, dispatcher_end_##id},
#include "dispatcher_lines.inc"
#undef DISPATCH_LINE
    };
    for (const Record& row : records) {
        if (std::strcmp(clipId, row.id) == 0) {
            return {row.begin, reinterpret_cast<std::uintptr_t>(row.end) -
                                   reinterpret_cast<std::uintptr_t>(row.begin)};
        }
    }
    return {nullptr, 0};
}
