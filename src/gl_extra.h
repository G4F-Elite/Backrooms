#pragma once
// The engine ships a hand-trimmed glad with only the calls it uses. The demo needs a few
// more; all of them are OpenGL 1.1 and exported by opengl32.dll itself, so they are
// declared here the same way the engine's own tools do it (see dev_selftest.h there).

extern "C" {
__declspec(dllimport) void __stdcall glReadPixels(int x, int y, int w, int h, unsigned int format, unsigned int type, void* data);
__declspec(dllimport) void __stdcall glReadBuffer(unsigned int mode);
__declspec(dllimport) void __stdcall glPixelStorei(unsigned int name, int value);
__declspec(dllimport) void __stdcall glDepthMask(unsigned char flag);
__declspec(dllimport) void __stdcall glPolygonOffset(float factor, float units);
__declspec(dllimport) void __stdcall glFinish(void);
}

#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
