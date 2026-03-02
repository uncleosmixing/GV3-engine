#include "GV3ControllerBase.h"
#include "GV3EditorView.h"

#include "pluginterfaces/base/ibstream.h"

#include <algorithm>
#include <cstring>

namespace gv3::vst3
{

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace
{
    void stringToTChar(const std::string& src, String128& dst)
    {
        const std::size_t maxLen = 127;
        const auto len = (src.size() < maxLen) ? src.size() : maxLen;
        for (std::size_t i = 0; i < len; ++i)
        {
            dst[i] = static_cast<char16>(src[i]);
        }
        dst[len] = 0;
    }
}

GV3ControllerBase::GV3ControllerBase(EngineFactory factory)
    : m_factory(std::move(factory))
{
}

tresult PLUGIN_API GV3ControllerBase::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    m_reference = m_factory();
    if (!m_reference)
    {
        return kResultFalse;
    }

    const auto& defs = m_reference->parameters().definitions();
    for (std::size_t i = 0; i < defs.size(); ++i)
    {
        const auto& def = defs[i];
        String128 title {};
        stringToTChar(def.name, title);

        auto normalizedDefault = m_reference->parameters().getNormalized(i);
        parameters.addParameter(
            title,
            nullptr,
            0,
            normalizedDefault,
            ParameterInfo::kCanAutomate,
            static_cast<ParamID>(i));
    }

    return kResultOk;
}

tresult PLUGIN_API GV3ControllerBase::terminate()
{
    m_reference.reset();
    return EditController::terminate();
}

tresult PLUGIN_API GV3ControllerBase::setComponentState(IBStream* state)
{
    if (!state || !m_reference)
    {
        return kResultFalse;
    }

    uint32 version = 0;
    int32 numBytesRead = 0;
    state->read(&version, sizeof(version), &numBytesRead);
    if (numBytesRead != sizeof(version) || version != 1)
    {
        return kResultFalse;
    }

    uint32 paramCount = 0;
    state->read(&paramCount, sizeof(paramCount), &numBytesRead);
    if (numBytesRead != sizeof(paramCount))
    {
        return kResultFalse;
    }

    auto engineParamCount = m_reference->parameters().count();
    for (uint32 i = 0; i < paramCount; ++i)
    {
        float value = 0.0f;
        state->read(&value, sizeof(value), &numBytesRead);
        if (numBytesRead != sizeof(value))
        {
            break;
        }

        if (i < engineParamCount)
        {
            m_reference->parameters().setByIndex(i, value);
            auto norm = m_reference->parameters().getNormalized(i);
            setParamNormalized(static_cast<ParamID>(i), norm);
        }
    }

    return kResultOk;
}

IPlugView* PLUGIN_API GV3ControllerBase::createView(FIDString name)
{
    if (std::strcmp(name, ViewType::kEditor) == 0 && m_bridge)
    {
        return new GV3EditorView(this);
    }

    return nullptr;
}

NanoVGEditorModel GV3ControllerBase::buildEditorModel() const
{
    if (!m_reference)
    {
        return NanoVGEditorModel(std::vector<NanoVGEditorModel::Knob> {});
    }

    const auto& defs = m_reference->parameters().definitions();
    std::vector<NanoVGEditorModel::Knob> knobs;
    knobs.reserve(defs.size());

    for (std::size_t i = 0; i < defs.size(); ++i)
    {
        const auto& def = defs[i];
        auto normalizedValue = static_cast<float>(const_cast<GV3ControllerBase*>(this)->getParamNormalized(static_cast<ParamID>(i)));
        knobs.push_back({
            .id = def.id,
            .label = def.name,
            .normalizedValue = normalizedValue,
            .actualValue = def.minValue + normalizedValue * (def.maxValue - def.minValue),
            .minValue = def.minValue,
            .maxValue = def.maxValue
        });
    }

    return NanoVGEditorModel(std::move(knobs));
}

void GV3ControllerBase::setEditorBridge(NanoVGEditorBridge bridge)
{
    m_bridge = std::move(bridge);
}

const NanoVGEditorBridge& GV3ControllerBase::editorBridge() const noexcept
{
    return m_bridge;
}

void GV3ControllerBase::setGraphicsCallbacks(InitGraphicsCallback init, ShutdownGraphicsCallback shutdown)
{
    m_initGraphics = std::move(init);
    m_shutdownGraphics = std::move(shutdown);
}

const GV3ControllerBase::InitGraphicsCallback& GV3ControllerBase::graphicsInit() const noexcept
{
    return m_initGraphics;
}

const GV3ControllerBase::ShutdownGraphicsCallback& GV3ControllerBase::graphicsShutdown() const noexcept
{
    return m_shutdownGraphics;
}

const PluginProcessor& GV3ControllerBase::referenceProcessor() const noexcept
{
    return *m_reference;
}

} // namespace gv3::vst3
