#include "src/dsp/simd/GV3SimdGain.h"
#include "src/dsp/simd/GV3SimdConfig.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <random>

// Simple offline test to verify SIMD backend produces correct results
// compared to scalar reference implementation

namespace gv3::test
{

// Reference scalar implementation (always scalar, no SIMD)
void apply_gain_reference(float* data, std::size_t numSamples, float gain)
{
    for (std::size_t i = 0; i < numSamples; ++i)
    {
        data[i] *= gain;
    }
}

float compute_peak_reference(const float* data, std::size_t numSamples)
{
    float peak = 0.0f;
    for (std::size_t i = 0; i < numSamples; ++i)
    {
        peak = std::max(peak, std::abs(data[i]));
    }
    return peak;
}

// Generate test signal: sin + noise
void generate_test_signal(float* data, std::size_t numSamples, float freq, float sampleRate)
{
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    
    for (std::size_t i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / sampleRate;
        float sin_wave = 0.8f * std::sin(2.0f * 3.14159265f * freq * t);
        float noise = dist(rng);
        data[i] = sin_wave + noise;
    }
}

// Compute max absolute difference
float compute_max_diff(const float* a, const float* b, std::size_t numSamples)
{
    float max_diff = 0.0f;
    for (std::size_t i = 0; i < numSamples; ++i)
    {
        max_diff = std::max(max_diff, std::abs(a[i] - b[i]));
    }
    return max_diff;
}

// Compute RMS difference
float compute_rms_diff(const float* a, const float* b, std::size_t numSamples)
{
    float sum_sq = 0.0f;
    for (std::size_t i = 0; i < numSamples; ++i)
    {
        float diff = a[i] - b[i];
        sum_sq += diff * diff;
    }
    return std::sqrt(sum_sq / static_cast<float>(numSamples));
}

void run_simd_test()
{
    std::cout << "\n=== GV3 SIMD Backend Test ===" << std::endl;
    std::cout << "Backend: " << simd::backend_name << std::endl;
    std::cout << "SIMD Width: " << simd::width << std::endl;
    std::cout << "================================\n" << std::endl;

    // Test parameters
    constexpr std::size_t bufferSize = 4096;
    constexpr float sampleRate = 48000.0f;
    constexpr float testFreq = 440.0f;
    constexpr float testGain = 0.5f;

    // Allocate buffers
    std::vector<float> signal(bufferSize);
    std::vector<float> result_simd(bufferSize);
    std::vector<float> result_reference(bufferSize);

    // Generate test signal
    generate_test_signal(signal.data(), bufferSize, testFreq, sampleRate);

    // Test 1: Apply gain comparison
    std::cout << "Test 1: Apply Gain" << std::endl;
    std::cout << "-------------------" << std::endl;

    // Copy signal to both buffers
    std::copy(signal.begin(), signal.end(), result_simd.begin());
    std::copy(signal.begin(), signal.end(), result_reference.begin());

    // Apply gain using SIMD backend
    simd::apply_gain(result_simd.data(), bufferSize, testGain);

    // Apply gain using reference scalar implementation
    apply_gain_reference(result_reference.data(), bufferSize, testGain);

    // Compare results
    float max_diff_gain = compute_max_diff(result_simd.data(), result_reference.data(), bufferSize);
    float rms_diff_gain = compute_rms_diff(result_simd.data(), result_reference.data(), bufferSize);

    std::cout << "  Max Absolute Diff: " << std::scientific << std::setprecision(6) << max_diff_gain << std::endl;
    std::cout << "  RMS Diff:          " << std::scientific << std::setprecision(6) << rms_diff_gain << std::endl;
    std::cout << "  Status:            " << (max_diff_gain < 1e-5f ? "PASS" : "FAIL") << std::endl;
    std::cout << std::endl;

    // Test 2: Peak detection comparison
    std::cout << "Test 2: Peak Detection" << std::endl;
    std::cout << "----------------------" << std::endl;

    float peak_simd = simd::compute_peak(signal.data(), bufferSize);
    float peak_reference = compute_peak_reference(signal.data(), bufferSize);
    float peak_diff = std::abs(peak_simd - peak_reference);

    std::cout << "  Peak (SIMD):      " << std::fixed << std::setprecision(6) << peak_simd << std::endl;
    std::cout << "  Peak (Reference): " << std::fixed << std::setprecision(6) << peak_reference << std::endl;
    std::cout << "  Difference:       " << std::scientific << std::setprecision(6) << peak_diff << std::endl;
    std::cout << "  Status:           " << (peak_diff < 1e-5f ? "PASS" : "FAIL") << std::endl;
    std::cout << std::endl;

    // Overall result
    bool overall_pass = (max_diff_gain < 1e-5f) && (peak_diff < 1e-5f);
    std::cout << "=== Overall Result: " << (overall_pass ? "PASS" : "FAIL") << " ===" << std::endl;
    std::cout << std::endl;
}

} // namespace gv3::test

int main()
{
    gv3::test::run_simd_test();
    return 0;
}
