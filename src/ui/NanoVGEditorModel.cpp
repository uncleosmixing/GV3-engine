#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"

namespace gv3
{
NanoVGEditorModel::NanoVGEditorModel(const PluginProcessor& processor)
{
    const auto& defs = processor.parameters().definitions();
    m_knobs.reserve(defs.size());

    for (const auto& def : defs)
    {
        const float value = processor.parameters().get(def.id);
        m_knobs.push_back({
            .id = def.id,
            .label = def.name,
            .normalizedValue = detail::normalized(value, def.minValue, def.maxValue),
            .actualValue = value,
            .minValue = def.minValue,
            .maxValue = def.maxValue
        });
    }
}

NanoVGEditorModel::NanoVGEditorModel(std::vector<Knob> knobs)
    : m_knobs(std::move(knobs))
{
}

const std::vector<NanoVGEditorModel::Knob>& NanoVGEditorModel::knobs() const noexcept
{
    return m_knobs;
}

NanoVGEditorBridge::NanoVGEditorBridge(DrawCallback drawCallback)
    : m_drawCallback(std::move(drawCallback))
{
}

void NanoVGEditorBridge::draw(void* nanovgContext, int width, int height, const PluginProcessor& processor) const
{
    const NanoVGEditorModel model(processor);
    draw(nanovgContext, width, height, model);
}

void NanoVGEditorBridge::draw(void* nanovgContext, int width, int height, const NanoVGEditorModel& model) const
{
    if (!m_drawCallback)
    {
        return;
    }

    m_drawCallback(nanovgContext, width, height, model);
}

NanoVGEditorBridge::operator bool() const noexcept
{
    return static_cast<bool>(m_drawCallback);
}
} // namespace gv3
