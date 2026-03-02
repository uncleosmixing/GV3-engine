// GlobalVST3Engine.h : включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace gv3
{
    enum class PluginKind
    {
        Equalizer,
        Compressor,
        Gain
    };

    struct ParameterDefinition
    {
        std::string id;
        std::string name;
        float defaultValue { 0.0f };
        float minValue { 0.0f };
        float maxValue { 1.0f };
    };

    class ParameterStore
    {
    public:
        void define(ParameterDefinition definition);
        bool set(const std::string& id, float value);
        float get(const std::string& id) const;
        const std::vector<ParameterDefinition>& definitions() const noexcept;

        std::size_t count() const noexcept;
        const ParameterDefinition& definitionAt(std::size_t index) const;
        float getByIndex(std::size_t index) const;
        bool setByIndex(std::size_t index, float value);
        float getNormalized(std::size_t index) const;
        bool setNormalized(std::size_t index, float normalizedValue);

    private:
        std::vector<ParameterDefinition> m_definitions;
        std::unordered_map<std::string, std::size_t> m_indexById;
        std::vector<float> m_values;
    };

    struct ProcessSpec
    {
        double sampleRate { 44100.0 };
        std::uint32_t maxBlockSize { 512 };
        std::uint32_t channelCount { 2 };
    };

    class AudioBlock
    {
    public:
        explicit AudioBlock(std::vector<std::vector<float>>& channels);
        AudioBlock(float** channelPointers, std::size_t numChannels, std::size_t numSamples);

        std::size_t channelCount() const noexcept;
        std::size_t sampleCount() const noexcept;
        std::span<float> channel(std::size_t index);

    private:
        std::vector<float*> m_pointerStorage;
        float** m_channels { nullptr };
        std::size_t m_channelCount { 0 };
        std::size_t m_sampleCount { 0 };
    };

    class PluginProcessor
    {
    public:
        virtual ~PluginProcessor() = default;

        virtual std::string name() const = 0;
        virtual void prepare(const ProcessSpec& spec) = 0;
        virtual void reset() = 0;
        virtual void process(AudioBlock block) = 0;

        ParameterStore& parameters() noexcept;
        const ParameterStore& parameters() const noexcept;

    protected:
        ParameterStore m_parameters;
        ProcessSpec m_spec;
    };

    class EqualizerProcessor final : public PluginProcessor
    {
    public:
        EqualizerProcessor();

        std::string name() const override;
        void prepare(const ProcessSpec& spec) override;
        void reset() override;
        void process(AudioBlock block) override;

    private:
        struct FilterState
        {
            float low { 0.0f };
            float high { 0.0f };
        };

        std::vector<FilterState> m_state;
    };

    class CompressorProcessor final : public PluginProcessor
    {
    public:
        CompressorProcessor();

        std::string name() const override;
        void prepare(const ProcessSpec& spec) override;
        void reset() override;
        void process(AudioBlock block) override;

    private:
        std::vector<float> m_envelope;
    };

    class GainProcessor final : public PluginProcessor
    {
    public:
        GainProcessor();

        std::string name() const override;
        void prepare(const ProcessSpec& spec) override;
        void reset() override;
        void process(AudioBlock block) override;
    };

    class PluginFactory
    {
    public:
        static std::unique_ptr<PluginProcessor> createProcessor(PluginKind kind);
    };

    class NanoVGEditorModel
    {
    public:
        struct Knob
        {
            std::string id;
            std::string label;
            float normalizedValue { 0.0f };
            float actualValue { 0.0f };
            float minValue { 0.0f };
            float maxValue { 1.0f };
        };

        explicit NanoVGEditorModel(const PluginProcessor& processor);
        explicit NanoVGEditorModel(std::vector<Knob> knobs);

        const std::vector<Knob>& knobs() const noexcept;

    private:
        std::vector<Knob> m_knobs;
    };

    class NanoVGEditorBridge
    {
    public:
        using DrawCallback = std::function<void(void* nanovgContext, int width, int height, const NanoVGEditorModel& model)>;

        NanoVGEditorBridge() = default;
        explicit NanoVGEditorBridge(DrawCallback drawCallback);

        void draw(void* nanovgContext, int width, int height, const PluginProcessor& processor) const;
        void draw(void* nanovgContext, int width, int height, const NanoVGEditorModel& model) const;

        explicit operator bool() const noexcept;

    private:
        DrawCallback m_drawCallback;
    };
}
