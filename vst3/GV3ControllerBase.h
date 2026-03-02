#pragma once

#include "GlobalVST3Engine.h"

#include "public.sdk/source/vst/vsteditcontroller.h"

#include <functional>

namespace gv3::vst3
{

class GV3ControllerBase : public Steinberg::Vst::EditController
{
public:
    using EngineFactory = std::function<std::unique_ptr<PluginProcessor>()>;

    // Graphics callbacks for the editor view
    using InitGraphicsCallback = std::function<void*()>;            // returns NVGcontext*
    using ShutdownGraphicsCallback = std::function<void(void*)>;    // destroys NVGcontext*

    explicit GV3ControllerBase(EngineFactory factory);

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;

    NanoVGEditorModel buildEditorModel() const;

    void setEditorBridge(NanoVGEditorBridge bridge);
    const NanoVGEditorBridge& editorBridge() const noexcept;

    void setGraphicsCallbacks(InitGraphicsCallback init, ShutdownGraphicsCallback shutdown);
    const InitGraphicsCallback& graphicsInit() const noexcept;
    const ShutdownGraphicsCallback& graphicsShutdown() const noexcept;

protected:
    const PluginProcessor& referenceProcessor() const noexcept;

private:
    EngineFactory m_factory;
    std::unique_ptr<PluginProcessor> m_reference;
    NanoVGEditorBridge m_bridge;
    InitGraphicsCallback m_initGraphics;
    ShutdownGraphicsCallback m_shutdownGraphics;
};

} // namespace gv3::vst3
