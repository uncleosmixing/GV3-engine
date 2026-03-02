// GainEntry.cpp — VST3 factory entry point for the GV3 Gain plugin

#include "vst3/GV3Entry.h"
#include "plugins/GainPlugin/GainUI.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <glad/gl.h>
#include <nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

// ---- UIDs ------------------------------------------------------------------
// Generate your own for production use; these are for development.
static const Steinberg::FUID kGainProcessorUID (0x47563347, 0x41494E50, 0x524F4300, 0x00000001);
static const Steinberg::FUID kGainControllerUID(0x47563347, 0x41494E43, 0x54524C00, 0x00000001);

// ---- Engine factory --------------------------------------------------------
static auto createGainEngine()
{
    return gv3::PluginFactory::createProcessor(gv3::PluginKind::Gain);
}

static GLADapiproc loadGLProc(const char* name)
{
    auto proc = reinterpret_cast<GLADapiproc>(wglGetProcAddress(name));
    if (proc)
    {
        return proc;
    }

    static HMODULE opengl32 = GetModuleHandleW(L"opengl32.dll");
    return reinterpret_cast<GLADapiproc>(GetProcAddress(opengl32, name));
}

// ---- Processor (use convenience macro) -------------------------------------
GV3_DEFINE_PROCESSOR(GV3GainProcessor, createGainEngine, kGainControllerUID)

// ---- Controller (custom — sets up NanoVG editor bridge) --------------------
class GV3GainController final : public gv3::vst3::GV3ControllerBase
{
public:
    GV3GainController() : GV3ControllerBase(createGainEngine)
    {
        setEditorBridge(gv3::plugins::createGainEditorBridge());

        setGraphicsCallbacks(
            []() -> void* {
                if (!gladLoadGL(loadGLProc))
                    return nullptr;
                return nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
            },
            [](void* ctx) {
                if (ctx)
                    nvgDeleteGL3(static_cast<NVGcontext*>(ctx));
            });
    }

    static Steinberg::FUnknown* createInstance(void*)
    {
        return static_cast<Steinberg::Vst::IEditController*>(new GV3GainController());
    }
};

BEGIN_FACTORY_DEF("GV3 Audio", "https://github.com/gv3audio", "mailto:dev@gv3audio.com")

    GV3_EXPORT_PROCESSOR(GV3GainProcessor, kGainProcessorUID,
                          "GV3 Gain", "Fx", "1.0.0")

    GV3_EXPORT_CONTROLLER(GV3GainController, kGainControllerUID,
                           "GV3 Gain Controller", "1.0.0")

END_FACTORY
