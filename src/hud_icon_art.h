#pragma once
// Draws the HUD pictograms into an alpha atlas with a handful of shape primitives.
// CPU only (no GL), so tools and tests can render the sheet too. Icon space is 32 x 32,
// origin top-left, y down.

#include <cmath>
#include <vector>
#include "hud_icon_ids.h"

struct IconCanvas {
    float a[HUD_ICON_PX * HUD_ICON_PX] = {};
    float ink = 1.0f; // 1 = draw, 0 = erase

    template <typename Inside>
    void plot(Inside inside) {
        for (int y = 0; y < HUD_ICON_PX; y++) {
            for (int x = 0; x < HUD_ICON_PX; x++) {
                int hits = 0;
                for (int s = 0; s < 4; s++) {
                    float px = (float)x + ((s & 1) ? 0.75f : 0.25f);
                    float py = (float)y + ((s & 2) ? 0.75f : 0.25f);
                    if (inside(px, py)) hits++;
                }
                if (hits == 0) continue;
                float cover = (float)hits * 0.25f;
                // Additive, so two shapes sharing an edge leave no seam between them.
                float& t = a[y * HUD_ICON_PX + x];
                t = ink > 0.5f ? t + cover : t * (1.0f - cover);
                if (t > 1.0f) t = 1.0f;
            }
        }
    }

    void rect(float x0, float y0, float x1, float y1) {
        plot([=](float x, float y) { return x >= x0 && x <= x1 && y >= y0 && y <= y1; });
    }
    void oval(float cx, float cy, float rx, float ry) {
        plot([=](float x, float y) {
            float dx = (x - cx) / rx, dy = (y - cy) / ry;
            return dx * dx + dy * dy <= 1.0f;
        });
    }
    void disc(float cx, float cy, float r) { oval(cx, cy, r, r); }
    void ring(float cx, float cy, float r, float th) {
        plot([=](float x, float y) {
            float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
            return d <= r && d >= r - th;
        });
    }
    void line(float x0, float y0, float x1, float y1, float th) {
        float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy, h = th * 0.5f;
        plot([=](float x, float y) {
            float t = len2 > 0.0f ? ((x - x0) * dx + (y - y0) * dy) / len2 : 0.0f;
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            float ex = x - (x0 + dx * t), ey = y - (y0 + dy * t);
            return ex * ex + ey * ey <= h * h;
        });
    }
    void tri(float x0, float y0, float x1, float y1, float x2, float y2) {
        plot([=](float x, float y) {
            float d0 = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
            float d1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
            float d2 = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);
            return (d0 >= 0 && d1 >= 0 && d2 >= 0) || (d0 <= 0 && d1 <= 0 && d2 <= 0);
        });
    }
    void frame(float x0, float y0, float x1, float y1, float th) {
        rect(x0, y0, x1, y0 + th);
        rect(x0, y1 - th, x1, y1);
        rect(x0, y0, x0 + th, y1);
        rect(x1 - th, y0, x1, y1);
    }
    void erase() { ink = 0.0f; }
    void draw() { ink = 1.0f; }
};

inline void iconSpeaker(IconCanvas& c) {
    c.rect(3, 12, 9, 20);
    c.tri(9, 12, 17, 5, 17, 27);
    c.tri(9, 12, 9, 20, 17, 27);
}

inline void iconPersonAt(IconCanvas& c, float cx, float s) {
    c.disc(cx, 4.0f + 5.0f * s, 4.6f * s);
    c.tri(cx - 8.0f * s, 29.0f, cx + 8.0f * s, 29.0f, cx + 4.0f * s, 4.0f + 11.0f * s);
    c.tri(cx - 8.0f * s, 29.0f, cx - 4.0f * s, 4.0f + 11.0f * s, cx + 4.0f * s, 4.0f + 11.0f * s);
}

