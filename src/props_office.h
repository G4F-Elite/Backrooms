#pragma once
#include "decor_parts.h"

// ============================================================================
// OFFICE PROPS
// Break rooms, meeting rooms and copy corners. Themes: see prop_catalog.h.
// ============================================================================

inline void buildCopierModel(DecorBuilder& b) {
    const float f = 0.30f;
    b.box(0.0f, 0.00f, 0.0f, 0.66f, 0.06f, 0.56f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.06f, 0.0f, 0.70f, 0.84f, 0.60f, DECOR_MAT_WHITE);
    for (int i = 0; i < 2; i++) {
        float y = 0.12f + 0.22f * (float)i;
        b.box(0.0f, y, f + 0.003f, 0.60f, 0.18f, 0.006f, DECOR_MAT_METAL);
        b.box(0.0f, y + 0.12f, f + 0.009f, 0.18f, 0.02f, 0.006f, DECOR_MAT_DARK_METAL);
    }
    // scanner lid, control strip with a small display
    b.box(0.0f, 0.90f, -0.02f, 0.66f, 0.05f, 0.52f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.90f, 0.27f, 0.66f, 0.03f, 0.06f, DECOR_MAT_METAL);
    b.panel(0.18f, 0.93f, 0.27f, 0.16f, 0.006f, 0.05f, DECOR_MAT_SCREEN);
    // output tray and side feeder
    b.box(-0.40f, 0.55f, 0.0f, 0.10f, 0.02f, 0.36f, DECOR_MAT_WHITE);
    b.box(0.40f, 0.40f, 0.0f, 0.10f, 0.24f, 0.40f, DECOR_MAT_WHITE);
}

inline void buildMeetingTableModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.70f * (float)side;
        b.box(x, 0.00f, 0.0f, 0.10f, 0.04f, 0.80f, DECOR_MAT_DARK_METAL);
        b.box(x, 0.04f, 0.0f, 0.06f, 0.66f, 0.70f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.40f, 0.0f, 1.34f, 0.08f, 0.04f, DECOR_MAT_DARK_METAL);
    b.box(0.0f, 0.70f, 0.0f, 2.00f, 0.04f, 1.00f, DECOR_MAT_WOOD);
    b.box(0.10f, 0.74f, 0.05f, 0.22f, 0.04f, 0.22f, DECOR_MAT_DARK_METAL);
    DecorBuilder paper = b.sub(-0.50f, 0.10f, 0.3f);
    paper.panel(0.0f, 0.74f, 0.0f, 0.21f, 0.004f, 0.29f, DECOR_MAT_WHITE);
    DecorBuilder paper2 = b.sub(0.62f, -0.22f, -0.5f);
    paper2.panel(0.0f, 0.74f, 0.0f, 0.21f, 0.004f, 0.29f, DECOR_MAT_WHITE);
}

inline void buildSofaModel(DecorBuilder& b) {
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            b.box(0.82f * (float)sx, 0.00f, 0.32f * (float)sz, 0.06f, 0.08f, 0.06f, DECOR_MAT_TRIM);
        }
    }
    b.box(0.0f, 0.08f, 0.0f, 1.80f, 0.20f, 0.80f, DECOR_MAT_FABRIC);
    b.box(0.0f, 0.28f, -0.33f, 1.80f, 0.52f, 0.14f, DECOR_MAT_FABRIC);
    for (int side = -1; side <= 1; side += 2) {
        b.box(0.37f * (float)side, 0.28f, 0.06f, 0.72f, 0.14f, 0.64f, DECOR_MAT_FABRIC);
        b.box(0.82f * (float)side, 0.28f, 0.07f, 0.16f, 0.30f, 0.66f, DECOR_MAT_FABRIC);
    }
}

// Rolling whiteboard, usable from both sides.
inline void buildWhiteboardModel(DecorBuilder& b) {
    for (int side = -1; side <= 1; side += 2) {
        float x = 0.70f * (float)side;
        b.box(x, 0.00f, -0.20f, 0.05f, 0.06f, 0.06f, DECOR_MAT_RUBBER);
        b.box(x, 0.00f, 0.20f, 0.05f, 0.06f, 0.06f, DECOR_MAT_RUBBER);
        b.box(x, 0.06f, 0.0f, 0.05f, 0.04f, 0.50f, DECOR_MAT_METAL);
        b.box(x, 0.10f, 0.0f, 0.04f, 1.72f, 0.04f, DECOR_MAT_METAL);
    }
    b.box(0.0f, 0.82f, 0.0f, 1.36f, 1.00f, 0.03f, DECOR_MAT_METAL);
    b.panel(0.0f, 0.86f, 0.017f, 1.28f, 0.92f, 0.004f, DECOR_MAT_WHITE);
    b.panel(0.0f, 0.86f, -0.017f, 1.28f, 0.92f, 0.004f, DECOR_MAT_WHITE);
    b.box(0.0f, 0.82f, 0.04f, 1.00f, 0.02f, 0.05f, DECOR_MAT_METAL);
}

// Archive boxes and loose piles of paper.
inline void buildPaperStacksModel(DecorBuilder& root) {
    DecorBuilder b = root.sub(0.0f, 0.064f, 0.0f);
    b.box(-0.20f, 0.00f, -0.05f, 0.40f, 0.28f, 0.50f, DECOR_MAT_CARDBOARD);
    b.box(-0.20f, 0.28f, -0.05f, 0.42f, 0.04f, 0.52f, DECOR_MAT_CARDBOARD);
    DecorBuilder top = b.sub(-0.18f, -0.04f, 0.25f);
    top.box(0.0f, 0.32f, 0.0f, 0.38f, 0.26f, 0.46f, DECOR_MAT_CARDBOARD);
    top.box(0.0f, 0.58f, 0.0f, 0.21f, 0.05f, 0.29f, DECOR_MAT_WHITE);
    b.box(0.26f, 0.00f, 0.12f, 0.34f, 0.30f, 0.34f, DECOR_MAT_CARDBOARD);
    b.box(0.26f, 0.30f, 0.12f, 0.21f, 0.06f, 0.29f, DECOR_MAT_WHITE);
    DecorBuilder pile = b.sub(0.24f, -0.24f, 0.5f);
    pile.box(0.0f, 0.00f, 0.0f, 0.21f, 0.10f, 0.29f, DECOR_MAT_WHITE);
}
