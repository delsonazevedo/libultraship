#=================== Nintendo Switch (devkitPro / libnx) ===================
# Uses devkitPro's portlibs SDL2 + glad. Game already has SDL2 found at top-level.

find_package(SDL2 REQUIRED)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

# imgui_impl_opengl3 should not try to load GL via glew/gl3w on Switch — glad is used.
target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)

# Make sure libultraship sees the devkitPro portlibs include path too.
target_include_directories(libultraship PRIVATE ${DEVKITPRO}/portlibs/switch/include/)
