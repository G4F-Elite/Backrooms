#include "../src/narrator_subtitle_layout.h"
#include <cassert>
#include <cmath>

static void testWordWrapping() {
    using Rows = std::vector<std::string>;
    assert(wrapNarratorSubtitle("", 40).empty());
    assert(wrapNarratorSubtitle(" \t\r\n", 40).empty());
    assert(wrapNarratorSubtitle("abc", 0).empty());
    assert(wrapNarratorSubtitle(" Keep it quiet.  ", 8) == (Rows{"Keep it", "quiet."}));
    assert(wrapNarratorSubtitle("abc\r\ndef\nxyz", 4) == (Rows{"abc", "def", "xyz"}));
    assert(wrapNarratorSubtitle("012345678", 3) == (Rows{"012", "345", "678"}));
    assert(wrapNarratorSubtitle("I\tsee it.", 6) == (Rows{"I see", "it."}));
    assert(wrapNarratorSubtitle("aa bb", 2) == (Rows{"aa", "bb"}));
}

static void testPlacement() {
    auto plate = layoutNarratorSubtitle("Hello.", 640, 360, 12, 16, false);
    assert(plate.top < -0.54f * 360 && plate.bottom > -0.85f * 360);
    assert(plate.width >= 76 && plate.lineHeight >= 16);
    auto upper = layoutNarratorSubtitle("Hello.", 640, 360, 12, 16, true);
    assert(upper.top > 0 && upper.bottom > 0);
    assert(layoutNarratorSubtitle("Hello.", 0, 360, 12, 16, true).lines.empty());
    assert(layoutNarratorSubtitle(" ", 640, 360, 12, 16, true).lines.empty());
}

static void testBankFitsViewports() {
    const char* clips[] = {
#define DISPATCH_LINE(event, id, words) words,
#include "../src/dispatcher_lines.inc"
#undef DISPATCH_LINE
    };
    struct Viewport { int w, h; };
    for (auto viewport : {Viewport{640, 360}, {1024, 768}, {1280, 720}, {3840, 2160}, {3440, 1440}}) {
        float scale = (std::max)(0.5f, (std::min)(viewport.w / 1280.f, viewport.h / 720.f));
        float cell = std::round((std::max)(11.f, 16.f * scale));
        float advance = std::round(cell * 0.75f) / scale;
        float halfW = viewport.w / (2 * scale), halfH = viewport.h / (2 * scale);
        for (const char* clip : clips) {
            auto plate = layoutNarratorSubtitle(clip, halfW, halfH, advance, cell / scale, false);
            assert(!plate.lines.empty() && plate.width <= halfW * 0.94f);
            assert(plate.bottom > -halfH && plate.top < halfH);
            assert(plate.top < -halfH * 0.54f || plate.bottom > 0);
            std::string restored;
            for (const auto& line : plate.lines) {
                assert(line.size() * advance + 2 * NARRATOR_PAD_X <= plate.width + 0.01f);
                if (!restored.empty()) restored.push_back(' ');
                restored += line;
            }
            assert(restored == clip);
        }
    }
}

int main() {
    testWordWrapping();
    testPlacement();
    testBankFitsViewports();
}
