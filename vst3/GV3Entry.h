#pragma once

// ============================================================================
// GlobalVST3Engine - VST3 Plugin Entry Point Helpers
// ============================================================================
//
// Usage example — create a file (e.g. MyEQPlugin.cpp) in your plugin project:
//
//   #include "vst3/GV3Entry.h"
//
//   static const Steinberg::FUID kProcessorUID (0x01234567, 0x89ABCDEF, 0x01234567, 0x89ABCDEF);
//   static const Steinberg::FUID kControllerUID(0xFEDCBA98, 0x76543210, 0xFEDCBA98, 0x76543210);
//
//   static auto createEngine()
//   {
//       return gv3::PluginFactory::createProcessor(gv3::PluginKind::Equalizer);
//   }
//
//   GV3_DEFINE_PROCESSOR(MyEQProcessor, createEngine, kControllerUID)
//   GV3_DEFINE_CONTROLLER(MyEQController, createEngine)
//
//   BEGIN_FACTORY_DEF("MyCompany", "https://mycompany.com", "mailto:info@mycompany.com")
//     GV3_EXPORT_PROCESSOR(MyEQProcessor, kProcessorUID, "My EQ", "Fx|EQ", "1.0.0")
//     GV3_EXPORT_CONTROLLER(MyEQController, kControllerUID, "My EQ Controller", "1.0.0")
//   END_FACTORY
//
// Link against: GlobalVST3Engine, GV3Adapter, and the VST3 SDK targets.
// On Windows, also link the platform entry point (public.sdk/source/main/dllmain.cpp).
// ============================================================================

#include "vst3/GV3ProcessorBase.h"
#include "vst3/GV3ControllerBase.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"

// ---------------------------------------------------------------------------
// GV3_DEFINE_PROCESSOR  — generates a concrete AudioEffect class
// ---------------------------------------------------------------------------
//   engineFactory : callable returning std::unique_ptr<gv3::PluginProcessor>
//   controllerUID : Steinberg::FUID of the matching controller class
// ---------------------------------------------------------------------------
#define GV3_DEFINE_PROCESSOR(ClassName, engineFactory, controllerUID) \
    class ClassName final : public gv3::vst3::GV3ProcessorBase \
    { \
    public: \
        ClassName() : GV3ProcessorBase(engineFactory) \
        { \
            setControllerClass(controllerUID); \
        } \
        static Steinberg::FUnknown* createInstance(void*) \
        { \
            return static_cast<Steinberg::Vst::IAudioProcessor*>(new ClassName()); \
        } \
    };

// ---------------------------------------------------------------------------
// GV3_DEFINE_CONTROLLER — generates a concrete EditController class
// ---------------------------------------------------------------------------
//   engineFactory : same callable as the processor (used to mirror parameters)
// ---------------------------------------------------------------------------
#define GV3_DEFINE_CONTROLLER(ClassName, engineFactory) \
    class ClassName final : public gv3::vst3::GV3ControllerBase \
    { \
    public: \
        ClassName() : GV3ControllerBase(engineFactory) {} \
        static Steinberg::FUnknown* createInstance(void*) \
        { \
            return static_cast<Steinberg::Vst::IEditController*>(new ClassName()); \
        } \
    };

// ---------------------------------------------------------------------------
// GV3_EXPORT_PROCESSOR / GV3_EXPORT_CONTROLLER — register in plugin factory
// ---------------------------------------------------------------------------
#define GV3_EXPORT_PROCESSOR(ClassName, uid, name, category, version) \
    DEF_CLASS2( \
        INLINE_UID_FROM_FUID(uid), \
        Steinberg::PClassInfo::kManyInstances, \
        kVstAudioEffectClass, \
        name, \
        Steinberg::Vst::kDistributable, \
        category, \
        version, \
        kVstVersionString, \
        ClassName::createInstance)

#define GV3_EXPORT_CONTROLLER(ClassName, uid, name, version) \
    DEF_CLASS2( \
        INLINE_UID_FROM_FUID(uid), \
        Steinberg::PClassInfo::kManyInstances, \
        kVstComponentControllerClass, \
        name, \
        0, \
        "", \
        version, \
        kVstVersionString, \
        ClassName::createInstance)
