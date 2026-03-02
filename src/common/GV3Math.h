#pragma once

#include <algorithm>
#include <cmath>

namespace gv3::detail
{
inline constexpr float kPi = 3.14159265358979323846f;

inline float clampValue(float value, float minValue, float maxValue)
{
    return std::clamp(value, minValue, maxValue);
}

inline float dbToLinear(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

inline float normalized(float value, float minValue, float maxValue)
{
    if (maxValue <= minValue)
    {
        return 0.0f;
    }

    return clampValue((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
}
} // namespace gv3::detail
