// GainUI.h — NanoVG draw function for the GV3 Gain plugin
// Requires: nanovg.h included before this header

#pragma once

#include <nanovg.h>
#include "GlobalVST3Engine.h"
#include "plugins/GainPlugin/GainRuntimeState.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gv3::plugins
{

inline void ensureUIFont(NVGcontext* ctx)
{
#ifdef _WIN32
    static NVGcontext* lastCtx = nullptr;
    static bool ok = false;
    if (ctx != lastCtx)
    {
        lastCtx = ctx;
        ok = nvgCreateFont(ctx, "gv3_ui", "C:/Windows/Fonts/segoeui.ttf") >= 0;
        if (!ok)
        {
            ok = nvgCreateFont(ctx, "gv3_ui", "C:/Windows/Fonts/arial.ttf") >= 0;
        }
    }
    if (ok)
    {
        nvgFontFace(ctx, "gv3_ui");
    }
#else
    (void)ctx;
#endif
}

inline NVGcolor meterColor(float level)
{
    if (level < 0.6f)
    {
        float t = level / 0.6f;
        return nvgLerpRGBA(nvgRGBA(0, 200, 80, 255), nvgRGBA(200, 220, 0, 255), t);
    }
    float t = (level - 0.6f) / 0.4f;
    return nvgLerpRGBA(nvgRGBA(200, 220, 0, 255), nvgRGBA(255, 40, 30, 255), t);
}

inline void drawMeterBar(NVGcontext* ctx, float x, float y, float barW, float barH, float level)
{
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, x, y, barW, barH, 3.0f);
    nvgFillColor(ctx, nvgRGBA(20, 20, 35, 200));
    nvgFill(ctx);

    float fillH = barH * std::clamp(level, 0.0f, 1.0f);
    if (fillH > 1.0f)
    {
        constexpr int kSegments = 24;
        float segH = barH / static_cast<float>(kSegments);
        float segGap = 1.5f;
        int litSegments = static_cast<int>(level * kSegments);
        for (int i = 0; i < litSegments && i < kSegments; ++i)
        {
            float segY = y + barH - (static_cast<float>(i + 1) * segH);
            float segLevel = static_cast<float>(i + 1) / static_cast<float>(kSegments);
            NVGcolor col = meterColor(segLevel);
            nvgBeginPath(ctx);
            nvgRoundedRect(ctx, x + 1.0f, segY + segGap * 0.5f,
                           barW - 2.0f, segH - segGap, 1.5f);
            nvgFillColor(ctx, col);
            nvgFill(ctx);
        }
    }

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, x, y, barW, barH, 3.0f);
    nvgStrokeWidth(ctx, 1.0f);
    nvgStrokeColor(ctx, nvgRGBA(50, 50, 70, 120));
    nvgStroke(ctx);
}