inline void iconHeart(IconCanvas& c) {
    c.disc(10.5f, 12, 6.5f);
    c.disc(21.5f, 12, 6.5f);
    c.tri(4.6f, 15, 27.4f, 15, 16, 28.5f);
}

inline void paintHudIconA(IconCanvas& c, int icon) {
    if (icon == HUD_ICON_LOOT) {
        c.frame(4, 11, 28, 28, 3);
        c.line(4, 12, 16, 5, 3); c.line(16, 5, 28, 12, 3);
        c.rect(14, 11, 18, 19);
    } else if (icon == HUD_ICON_COIN) {
        c.ring(16, 16, 14, 3);
        c.rect(15, 6, 17, 26);
        c.rect(11, 9, 21, 11.5f); c.rect(11, 14.8f, 21, 17.2f); c.rect(11, 20.5f, 21, 23);
        c.rect(11, 9, 13.5f, 17); c.rect(18.5f, 15, 21, 23);
    } else if (icon == HUD_ICON_DRILL) {
        c.rect(7, 3, 25, 9);
        c.tri(8, 10, 24, 10, 16, 30);
        c.erase();
        c.line(7, 16, 25, 12, 2); c.line(9, 22, 23, 18, 2);
    } else if (icon == HUD_ICON_PORTAL) {
        c.rect(6, 4, 10, 29); c.rect(22, 4, 26, 29); c.rect(6, 4, 26, 8);
        c.ring(16, 19, 5, 2); c.disc(16, 19, 1.4f);
    } else if (icon == HUD_ICON_HAND) {
        c.disc(17, 21, 7.5f); c.rect(10, 14, 24, 21);
        c.rect(10, 7, 12.8f, 16); c.rect(13.8f, 4, 16.6f, 16); c.rect(17.6f, 4, 20.4f, 16); c.rect(21.4f, 7, 24, 16);
        c.line(10, 22, 5, 15, 3.4f);
    } else if (icon == HUD_ICON_PICKAXE) {
        c.line(7, 28, 22, 10, 3.2f);
        c.line(13, 6, 27, 14, 3.4f); c.line(13, 6, 8, 9, 3); c.line(27, 14, 28, 20, 3);
    } else if (icon == HUD_ICON_FLASHLIGHT) {
        c.rect(3, 13, 15, 19);
        c.tri(15, 11, 21, 8, 21, 24); c.tri(15, 11, 15, 21, 21, 24);
        c.line(24, 10, 29, 6, 2); c.line(24, 16, 30, 16, 2); c.line(24, 22, 29, 26, 2);
    } else if (icon == HUD_ICON_SCANNER) {
        c.ring(16, 16, 14, 2.6f); c.ring(16, 16, 8, 2); c.disc(16, 16, 2.6f);
        c.line(16, 16, 25, 7, 2.2f);
    } else if (icon == HUD_ICON_BATTERY) {
        c.frame(9, 8, 23, 29, 3); c.rect(13, 4, 19, 8); c.rect(12.5f, 17, 19.5f, 25.5f);
    } else if (icon == HUD_ICON_MED) {
        c.disc(16, 16, 14);
        c.erase();
        c.rect(13, 7, 19, 25); c.rect(7, 13, 25, 19);
    } else if (icon == HUD_ICON_PLUSH) {
        c.disc(8, 8, 5); c.disc(24, 8, 5); c.disc(16, 18, 11);
        c.erase();
        c.disc(12, 16, 1.9f); c.disc(20, 16, 1.9f); c.oval(16, 22, 3.2f, 2.2f);
    } else if (icon == HUD_ICON_EXTRACT) {
        c.tri(16, 2, 6, 14, 26, 14); c.rect(13, 14, 19, 22); c.rect(7, 25, 25, 29);
    } else if (icon == HUD_ICON_SKULL) {
        c.disc(16, 13, 11); c.rect(10, 18, 22, 28);
        c.erase();
        c.disc(11.5f, 13.5f, 3.2f); c.disc(20.5f, 13.5f, 3.2f); c.tri(16, 17, 14, 21, 18, 21);
        c.rect(13.2f, 24, 14.4f, 28); c.rect(17.6f, 24, 18.8f, 28);
    } else if (icon == HUD_ICON_EYE) {
        c.oval(16, 16, 14.5f, 9);
        c.erase(); c.oval(16, 16, 11.5f, 6.2f);
        c.draw(); c.disc(16, 16, 4.6f);
    } else if (icon == HUD_ICON_NOISE) {
        c.ring(15, 16, 12, 2.2f); c.ring(15, 16, 16, 2.2f);
        c.erase(); c.rect(0, 0, 20, 32);
        c.draw(); iconSpeaker(c);
    } else if (icon == HUD_ICON_MUTE) {
        iconSpeaker(c);
        c.line(20, 11, 29, 21, 2.8f); c.line(29, 11, 20, 21, 2.8f);
    } else if (icon == HUD_ICON_PERSON) {
        iconPersonAt(c, 16, 1.0f);
    } else if (icon == HUD_ICON_GROUP) {
        iconPersonAt(c, 9.5f, 0.78f); iconPersonAt(c, 22.5f, 0.78f);
    } else if (icon == HUD_ICON_ARROW_DOWN) {
        c.rect(13, 3, 19, 17); c.tri(16, 29, 6, 16, 26, 16);
    }
}

