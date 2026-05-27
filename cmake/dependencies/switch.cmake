#=================== Nintendo Switch (devkitPro / libnx) ===================
# Uses devkitPro's portlibs SDL2 + glad. devkitPro toolchain already adds
# ${DEVKITPRO}/portlibs/switch/include to the compiler include path, so we
# don't need to wire it on the libultraship target here.

find_package(SDL2 REQUIRED)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

# imgui_impl_opengl3 should not try to load GL via glew/gl3w on Switch — glad is used.
# CUSTOM tells ImGui "you provide GL". Force-include glad in every ImGui TU so
# GLuint / GL_COLOR_BUFFER_BIT / glClear etc. are defined.
# The shim header (which itself includes glad) additionally neutralises
# glPolygonMode (desktop-only, null in glad-libnx) and glBindSampler (Mesa
# NVC0 driver bug) - see the header for why.
target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
target_compile_options(ImGui PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies/switch_imgui_shim.h
)

# Switch (libnx) has no fork/exec/waitpid, so disable ImGui's default
# Platform_OpenInShellFn implementation that pulls them in.
target_compile_definitions(ImGui PUBLIC IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS)

# devkitPro doesn't ship libzip / spdlog / nlohmann_json for Switch, so fetch them.
# tinyxml2 IS in devkitPro (switch-tinyxml2 6.0.0) but it's too old for torch's
# InsertNewChildElement (added in 7.1.0), so fetch a modern one too.
# src/CMakeLists.txt has matching guards that skip find_package when these targets
# are already created on Switch.
include(FetchContent)

#--- tinyxml2 (newer than devkitPro's 6.0.0) ------------------------------------
set(tinyxml2_BUILD_TESTING OFF CACHE INTERNAL "")
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 10.0.0
)
FetchContent_MakeAvailable(tinyxml2)
if(TARGET tinyxml2 AND NOT TARGET tinyxml2::tinyxml2)
    add_library(tinyxml2::tinyxml2 ALIAS tinyxml2)
endif()

#--- nlohmann_json (header-only) -------------------------------------------------
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

#--- spdlog ----------------------------------------------------------------------
set(SPDLOG_BUILD_EXAMPLE OFF CACHE INTERNAL "")
set(SPDLOG_BUILD_TESTS OFF CACHE INTERNAL "")
set(SPDLOG_INSTALL OFF CACHE INTERNAL "")
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)
# devkitPro / newlib hides POSIX symbols (fileno, isatty, fsync) unless a
# feature test macro is set. spdlog needs them in os-inl.h.
target_compile_definitions(spdlog PUBLIC _POSIX_C_SOURCE=200809L _DEFAULT_SOURCE)

#--- libzip ---------------------------------------------------------------------
# Pulls in zlib + bzip2 from devkitPro portlibs (switch-zlib, switch-bzip2).
# Disable everything we don't need to keep the cross-build small.
set(BUILD_TOOLS OFF CACHE INTERNAL "")
set(BUILD_REGRESS OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_DOC OFF CACHE INTERNAL "")
set(BUILD_OSSFUZZ OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(LIBZIP_DO_INSTALL OFF CACHE INTERNAL "")
set(ENABLE_LZMA OFF CACHE INTERNAL "")
set(ENABLE_ZSTD OFF CACHE INTERNAL "")
set(ENABLE_OPENSSL OFF CACHE INTERNAL "")
set(ENABLE_GNUTLS OFF CACHE INTERNAL "")
set(ENABLE_MBEDTLS OFF CACHE INTERNAL "")
set(ENABLE_WINDOWS_CRYPTO OFF CACHE INTERNAL "")
FetchContent_Declare(
    libzip
    GIT_REPOSITORY https://github.com/nih-at/libzip.git
    GIT_TAG v1.10.1
)
FetchContent_MakeAvailable(libzip)
# libzip 1.10 exports the target as `zip`. Provide an alias for the find_package name.
if(TARGET zip AND NOT TARGET libzip::zip)
    add_library(libzip::zip ALIAS zip)
endif()
