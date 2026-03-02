#include "GlobalVST3Engine.h"

#include <stdexcept>

namespace gv3
{
std::unique_ptr<PluginProcessor> PluginFactory::createProcessor(PluginKind kind)
{
    switch (kind)
    {
    case PluginKind::Equalizer:
        return std::make_unique<EqualizerProcessor>();
    case PluginKind::Compressor:
        return std::make_unique<CompressorProcessor>();
    case PluginKind::Gain:
        return std::make_unique<GainProcessor>();
    default:
        throw std::invalid_argument("Unsupported plugin kind");
    }
}
} // namespace gv3
