#=================== Nintendo Switch (devkitPro / libnx) ===================
# Uses devkitPro's portlibs SDL2 + glad. devkitPro toolchain already adds
# ${DEVKITPRO}/portlibs/switch/include to the compiler include path, so we
# don't need to wire it on the libultraship target here.

find_package(SDL2 REQUIRED)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

# imgui_impl_opengl3 should not try to load GL via glew/gl3w on Switch — glad is used.
target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
