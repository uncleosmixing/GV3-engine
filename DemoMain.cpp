#include "GlobalVST3Engine.h"

#include <iostream>

int main()
{
    using namespace gv3;

    auto eq = PluginFactory::createProcessor(PluginKind::Equalizer);
    auto comp = PluginFactory::createProcessor(PluginKind::Compressor);

    ProcessSpec spec {};
    spec.sampleRate = 48000.0;
    spec.maxBlockSize = 256;
    spec.channelCount = 2;

    eq->prepare(spec);
    comp->prepare(spec);

    std::vector<std::vector<float>> channels(spec.channelCount, std::vector<float>(spec.maxBlockSize, 0.05f));
    AudioBlock block(channels);

    eq->parameters().set("eq.lowGainDb", 2.5f);
    eq->parameters().set("eq.highGainDb", 1.0f);
    eq->process(block);

    comp->parameters().set("comp.thresholdDb", -24.0f);
    comp->parameters().set("comp.makeupDb", 4.0f);
    comp->process(block);

    std::cout << "Engine ready: " << eq->name() << " + " << comp->name() << '\n';
    std::cout << "First sample L: " << channels[0][0] << '\n';

    return 0;
}
