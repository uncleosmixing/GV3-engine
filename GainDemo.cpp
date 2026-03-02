// GainDemo.cpp — Standalone visual demo of the GV3 Gain plugin knob
// Renders with GDI+ (no external dependencies). Drag vertically to adjust gain.

#include "GlobalVST3Engine.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

using namespace Gdiplus;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int kWinW = 380;
static constexpr int kWinH = 480;
static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kArcStartDeg = 135.0f;
static constexpr float kArcSweepDeg = 270.0f;
static constexpr UINT_PTR kTimerId = 1;
static constexpr UINT kTimerMs = 16;

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
struct AppState
{
    std::unique_ptr<gv3::PluginProcessor> processor;
    float displayNorm = 0.0f;
    float targetNorm = 0.0f;
    bool dragging = false;
    int dragLastY = 0;
    HDC backDC = nullptr;
    HBITMAP backBmp = nullptr;
    int bbW = 0;
    int bbH = 0;
};

static AppState g_app;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static Color rgba(int r, int g, int b, int a = 255)
{
    return Color(static_cast<BYTE>(a), static_cast<BYTE>(r),
                 static_cast<BYTE>(g), static_cast<BYTE>(b));
}

static PointF polarPt(float cx, float cy, float radius, float angleDeg)
{
    float rad = angleDeg * kPi / 180.0f;
    return { cx + std::cos(rad) * radius, cy + std::sin(rad) * radius };
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
static void renderFrame(HDC hdc, int width, int height)
{
    Graphics gfx(hdc);
    gfx.SetSmoothingMode(SmoothingModeHighQuality);
    gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);
    gfx.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    float cx = w * 0.5f;
    float cy = h * 0.42f;
    float knobR = std::min(w, h) * 0.20f;
    float arcR = knobR + 16.0f;
    float norm = g_app.displayNorm;
    float db = -60.0f + norm * 84.0f;

    // ---- Background gradient ----
    {
        LinearGradientBrush bg(PointF(0.0f, 0.0f), PointF(0.0f, h),
                               rgba(15, 15, 30), rgba(8, 8, 18));
        gfx.FillRectangle(&bg, 0.0f, 0.0f, w, h);
    }

    // ---- Subtle panel ----
    {
        float pw = 300.0f, ph = 380.0f;
        float px = (w - pw) * 0.5f, py = (h - ph) * 0.5f - 10.0f;
        GraphicsPath panelPath;
        float cr = 18.0f;
        panelPath.AddArc(px, py, cr, cr, 180, 90);
        panelPath.AddArc(px + pw - cr, py, cr, cr, 270, 90);
        panelPath.AddArc(px + pw - cr, py + ph - cr, cr, cr, 0, 90);
        panelPath.AddArc(px, py + ph - cr, cr, cr, 90, 90);
        panelPath.CloseFigure();

        LinearGradientBrush panelBg(PointF(px, py), PointF(px, py + ph),
                                    rgba(22, 22, 42, 200), rgba(16, 16, 32, 200));
        gfx.FillPath(&panelBg, &panelPath);

        Pen panelBorder(rgba(40, 40, 65, 100), 1.0f);
        gfx.DrawPath(&panelBorder, &panelPath);
    }

    // ---- Soft shadow under knob ----
    for (int i = 22; i >= 1; --i)
    {
        float spread = static_cast<float>(i) * 2.2f;
        int alpha = static_cast<int>(50.0f * (1.0f - static_cast<float>(i) / 22.0f));
        SolidBrush sh(rgba(0, 0, 0, alpha));
        gfx.FillEllipse(&sh,
                         cx - knobR - spread,
                         cy - knobR - spread + 8.0f,
                         (knobR + spread) * 2.0f,
                         (knobR + spread) * 2.0f);
    }

    // ---- Track arc (background ring) ----
    {
        RectF arcRect(cx - arcR, cy - arcR, arcR * 2, arcR * 2);
        Pen track(rgba(30, 30, 50, 180), 3.5f);
        track.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        gfx.DrawArc(&track, arcRect, kArcStartDeg, kArcSweepDeg);
    }

    // ---- Value arc: glow + main ----
    float valueSweep = kArcSweepDeg * norm;
    if (valueSweep > 0.5f)
    {
        RectF arcRect(cx - arcR, cy - arcR, arcR * 2, arcR * 2);

        // Outer glow
        Pen glow(rgba(0, 170, 255, 30), 14.0f);
        glow.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        gfx.DrawArc(&glow, arcRect, kArcStartDeg, valueSweep);

        // Inner glow
        Pen glow2(rgba(0, 200, 255, 50), 7.0f);
        glow2.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        gfx.DrawArc(&glow2, arcRect, kArcStartDeg, valueSweep);

        // Main arc
        Pen main(rgba(0, 215, 255, 235), 3.5f);
        main.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        gfx.DrawArc(&main, arcRect, kArcStartDeg, valueSweep);

        // End dot glow
        PointF dot = polarPt(cx, cy, arcR, kArcStartDeg + valueSweep);
        SolidBrush dotGlow(rgba(0, 200, 255, 60));
        gfx.FillEllipse(&dotGlow, dot.X - 7.0f, dot.Y - 7.0f, 14.0f, 14.0f);
        SolidBrush dotCore(rgba(0, 225, 255, 255));
        gfx.FillEllipse(&dotCore, dot.X - 3.0f, dot.Y - 3.0f, 6.0f, 6.0f);
    }

    // ---- Knob body ----
    {
        GraphicsPath knobPath;
        knobPath.AddEllipse(cx - knobR, cy - knobR, knobR * 2, knobR * 2);

        PathGradientBrush body(&knobPath);
        body.SetCenterPoint(PointF(cx - knobR * 0.12f, cy - knobR * 0.22f));
        body.SetCenterColor(rgba(78, 78, 108));
        Color surround = rgba(28, 28, 44);
        int cnt = 1;
        body.SetSurroundColors(&surround, &cnt);
        gfx.FillPath(&body, &knobPath);

        // Rim
        Pen rim(rgba(55, 55, 80, 160), 1.5f);
        gfx.DrawEllipse(&rim, cx - knobR, cy - knobR, knobR * 2, knobR * 2);

        // Top-left highlight (light reflection)
        GraphicsPath hlPath;
        float hlR = knobR * 0.65f;
        hlPath.AddEllipse(cx - hlR - knobR * 0.08f,
                          cy - hlR * 0.8f - knobR * 0.18f,
                          hlR * 2.0f, hlR * 1.3f);
        PathGradientBrush hl(&hlPath);
        hl.SetCenterColor(rgba(255, 255, 255, 22));
        Color hlSur = rgba(255, 255, 255, 0);
        cnt = 1;
        hl.SetSurroundColors(&hlSur, &cnt);
        gfx.FillPath(&hl, &hlPath);

        // Inner ring
        Pen inner(rgba(20, 20, 35, 80), 1.0f);
        gfx.DrawEllipse(&inner, cx - knobR * 0.88f, cy - knobR * 0.88f,
                         knobR * 1.76f, knobR * 1.76f);
    }

    // ---- Indicator line ----
    {
        float angleDeg = kArcStartDeg + kArcSweepDeg * norm;
        PointF p1 = polarPt(cx, cy, knobR * 0.40f, angleDeg);
        PointF p2 = polarPt(cx, cy, knobR * 0.82f, angleDeg);

        // Glow
        Pen indGlow(rgba(0, 200, 255, 50), 6.0f);
        indGlow.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        gfx.DrawLine(&indGlow, p1, p2);

        // Core
        Pen indCore(rgba(0, 225, 255, 245), 2.5f);
        indCore.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        gfx.DrawLine(&indCore, p1, p2);
    }

    // ---- Center dimple ----
    {
        float dr = 4.0f;
        GraphicsPath dp;
        dp.AddEllipse(cx - dr, cy - dr, dr * 2, dr * 2);
        PathGradientBrush dpb(&dp);
        dpb.SetCenterColor(rgba(18, 18, 30));
        Color ds = rgba(40, 40, 60);
        int cnt = 1;
        dpb.SetSurroundColors(&ds, &cnt);
        gfx.FillPath(&dpb, &dp);
    }

    // ---- Text rendering ----
    FontFamily ff(L"Segoe UI");
    StringFormat cfmt;
    cfmt.SetAlignment(StringAlignmentCenter);
    cfmt.SetLineAlignment(StringAlignmentCenter);

    // Label above knob
    {
        Font f(&ff, 11.0f, FontStyleRegular, UnitPixel);
        SolidBrush br(rgba(110, 110, 140));
        RectF rc(0, cy - knobR - 48.0f, w, 22.0f);
        gfx.DrawString(L"G A I N", -1, &f, rc, &cfmt, &br);
    }

    // dB value below knob
    {
        wchar_t buf[32];
        swprintf_s(buf, L"%+.1f dB", static_cast<double>(db));
        Font f(&ff, 22.0f, FontStyleBold, UnitPixel);
        SolidBrush br(rgba(235, 235, 250));
        RectF rc(0, cy + knobR + 22.0f, w, 32.0f);
        gfx.DrawString(buf, -1, &f, rc, &cfmt, &br);
    }

    // Separator
    {
        float sy = h - 52.0f;
        float sx1 = w * 0.18f, sx2 = w * 0.82f;
        LinearGradientBrush sepBr(PointF(sx1, sy), PointF(sx2, sy),
                                  rgba(50, 50, 75, 0), rgba(50, 50, 75, 120));
        Pen sep(&sepBr, 1.0f);
        gfx.DrawLine(&sep, sx1, sy, w * 0.5f, sy);
        LinearGradientBrush sepBr2(PointF(w * 0.5f, sy), PointF(sx2, sy),
                                   rgba(50, 50, 75, 120), rgba(50, 50, 75, 0));
        Pen sep2(&sepBr2, 1.0f);
        gfx.DrawLine(&sep2, w * 0.5f, sy, sx2, sy);
    }

    // Footer
    {
        Font f(&ff, 10.0f, FontStyleRegular, UnitPixel);
        SolidBrush br(rgba(60, 60, 85));
        RectF rc(0, h - 42.0f, w, 18.0f);
        gfx.DrawString(L"GV3 ENGINE", -1, &f, rc, &cfmt, &br);
    }

    // Hint
    if (!g_app.dragging)
    {
        Font f(&ff, 9.0f, FontStyleItalic, UnitPixel);
        SolidBrush br(rgba(55, 55, 78));
        RectF rc(0, h - 24.0f, w, 16.0f);
        gfx.DrawString(L"drag vertically \u00B7 right-click to reset", -1,
                        &f, rc, &cfmt, &br);
    }
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SetTimer(hwnd, kTimerId, kTimerMs, nullptr);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        if (g_app.backDC) DeleteDC(g_app.backDC);
        if (g_app.backBmp) DeleteObject(g_app.backBmp);
        PostQuitMessage(0);
        return 0;

    case WM_TIMER:
        if (wParam == kTimerId)
        {
            float diff = g_app.targetNorm - g_app.displayNorm;
            if (std::abs(diff) > 0.0001f)
            {
                g_app.displayNorm += diff * 0.18f;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (g_app.displayNorm != g_app.targetNorm)
            {
                g_app.displayNorm = g_app.targetNorm;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        if (!g_app.backDC || g_app.bbW != w || g_app.bbH != h)
        {
            if (g_app.backDC) DeleteDC(g_app.backDC);
            if (g_app.backBmp) DeleteObject(g_app.backBmp);
            g_app.backDC = CreateCompatibleDC(hdc);
            g_app.backBmp = CreateCompatibleBitmap(hdc, w, h);
            SelectObject(g_app.backDC, g_app.backBmp);
            g_app.bbW = w;
            g_app.bbH = h;
        }

        renderFrame(g_app.backDC, w, h);
        BitBlt(hdc, 0, 0, w, h, g_app.backDC, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
        g_app.dragging = true;
        g_app.dragLastY = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        return 0;

    case WM_MOUSEMOVE:
        if (g_app.dragging)
        {
            int y = GET_Y_LPARAM(lParam);
            float delta = static_cast<float>(g_app.dragLastY - y) / 300.0f;
            g_app.targetNorm = std::clamp(g_app.targetNorm + delta, 0.0f, 1.0f);
            g_app.processor->parameters().setNormalized(0, g_app.targetNorm);
            g_app.dragLastY = y;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        g_app.dragging = false;
        ReleaseCapture();
        return 0;

    case WM_RBUTTONDOWN:
        g_app.processor->parameters().setByIndex(0, 0.0f);
        g_app.targetNorm = g_app.processor->parameters().getNormalized(0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        float step = static_cast<float>(delta) / 120.0f * 0.015f;
        g_app.targetNorm = std::clamp(g_app.targetNorm + step, 0.0f, 1.0f);
        g_app.processor->parameters().setNormalized(0, g_app.targetNorm);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GdiplusStartupInput gdipInput;
    ULONG_PTR gdipToken = 0;
    GdiplusStartup(&gdipToken, &gdipInput, nullptr);

    g_app.processor = gv3::PluginFactory::createProcessor(gv3::PluginKind::Gain);
    g_app.processor->prepare({ 48000.0, 256, 2 });
    g_app.targetNorm = g_app.processor->parameters().getNormalized(0);
    g_app.displayNorm = g_app.targetNorm;

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.lpszClassName = L"GV3GainDemo";
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    RECT wr = { 0, 0, kWinW, kWinH };
    AdjustWindowRectEx(&wr, style, FALSE, 0);

    HWND hwnd = CreateWindowExW(
        0, L"GV3GainDemo", L"GV3 Gain",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_app.processor.reset();
    GdiplusShutdown(gdipToken);
    return static_cast<int>(msg.wParam);
}
