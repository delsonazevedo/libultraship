#ifdef __SWITCH__
#include "SwitchImpl.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include "SwitchPerformanceProfiles.h"
#include "public/bridge/consolevariablebridge.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "audio/Audio.h"

#include <imgui_internal.h>

#define DOCKED_MODE 1
#define HANDHELD_MODE 0

static AppletHookCookie applet_hook_cookie;
static bool isRunning = true;
static bool hasFocus = true;
static bool isShowingVirtualKeyboard = true;

void DetectAppletMode();

static void on_applet_hook(AppletHookType hook, void* param);

void Ship::Switch::Init(SwitchPhase phase) {
    switch (phase) {
        case PreInitPhase:
            DetectAppletMode();
            break;
        case PostInitPhase:
            appletInitializeGamePlayRecording();
#ifdef DEBUG
            socketInitializeDefault();
            nxlinkStdio();
#endif
            appletSetGamePlayRecordingState(true);
            appletHook(&applet_hook_cookie, on_applet_hook, NULL);
            appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);
            if (!hosversionBefore(8, 0, 0)) {
                clkrstInitialize();
            }
            break;
    }
}

void Ship::Switch::Exit() {
#ifdef DEBUG
    socketExit();
#endif
    clkrstExit();
    appletSetGamePlayRecordingState(false);
}

void Ship::Switch::ImGuiSetupFont(ImFontAtlas* fonts) {
    plInitialize(PlServiceType_User);
    static PlFontData stdFontData, extFontData;

    PlFontData fonts_std;
    PlFontData fonts_ext;

    plGetSharedFontByType(&fonts_std, PlSharedFontType_Standard);
    plGetSharedFontByType(&fonts_ext, PlSharedFontType_NintendoExt);

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;

    strcpy(config.Name, "Nintendo Standard");
    fonts->AddFontFromMemoryTTF(fonts_std.address, fonts_std.size, 24.0f, &config, fonts->GetGlyphRangesCyrillic());

    strcpy(config.Name, "Nintendo Ext");
    static const ImWchar ranges[] = {
        0xE000, 0xE06B, 0xE070, 0xE07E, 0xE080, 0xE099, 0xE0A0, 0xE0BA, 0xE0C0, 0xE0D6, 0xE0E0, 0xE0F5, 0xE100,
        0xE105, 0xE110, 0xE116, 0xE121, 0xE12C, 0xE130, 0xE13C, 0xE140, 0xE14D, 0xE150, 0xE153, 0,
    };

    fonts->AddFontFromMemoryTTF(fonts_ext.address, fonts_ext.size, 24.0f, &config, ranges);
    fonts->Build();

    plExit();
}

void Ship::Switch::ImGuiProcessEvent(bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());

    if (wantsTextInput) {
        if (!isShowingVirtualKeyboard) {
            state->ClearText();

            isShowingVirtualKeyboard = true;
            SDL_StartTextInput();
        }
    } else {
        if (isShowingVirtualKeyboard) {
            isShowingVirtualKeyboard = false;
            SDL_StopTextInput();
        }
    }
}

bool Ship::Switch::IsRunning() {
    return isRunning;
}

void Ship::Switch::GetDisplaySize(int* width, int* height) {
    switch (appletGetOperationMode()) {
        case DOCKED_MODE:
            *width = 1920;
            *height = 1080;
            break;
        case HANDHELD_MODE:
            *width = 1280;
            *height = 720;
            break;
    }
}

void Ship::Switch::ApplyOverclock(void) {
    // on_applet_hook (registered in PostInitPhase) can fire from inside
    // appletMainLoop before GameEngine has finished initialising the Context
    // — e.g. when we show PrintErrorMessageToScreen for a missing .o2r.
    // GameEngine::GameEngine() creates an *uninitialised* Context up front
    // (via CreateUninitializedInstance), so checking GetInstance() alone
    // isn't enough: the ConsoleVariables member is still null until
    // InitConsoleVariables runs, and CVarGetInteger would deref it.
    auto ctx = Ship::Context::GetInstance();
    if (ctx == nullptr || ctx->GetConsoleVariables() == nullptr) {
        return;
    }
    SwitchProfiles perfMode = (SwitchProfiles)CVarGetInteger("gSwitchPerfMode", (int)Ship::MAXIMUM);

    if (perfMode >= 0 && perfMode <= Ship::POWERSAVINGM3) {
        if (hosversionBefore(8, 0, 0)) {
            pcvSetClockRate(PcvModule_CpuBus, SWITCH_CPU_SPEEDS_VALUES[perfMode]);
        } else {
            ClkrstSession session = { 0 };
            clkrstOpenSession(&session, PcvModuleId_CpuBus, 3);
            clkrstSetClockRate(&session, SWITCH_CPU_SPEEDS_VALUES[perfMode]);
            clkrstCloseSession(&session);
        }
    }
}

void Ship::Switch::PrintErrorMessageToScreen(const char* str, ...) {
    consoleInit(NULL);
    srand(time(0));

    va_list args;
    va_start(args, str);
    vprintf(str, args);
    va_end(args);

    while (appletMainLoop()) {
        consoleUpdate(NULL);
    }

    consoleExit(NULL);
}

static void on_applet_hook(AppletHookType hook, void* param) {
    AppletFocusState focus_state;

    /* Exit request */
    switch (hook) {
        case AppletHookType_OnExitRequest:
            isRunning = false;
            break;

            /* Focus state*/
        case AppletHookType_OnFocusState:
            focus_state = appletGetFocusState();
            hasFocus = focus_state == AppletFocusState_InFocus;

            if (!hasFocus) {
                if (hosversionBefore(8, 0, 0)) {
                    pcvSetClockRate(PcvModule_CpuBus, SWITCH_CPU_SPEEDS_VALUES[Ship::STOCK]);
                } else {
                    ClkrstSession session = { 0 };
                    clkrstOpenSession(&session, PcvModuleId_CpuBus, 3);
                    clkrstSetClockRate(&session, SWITCH_CPU_SPEEDS_VALUES[Ship::STOCK]);
                    clkrstCloseSession(&session);
                }
            } else {
                Ship::Switch::ApplyOverclock();
                // Skip the audio-restart workaround until the audio subsystem
                // is wired up; on early bailout paths (missing .o2r) the
                // Context exists but GetAudio() is still null.
                auto ctx = Ship::Context::GetInstance();
                if (ctx != nullptr && ctx->GetAudio() != nullptr) {
                    SPDLOG_INFO("restarting SDL audio system to work around audio problems on resume");
                    ctx->GetAudio()->SetCurrentAudioBackend(Ship::AudioBackend::SDL);
                }
            }

            break;

            /* Performance mode */
        case AppletHookType_OnPerformanceMode:
            Ship::Switch::ApplyOverclock();
            break;
        default:
            break;
    }
}

void DetectAppletMode() {
    AppletType at = appletGetAppletType();
    if (at == AppletType_Application || at == AppletType_SystemApplication)
        return;

    Ship::Switch::PrintErrorMessageToScreen("\x1b[2;2HYou've launched Starship while in Applet mode."
                                            "\x1b[4;2HPlease relaunch while in full-memory mode."
                                            "\x1b[5;2HHold R when opening any game to enter HBMenu.");
}

void Ship::Switch::ThrowMissingOTR(std::string OTRPath) {
    Ship::Switch::PrintErrorMessageToScreen("\x1b[2;2HYou've launched Starship without the OTR file."
                                            "\x1b[4;2HPlease relaunch making sure %s exists.",
                                            OTRPath.c_str());
}
#endif