inline void drawGainUI(NVGcontext* ctx, int width, int height,
                       const NanoVGEditorModel& model)
{
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    float cx = w * 0.5f;
    float cy = h * 0.40f;

    auto& rt = gainRuntimeState();
    float hoverAmount = rt.hoverAmount.load(std::memory_order_relaxed);

    float baseKnobR = std::min(w, h) * 0.18f;
    float knobR = baseKnobR * (1.0f + hoverAmount * 0.06f);
    float arcR = knobR + 14.0f;

    constexpr float pi = 3.14159265358979323846f;
    float startA = pi * 0.75f;
    float totalA = pi * 1.5f;

    float norm = 0.5f;
    float db = 0.0f;
    if (!model.knobs().empty())
    {
        norm = model.knobs()[0].normalizedValue;
        db = model.knobs()[0].actualValue;
    }
    float valueA = startA + totalA * norm;

    nvgBeginFrame(ctx, width, height, 1.0f);
    ensureUIFont(ctx);

    // Single panel background — fills entire window
    NVGpaint bgPaint = nvgLinearGradient(ctx, 0, 0, 0, h,
        nvgRGBA(22, 22, 42, 255), nvgRGBA(12, 12, 26, 255));
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, 0, 0, w, h, 8.0f);
    nvgFillPaint(ctx, bgPaint);
    nvgFill(ctx);

    // Subtle inner border
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, 0.5f, 0.5f, w - 1.0f, h - 1.0f, 8.0f);
    nvgStrokeWidth(ctx, 1.0f);
    nvgStrokeColor(ctx, nvgRGBA(50, 50, 75, 100));
    nvgStroke(ctx);

    // --- Input meters (LEFT side) ---
    {
        float inL = rt.inputMeterL.load(std::memory_order_relaxed);
        float inR = rt.inputMeterR.load(std::memory_order_relaxed);
        float meterH = h * 0.50f;
        float meterW = 7.0f;
        float meterX = cx - arcR - 34.0f;
        float meterY = cy - meterH * 0.45f;

        drawMeterBar(ctx, meterX, meterY, meterW, meterH, inL);
        drawMeterBar(ctx, meterX + meterW + 2.0f, meterY, meterW, meterH, inR);

        nvgFontSize(ctx, 8.0f);
        nvgFillColor(ctx, nvgRGBA(100, 100, 130, 200));
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgText(ctx, meterX + meterW + 1.0f, meterY - 3.0f, "IN", nullptr);
    }

    // --- Output meters (RIGHT side) ---
    {
        float outL = rt.outputMeterL.load(std::memory_order_relaxed);
        float outR = rt.outputMeterR.load(std::memory_order_relaxed);
        float meterH = h * 0.50f;
        float meterW = 7.0f;
        float meterX = cx + arcR + 18.0f;
        float meterY = cy - meterH * 0.45f;

        drawMeterBar(ctx, meterX, meterY, meterW, meterH, outL);
        drawMeterBar(ctx, meterX + meterW + 2.0f, meterY, meterW, meterH, outR);

        nvgFontSize(ctx, 8.0f);
        nvgFillColor(ctx, nvgRGBA(100, 100, 130, 200));
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgText(ctx, meterX + meterW + 1.0f, meterY - 3.0f, "OUT", nullptr);
    }

    // Knob shadow
    {
        NVGpaint shadow = nvgRadialGradient(ctx, cx, cy + 4, knobR * 0.4f, knobR * 1.8f,
            nvgRGBA(0, 0, 0, 60), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(ctx);
        nvgRect(ctx, cx - knobR * 2.5f, cy - knobR * 2.0f,
                     knobR * 5.0f, knobR * 4.5f);
        nvgFillPaint(ctx, shadow);
        nvgFill(ctx);
    }

    // Arc track
    nvgBeginPath(ctx);
    nvgArc(ctx, cx, cy, arcR, startA, startA + totalA, NVG_CW);
    nvgStrokeWidth(ctx, 3.5f);
    nvgStrokeColor(ctx, nvgRGBA(30, 30, 50, 180));
    nvgLineCap(ctx, NVG_ROUND);
    nvgStroke(ctx);

    // Arc value
    if (norm > 0.005f)
    {
        nvgBeginPath(ctx);
        nvgArc(ctx, cx, cy, arcR, startA, valueA, NVG_CW);
        nvgStrokeWidth(ctx, 12.0f);
        nvgStrokeColor(ctx, nvgRGBA(0, 170, 255, 28));
        nvgLineCap(ctx, NVG_ROUND);
        nvgStroke(ctx);

        nvgBeginPath(ctx);
        nvgArc(ctx, cx, cy, arcR, startA, valueA, NVG_CW);
        nvgStrokeWidth(ctx, 6.0f);
        nvgStrokeColor(ctx, nvgRGBA(0, 200, 255, 50));
        nvgLineCap(ctx, NVG_ROUND);
        nvgStroke(ctx);

        nvgBeginPath(ctx);
        nvgArc(ctx, cx, cy, arcR, startA, valueA, NVG_CW);
        nvgStrokeWidth(ctx, 3.0f);
        nvgStrokeColor(ctx, nvgRGBA(0, 215, 255, 235));
        nvgLineCap(ctx, NVG_ROUND);
        nvgStroke(ctx);

        float dx = cx + std::cos(valueA) * arcR;
        float dy = cy + std::sin(valueA) * arcR;
        nvgBeginPath(ctx);
        nvgCircle(ctx, dx, dy, 5.0f);
        nvgFillColor(ctx, nvgRGBA(0, 200, 255, 55));
        nvgFill(ctx);
        nvgBeginPath(ctx);
        nvgCircle(ctx, dx, dy, 2.5f);
        nvgFillColor(ctx, nvgRGBA(0, 225, 255, 255));
        nvgFill(ctx);
    }

    // Knob body
    {
        NVGpaint knobPaint = nvgRadialGradient(ctx,
            cx - knobR * 0.12f, cy - knobR * 0.22f,
            knobR * 0.05f, knobR,
            nvgRGBA(78, 78, 108, 255), nvgRGBA(28, 28, 44, 255));
        nvgBeginPath(ctx);
        nvgCircle(ctx, cx, cy, knobR);
        nvgFillPaint(ctx, knobPaint);
        nvgFill(ctx);

        nvgBeginPath(ctx);
        nvgCircle(ctx, cx, cy, knobR);
        nvgStrokeWidth(ctx, 1.5f);
        nvgStrokeColor(ctx, nvgRGBA(55, 55, 80, 160));
        nvgStroke(ctx);

        NVGpaint hl = nvgRadialGradient(ctx,
            cx - knobR * 0.2f, cy - knobR * 0.32f,
            knobR * 0.04f, knobR * 0.55f,
            nvgRGBA(255, 255, 255, 22), nvgRGBA(255, 255, 255, 0));
        nvgBeginPath(ctx);
        nvgCircle(ctx, cx, cy, knobR);
        nvgFillPaint(ctx, hl);
        nvgFill(ctx);

        nvgBeginPath(ctx);
        nvgCircle(ctx, cx, cy, knobR * 0.88f);
        nvgStrokeWidth(ctx, 1.0f);
        nvgStrokeColor(ctx, nvgRGBA(20, 20, 35, 80));
        nvgStroke(ctx);
    }

    // Knob indicator line
    {
        float ix1 = cx + std::cos(valueA) * knobR * 0.40f;
        float iy1 = cy + std::sin(valueA) * knobR * 0.40f;
        float ix2 = cx + std::cos(valueA) * knobR * 0.82f;
        float iy2 = cy + std::sin(valueA) * knobR * 0.82f;

        nvgBeginPath(ctx);
        nvgMoveTo(ctx, ix1, iy1);
        nvgLineTo(ctx, ix2, iy2);
        nvgStrokeWidth(ctx, 5.0f);
        nvgStrokeColor(ctx, nvgRGBA(0, 200, 255, 45));
        nvgLineCap(ctx, NVG_ROUND);
        nvgStroke(ctx);

        nvgBeginPath(ctx);
        nvgMoveTo(ctx, ix1, iy1);
        nvgLineTo(ctx, ix2, iy2);
        nvgStrokeWidth(ctx, 2.0f);
        nvgStrokeColor(ctx, nvgRGBA(0, 225, 255, 245));
        nvgLineCap(ctx, NVG_ROUND);
        nvgStroke(ctx);
    }

    // Knob center dot
    {
        NVGpaint dp = nvgRadialGradient(ctx, cx, cy, 1.0f, 4.0f,
            nvgRGBA(18, 18, 30, 255), nvgRGBA(40, 40, 60, 255));
        nvgBeginPath(ctx);
        nvgCircle(ctx, cx, cy, 3.5f);
        nvgFillPaint(ctx, dp);
        nvgFill(ctx);
    }

    // "GAIN" label
    nvgFontSize(ctx, 11.0f);
    nvgFillColor(ctx, nvgRGBA(120, 120, 150, 255));
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(ctx, cx, cy - knobR - 26.0f, "G A I N", nullptr);

    // dB value — large, clearly visible
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%+.1f dB", db);
        float dbTextY = cy + knobR + 30.0f;

        nvgFontSize(ctx, 22.0f);
        nvgFillColor(ctx, nvgRGBA(240, 240, 255, 255));
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(ctx, cx, dbTextY, buf, nullptr);
    }

    // Footer separator
    {
        float sy = h - 28.0f;
        NVGpaint sep = nvgLinearGradient(ctx, w * 0.25f, sy, w * 0.75f, sy,
            nvgRGBA(50, 50, 75, 0), nvgRGBA(50, 50, 75, 80));
        nvgBeginPath(ctx);
        nvgMoveTo(ctx, w * 0.25f, sy);
        nvgLineTo(ctx, w * 0.5f, sy);
        nvgStrokeWidth(ctx, 1.0f);
        nvgStrokePaint(ctx, sep);
        nvgStroke(ctx);

        NVGpaint sep2 = nvgLinearGradient(ctx, w * 0.5f, sy, w * 0.75f, sy,
            nvgRGBA(50, 50, 75, 80), nvgRGBA(50, 50, 75, 0));
        nvgBeginPath(ctx);
        nvgMoveTo(ctx, w * 0.5f, sy);
        nvgLineTo(ctx, w * 0.75f, sy);
        nvgStrokeWidth(ctx, 1.0f);
        nvgStrokePaint(ctx, sep2);
        nvgStroke(ctx);
    }

    // Footer text
    nvgFontSize(ctx, 9.0f);
    nvgFillColor(ctx, nvgRGBA(60, 60, 85, 180));
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(ctx, cx, h - 16.0f, "GV3 ENGINE", nullptr);

    nvgEndFrame(ctx);
}

inline NanoVGEditorBridge createGainEditorBridge()
{
    return NanoVGEditorBridge(
        [](void* vg, int w, int h, const NanoVGEditorModel& model) {
            drawGainUI(static_cast<NVGcontext*>(vg), w, h, model);
        });
}

} // namespace gv3::plugins
