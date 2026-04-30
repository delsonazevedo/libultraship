#pragma once
// Switch (devkitPro libnx + glad) workaround for ImGui's imgui_impl_opengl3.cpp.
//
// Problem:
//   glad's glPolygonMode function pointer is null on Switch (the function is
//   desktop-OpenGL-only and not exposed by libnx). ImGui's runtime detection
//   of GLES via glGetString(GL_VERSION) does not reliably classify the Switch
//   context as ES, so HasPolygonMode stays true and ImGui calls the null
//   pointer on the first frame.
//
// Fix:
//   This header is force-included into every ImGui translation unit on Switch
//   (see switch.cmake). It runs *after* glad/glad.h has #defined glPolygonMode
//   to glad_glPolygonMode and turns the call into a no-op so the null pointer
//   is never reached, regardless of HasPolygonMode.
#ifdef __SWITCH__
#include <glad/glad.h>
// glPolygonMode is desktop-only (not in GLES). glad-libnx leaves its function
// pointer null and ImGui's runtime feature detection doesn't reliably skip
// it, so we hard-stub it as a no-op.
#undef glPolygonMode
#define glPolygonMode(face, mode) ((void)0)
// glBindSampler triggers a Mesa NVC0 driver bug on Switch (crash in
// nvc0_sp_state_create deep inside st_program_string_notify) when called
// from ImGui's SetupRenderState. Stub it so ImGui doesn't reach the driver
// path. This means the font texture is sampled with whatever sampler
// state Fast3D last bound, which can produce visual issues but avoids the
// hard crash.
#undef glBindSampler
#define glBindSampler(unit, sampler) ((void)0)
#endif
