#include "GV3ProcessorBase.h"

#include "plugins/GainPlugin/GainRuntimeState.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gv3::vst3
{

using namespace Steinberg;
using namespace Steinberg::Vst;

GV3ProcessorBase::GV3ProcessorBase(EngineFactory factory)
    : m_factory(std::move(factory))
{
}

tresult PLUGIN_API GV3ProcessorBase::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    m_engine = m_factory();
    if (!m_engine)
    {
        return kResultFalse;
    }

    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API GV3ProcessorBase::terminate()
{
    m_engine.reset();
    return AudioEffect::terminate();
}

tresult PLUGIN_API GV3ProcessorBase::setActive(TBool state)
{
    if (!m_engine)
    {
        return kResultFalse;
    }

    if (state)
    {
        ProcessSpec spec {};
        spec.sampleRate = processSetup.sampleRate;
        spec.maxBlockSize = static_cast<std::uint32_t>(processSetup.maxSamplesPerBlock);
        spec.channelCount = 2;
        m_engine->prepare(spec);
    }
    else
    {
        m_engine->reset();
    }

    return AudioEffect::setActive(state);
}

tresult PLUGIN_API GV3ProcessorBase::setupProcessing(ProcessSetup& setup)
{
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API GV3ProcessorBase::setBusArrangements(
    SpeakerArrangement* inputs, int32 numIns,
    SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == SpeakerArr::kStereo &&
        outputs[0] == SpeakerArr::kStereo)
    {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }

    return kResultFalse;
}

tresult PLUGIN_API GV3ProcessorBase::process(ProcessData& data)
{
    if (!m_engine)
    {
        return kResultFalse;
    }

    if (data.inputParameterChanges)
    {
        int32 numChanges = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numChanges; ++i)
        {
            auto* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue)
            {
                continue;
            }

            ParamID paramId = queue->getParameterId();
            int32 numPoints = queue->getPointCount();
            if (numPoints > 0)
            {
                ParamValue value = 0.0;
                int32 sampleOffset = 0;
                queue->getPoint(numPoints - 1, sampleOffset, value);
                m_engine->parameters().setNormalized(
                    static_cast<std::size_t>(paramId),
                    static_cast<float>(value));
            }
        }
    }

    if (data.numSamples > 0 && data.numOutputs > 0 && data.outputs[0].numChannels > 0)
    {
        auto numChannels = static_cast<std::size_t>(data.outputs[0].numChannels);

        float inPeakL = 0.0f;
        float inPeakR = 0.0f;
        if (data.numInputs > 0 && data.inputs[0].channelBuffers32)
        {
            for (int32 ch = 0; ch < data.outputs[0].numChannels; ++ch)
            {
                if (ch < data.inputs[0].numChannels)
                {
                    const float* inBuf = data.inputs[0].channelBuffers32[ch];
                    if (inBuf)
                    {
                        float chPeak = 0.0f;
                        for (int32 s = 0; s < data.numSamples; ++s)
                        {
                            chPeak = std::max(chPeak, std::abs(inBuf[s]));
                        }
                        if (ch == 0) inPeakL = chPeak;
                        if (ch == 1) inPeakR = chPeak;
                    }
                }

                if (ch < data.inputs[0].numChannels &&
                    data.inputs[0].channelBuffers32[ch] != data.outputs[0].channelBuffers32[ch])
                {
                    std::memcpy(data.outputs[0].channelBuffers32[ch],
                                data.inputs[0].channelBuffers32[ch],
                                static_cast<std::size_t>(data.numSamples) * sizeof(float));
                }
            }
        }

        AudioBlock block(data.outputs[0].channelBuffers32,
                         numChannels,
                         static_cast<std::size_t>(data.numSamples));
        m_engine->process(block);

        float outPeakL = 0.0f;
        float outPeakR = 0.0f;
        if (data.outputs[0].channelBuffers32)
        {
            for (int32 ch = 0; ch < data.outputs[0].numChannels; ++ch)
            {
                const float* outBuf = data.outputs[0].channelBuffers32[ch];
                if (!outBuf)
                {
                    continue;
                }
                float chPeak = 0.0f;
                for (int32 s = 0; s < data.numSamples; ++s)
                {
                    chPeak = std::max(chPeak, std::abs(outBuf[s]));
                }
                if (ch == 0) outPeakL = chPeak;
                if (ch == 1) outPeakR = chPeak;
            }
        }

        auto toDb = [](float v) {
            return 20.0f * std::log10(std::max(v, 1.0e-6f));
        };

        auto& rt = gv3::plugins::gainRuntimeState();

        constexpr float kSmooth = 0.25f;
        constexpr float kMinDb = -60.0f;
        constexpr float kMaxDb = 6.0f;
        constexpr float kRange = kMaxDb - kMinDb;

        auto toNorm = [&](float peak) {
            return std::clamp((toDb(peak) - kMinDb) / kRange, 0.0f, 1.0f);
        };

        auto smooth = [&](std::atomic<float>& dst, float now) {
            float prev = dst.load(std::memory_order_relaxed);
            float s = (now > prev) ? prev + (now - prev) * kSmooth
                                   : prev + (now - prev) * 0.08f;
            dst.store(s, std::memory_order_relaxed);
        };

        smooth(rt.inputMeterL, toNorm(inPeakL));
        smooth(rt.inputMeterR, toNorm(inPeakR));
        smooth(rt.outputMeterL, toNorm(outPeakL));
        smooth(rt.outputMeterR, toNorm(outPeakR));

        const float inDbNow = std::clamp(toDb((inPeakL + inPeakR) * 0.5f), -60.0f, 12.0f);
        const float outDbNow = std::clamp(toDb((outPeakL + outPeakR) * 0.5f), -60.0f, 12.0f);

        smooth(rt.inputDb, inDbNow);
        smooth(rt.outputDb, outDbNow);
    }

    return kResultOk;
}

tresult PLUGIN_API GV3ProcessorBase::setState(IBStream* state)
{
    if (!m_engine || !state)
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

    auto engineParamCount = m_engine->parameters().count();
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
            m_engine->parameters().setByIndex(i, value);
        }
    }

    return kResultOk;
}

tresult PLUGIN_API GV3ProcessorBase::getState(IBStream* state)
{
    if (!m_engine || !state)
    {
        return kResultFalse;
    }

    int32 numBytesWritten = 0;

    uint32 version = 1;
    state->write(&version, sizeof(version), &numBytesWritten);

    auto paramCount = static_cast<uint32>(m_engine->parameters().count());
    state->write(&paramCount, sizeof(paramCount), &numBytesWritten);

    for (uint32 i = 0; i < paramCount; ++i)
    {
        float value = m_engine->parameters().getByIndex(i);
        state->write(&value, sizeof(value), &numBytesWritten);
    }

    return kResultOk;
}

} // namespace gv3::vst3