inline void paintHudIconB(IconCanvas& c, int icon) {
    if (icon == HUD_ICON_WEIGHT) {
        c.ring(16, 8, 5, 2.4f);
        c.tri(5, 28, 27, 28, 22, 12); c.tri(5, 28, 10, 12, 22, 12);
    } else if (icon == HUD_ICON_SHOP) {
        c.rect(3, 5, 29, 11);
        for (int i = 0; i < 4; i++) c.disc(6.5f + (float)i * 6.4f, 11.5f, 3.1f);
        c.rect(5, 12, 7, 28); c.rect(25, 12, 27, 28); c.rect(5, 20, 27, 28);
    } else if (icon == HUD_ICON_WARNING) {
        c.tri(16, 2, 1, 29, 31, 29);
        c.erase(); c.tri(16, 9, 7, 26, 25, 26);
        c.draw(); c.rect(14.8f, 13, 17.2f, 21); c.disc(16, 23.6f, 1.5f);
    } else if (icon == HUD_ICON_SEARCH) {
        c.ring(13, 13, 9, 3); c.line(19.5f, 19.5f, 28, 28, 4);
    } else if (icon == HUD_ICON_HEART) {
        iconHeart(c);
    } else if (icon == HUD_ICON_SANITY) {
        for (int i = 0; i <= 60; i++) {
            float t = (float)i / 60.0f, ang = t * 6.2832f * 2.4f, r = 1.5f + t * 11.5f;
            c.disc(16.0f + cosf(ang) * r, 16.0f + sinf(ang) * r, 1.5f);
        }
    } else if (icon == HUD_ICON_STAMINA) {
        c.tri(19, 2, 6, 18, 16, 18); c.tri(13, 30, 26, 14, 16, 14);
    } else if (icon == HUD_ICON_PALLET) {
        c.rect(13, 2, 19, 11); c.tri(16, 21, 8, 10, 24, 10);
        c.rect(3, 24, 29, 27); c.rect(5, 27, 9, 30); c.rect(23, 27, 27, 30);
    } else if (icon == HUD_ICON_CHECK) {
        c.line(5, 17, 13, 25, 4.4f); c.line(13, 25, 27, 8, 4.4f);
    } else if (icon == HUD_ICON_CROSS) {
        c.line(7, 7, 25, 25, 4.4f); c.line(25, 7, 7, 25, 4.4f);
    } else if (icon == HUD_ICON_REVIVE) {
        iconHeart(c);
        c.erase(); c.rect(14.4f, 9, 17.6f, 22); c.rect(9.5f, 13.9f, 22.5f, 17.1f);
    } else if (icon == HUD_ICON_PING) {
        c.disc(16, 12, 9); c.tri(8.5f, 16, 23.5f, 16, 16, 30);
        c.erase(); c.disc(16, 12, 3.6f);
    } else if (icon == HUD_ICON_CORE) {
        c.tri(16, 1, 11, 16, 21, 16); c.tri(16, 31, 11, 16, 21, 16);
        c.tri(1, 16, 16, 11, 16, 21); c.tri(31, 16, 16, 11, 16, 21);
        c.erase(); c.disc(16, 16, 2.6f);
    } else if (icon == HUD_ICON_DOWNED) {
        c.disc(7, 21, 4.4f); c.rect(12, 17.5f, 29, 25); c.rect(3, 27, 29, 29);
    } else if (icon == HUD_ICON_GHOST) {
        c.disc(16, 13, 10.5f); c.rect(5.5f, 13, 26.5f, 27);
        c.erase();
        c.disc(9, 28, 3); c.disc(16, 28, 3); c.disc(23, 28, 3); c.disc(12, 13, 2.3f); c.disc(20, 13, 2.3f);
    } else if (icon == HUD_ICON_CLOCK) {
        c.ring(16, 16, 14, 2.8f); c.rect(15, 7, 17, 17); c.line(16, 16, 22, 20, 2.2f);
    } else if (icon == HUD_ICON_STASH) {
        c.disc(16, 20, 10); c.tri(16, 12, 10, 3, 22, 3);
        c.erase(); c.line(11, 11.5f, 21, 11.5f, 1.6f);
    } else if (icon == HUD_ICON_ARROW_RIGHT) {
        c.rect(3, 13, 17, 19); c.tri(29, 16, 16, 6, 16, 26);
    } else if (icon == HUD_ICON_CRACK) {
        c.frame(3, 3, 29, 29, 2.6f);
        c.line(14, 3, 18, 11, 2.4f); c.line(18, 11, 12, 17, 2.4f); c.line(12, 17, 19, 23, 2.4f);
        c.line(19, 23, 15, 29, 2.4f); c.line(18, 11, 25, 13, 2); c.line(12, 17, 6, 20, 2);
    } else if (icon == HUD_ICON_WRENCH) {
        c.line(6, 26, 19, 13, 5); c.disc(22, 10, 7.5f);
        c.erase(); c.line(22.5f, 9.5f, 30, 2, 5);
    } else if (icon == HUD_ICON_HOME) {
        c.tri(16, 2, 2, 15, 30, 15); c.rect(6, 15, 26, 29);
        c.erase(); c.rect(13, 19, 19, 29);
    }
}

// One byte of coverage per pixel, HUD_ICON_COLS x HUD_ICON_ROWS cells, row 0 at the top.
inline void buildHudIconAtlas(std::vector<unsigned char>& out, int& w, int& h) {
    w = HUD_ICON_PX * HUD_ICON_COLS;
    h = HUD_ICON_PX * HUD_ICON_ROWS;
    out.assign((size_t)w * h, 0);
    for (int icon = 0; icon < HUD_ICON_COUNT; icon++) {
        IconCanvas c;
        paintHudIconA(c, icon);
        paintHudIconB(c, icon);
        int ox = (icon % HUD_ICON_COLS) * HUD_ICON_PX, oy = (icon / HUD_ICON_COLS) * HUD_ICON_PX;
        for (int y = 1; y < HUD_ICON_PX - 1; y++) {
            for (int x = 1; x < HUD_ICON_PX - 1; x++) {
                float v = c.a[y * HUD_ICON_PX + x];
                out[(size_t)(oy + y) * w + ox + x] = (unsigned char)(v < 0.0f ? 0 : (v > 1.0f ? 255 : (int)(v * 255.0f)));
            }
        }
    }
}
