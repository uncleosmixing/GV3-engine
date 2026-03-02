#pragma once

#include "GlobalVST3Engine.h"

#include "public.sdk/source/vst/vstaudioeffect.h"

#include <functional>

namespace gv3::vst3
{

class GV3ProcessorBase : public Steinberg::Vst::AudioEffect
{
public:
    using EngineFactory = std::function<std::unique_ptr<PluginProcessor>()>;

    explicit GV3ProcessorBase(EngineFactory factory);

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

protected:
    PluginProcessor& engine() noexcept { return *m_engine; }
    const PluginProcessor& engine() const noexcept { return *m_engine; }

private:
    EngineFactory m_factory;
    std::unique_ptr<PluginProcessor> m_engine;
};

} // namespace gv3::vst3
