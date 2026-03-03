#include "GV3EditorView.h"
#include "GV3ControllerBase.h"

#include "plugins/GainPlugin/GainRuntimeState.h"
#include "pluginterfaces/base/ftypes.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#ifdef _WIN32
#include <windowsx.h>
#undef min
#undef max
#include <GL/gl.h>
#pragma comment(lib, "opengl32.lib")
#endif

namespace gv3::vst3
{

using namespace Steinberg;
using namespace Steinberg::Vst;

GV3EditorView::GV3EditorView(GV3ControllerBase* controller, int width, int height)
    : CPluginView(nullptr)
    , m_controller(controller)
    , m_width(width)
    , m_height(height)
{
    ViewRect r { 0, 0, static_cast<Steinberg::int32>(width),
                        static_cast<Steinberg::int32>(height) };
    rect = r;
}

GV3EditorView::~GV3EditorView()
{
    destroyPlatformResources();
}

tresult PLUGIN_API GV3EditorView::isPlatformTypeSupported(FIDString type)
{
#ifdef _WIN32
    if (std::strcmp(type, kPlatformTypeHWND) == 0)
    {
        return kResultOk;
    }
#endif
    (void)type;
    return kResultFalse;
}

tresult PLUGIN_API GV3EditorView::attached(void* parent, FIDString type)
{
    if (!createPlatformResources(parent))
    {
        return kResultFalse;
    }

    return CPluginView::attached(parent, type);
}

tresult PLUGIN_API GV3EditorView::removed()
{
    destroyPlatformResources();
    return CPluginView::removed();
}

tresult PLUGIN_API GV3EditorView::onSize(ViewRect* newSize)
{
    if (newSize)
    {
        m_width = newSize->getWidth();
        m_height = newSize->getHeight();

#ifdef _WIN32
        if (m_hwnd)
        {
            MoveWindow(m_hwnd, 0, 0, m_width, m_height, TRUE);
        }
#endif
    }

    return CPluginView::onSize(newSize);
}

#ifdef _WIN32

bool GV3EditorView::createPlatformResources(void* parentWindow)
{
    auto parentHwnd = static_cast<HWND>(parentWindow);

    static const wchar_t* kClassName = L"GV3EditorViewClass";
    static bool classRegistered = false;
    if (!classRegistered)
    {
        WNDCLASSEXW wc {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = wndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        classRegistered = true;
    }

    m_hwnd = CreateWindowExW(
        0, kClassName, L"GV3Editor",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, m_width, m_height,
        parentHwnd, nullptr, GetModuleHandleW(nullptr), this);

    if (!m_hwnd)
    {
        return false;
    }

    m_hdc = GetDC(m_hwnd);
    if (!m_hdc)
    {
        destroyPlatformResources();
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pixelFormat = ChoosePixelFormat(m_hdc, &pfd);
    if (pixelFormat == 0)
    {
        destroyPlatformResources();
        return false;
    }

    SetPixelFormat(m_hdc, pixelFormat, &pfd);
    m_hglrc = wglCreateContext(m_hdc);
    if (!m_hglrc)
    {
        destroyPlatformResources();
        return false;
    }

    wglMakeCurrent(m_hdc, m_hglrc);

    if (m_controller)
    {
        const auto& initCb = m_controller->graphicsInit();
        if (initCb)
        {
            m_nvg = static_cast<NVGcontext*>(initCb());
        }
    }

    wglMakeCurrent(nullptr, nullptr);

    m_timerId = SetTimer(m_hwnd, kTimerId, kTimerIntervalMs, nullptr);
    return true;
}

void GV3EditorView::destroyPlatformResources()
{
    if (m_hwnd && m_timerId)
    {
        KillTimer(m_hwnd, m_timerId);
        m_timerId = 0;
    }

    if (m_dragging)
    {
        endDrag();
    }

    cancelValueEdit();

    if (m_hglrc)
    {
        wglMakeCurrent(m_hdc, m_hglrc);

        if (m_nvg && m_controller)
        {
            const auto& shutdownCb = m_controller->graphicsShutdown();
            if (shutdownCb)
            {
                shutdownCb(m_nvg);
            }
            m_nvg = nullptr;
        }

        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(m_hglrc);
        m_hglrc = nullptr;
    }

    if (m_hdc && m_hwnd)
    {
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = nullptr;
    }

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void GV3EditorView::render()
{
    if (!m_hglrc || !m_hdc)
    {
        return;
    }

    m_hoverAmount += (m_hoverTarget - m_hoverAmount) * 0.12f;
    auto& rt = gv3::plugins::gainRuntimeState();
    rt.hoverAmount.store(m_hoverAmount, std::memory_order_relaxed);

    wglMakeCurrent(m_hdc, m_hglrc);

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.08f, 0.08f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (m_nvg && m_controller)
    {
        const auto& bridge = m_controller->editorBridge();
        if (bridge)
        {
            auto model = m_controller->buildEditorModel();
            bridge.draw(m_nvg, m_width, m_height, model);
        }
    }

    SwapBuffers(m_hdc);
    wglMakeCurrent(nullptr, nullptr);
}

void GV3EditorView::applyNormalized(float normalizedValue)
{
    if (!m_controller)
    {
        return;
    }

    const float clamped = std::clamp(normalizedValue, 0.0f, 1.0f);
    constexpr ParamID kGainParamId = 0;
    m_controller->setParamNormalized(kGainParamId, clamped);
    m_controller->performEdit(kGainParamId, clamped);
}

void GV3EditorView::beginDrag(int mouseY)
{
    if (!m_controller || m_dragging || m_valueEdit)
    {
        return;
    }

    constexpr ParamID kGainParamId = 0;
    m_dragNorm = static_cast<float>(m_controller->getParamNormalized(kGainParamId));
    m_lastMouseY = mouseY;
    m_dragging = true;
    m_controller->beginEdit(kGainParamId);
    SetCapture(m_hwnd);
}

void GV3EditorView::updateDrag(int mouseY)
{
    if (!m_dragging)
    {
        return;
    }

    const int dy = mouseY - m_lastMouseY;
    m_lastMouseY = mouseY;

    constexpr float kSensitivity = 0.0045f;
    m_dragNorm = std::clamp(m_dragNorm - static_cast<float>(dy) * kSensitivity, 0.0f, 1.0f);
    applyNormalized(m_dragNorm);
}

void GV3EditorView::endDrag()
{
    if (!m_controller || !m_dragging)
    {
        return;
    }

    constexpr ParamID kGainParamId = 0;
    m_controller->endEdit(kGainParamId);
    m_dragging = false;
    ReleaseCapture();
}

bool GV3EditorView::isOverKnob(int mouseX, int mouseY) const
{
    const float w = static_cast<float>(m_width);
    const float h = static_cast<float>(m_height);
    const float cx = w * 0.5f;
    const float cy = h * 0.40f;
    const float knobR = std::min(w, h) * 0.18f * (1.0f + m_hoverAmount * 0.06f);
    const float dx = static_cast<float>(mouseX) - cx;
    const float dy = static_cast<float>(mouseY) - cy;
    return (dx * dx + dy * dy) <= (knobR * knobR);
}

bool GV3EditorView::isOverValueText(int mouseX, int mouseY) const
{
    const float w = static_cast<float>(m_width);
    const float h = static_cast<float>(m_height);
    const float cx = w * 0.5f;
    const float cy = h * 0.40f;
    const float knobR = std::min(w, h) * 0.18f;
    const float tx = cx - 75.0f;
    const float ty = cy + knobR + 14.0f;
    const float tw = 150.0f;
    const float th = 34.0f;
    return mouseX >= tx && mouseX <= (tx + tw) && mouseY >= ty && mouseY <= (ty + th);
}

void GV3EditorView::updateHover(int mouseX, int mouseY)
{
    const bool overKnob = isOverKnob(mouseX, mouseY);
    m_hoverTarget = overKnob ? 1.0f : 0.0f;
    gv3::plugins::gainRuntimeState().hoverTarget.store(m_hoverTarget, std::memory_order_relaxed);
}

void GV3EditorView::beginValueEdit()
{
    if (m_valueEdit || !m_hwnd || !m_controller)
    {
        return;
    }

    constexpr ParamID kGainParamId = 0;
    const float norm = static_cast<float>(m_controller->getParamNormalized(kGainParamId));
    const float db = -60.0f + norm * 84.0f;

    char text[32] {};
    std::snprintf(text, sizeof(text), "%+.1f", db);

    const float w = static_cast<float>(m_width);
    const float h = static_cast<float>(m_height);
    const float cx = w * 0.5f;
    const float cy = h * 0.40f;
    const float knobR = std::min(w, h) * 0.18f;

    const int x = static_cast<int>(cx - 65.0f);
    const int y = static_cast<int>(cy + knobR + 14.0f);
    const int ww = 130;
    const int hh = 26;

    m_valueEdit = CreateWindowExA(0, "EDIT", text,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
        x, y, ww, hh,
        m_hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kValueEditId)),
        GetModuleHandleW(nullptr),
        nullptr);

    if (!m_valueEdit)
    {
        return;
    }

    // Apply DARK THEME styling to match UI
    HBRUSH darkBrush = CreateSolidBrush(RGB(28, 28, 44));  // Match knob color
    SetClassLongPtrA(m_valueEdit, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(darkBrush));
    
    // Set text color to white (via WM_CTLCOLOREDIT in parent window proc)
    // Note: text color needs to be set in parent's WM_CTLCOLOREDIT handler

    m_prevValueEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
        m_valueEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&GV3EditorView::valueEditProc)));
    SetWindowLongPtrA(m_valueEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    SetFocus(m_valueEdit);
    SendMessageA(m_valueEdit, EM_SETSEL, 0, -1);
    gv3::plugins::gainRuntimeState().valueEditing.store(true, std::memory_order_relaxed);
}

void GV3EditorView::commitValueEdit()
{
    if (!m_valueEdit || !m_controller)
    {
        return;
    }

    char text[64] {};
    GetWindowTextA(m_valueEdit, text, static_cast<int>(sizeof(text)));

    char* endPtr = nullptr;
    const float parsed = std::strtof(text, &endPtr);
    if (endPtr != text)
    {
        const float clampedDb = std::clamp(parsed, -60.0f, 24.0f);
        const float norm = (clampedDb + 60.0f) / 84.0f;
        constexpr ParamID kGainParamId = 0;
        m_controller->beginEdit(kGainParamId);
        applyNormalized(norm);
        m_controller->endEdit(kGainParamId);
    }

    cancelValueEdit();
}

void GV3EditorView::cancelValueEdit()
{
    if (!m_valueEdit)
    {
        return;
    }

    if (m_prevValueEditProc)
    {
        SetWindowLongPtrA(m_valueEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_prevValueEditProc));
        m_prevValueEditProc = nullptr;
    }

    DestroyWindow(m_valueEdit);
    m_valueEdit = nullptr;
    gv3::plugins::gainRuntimeState().valueEditing.store(false, std::memory_order_relaxed);
}

LRESULT CALLBACK GV3EditorView::valueEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<GV3EditorView*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (!self || !self->m_prevValueEditProc)
    {
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    if (msg == WM_GETDLGCODE)
    {
        return DLGC_WANTALLKEYS;
    }

    if (msg == WM_KEYDOWN)
    {
        if (wParam == VK_RETURN)
        {
            self->commitValueEdit();
            SetFocus(self->m_hwnd);
            return 0;
        }
        if (wParam == VK_ESCAPE)
        {
            self->cancelValueEdit();
            SetFocus(self->m_hwnd);
            return 0;
        }
    }

    if (msg == WM_CHAR)
    {
        if (wParam == '\r' || wParam == '\n')
        {
            return 0;
        }
    }

    if (msg == WM_KILLFOCUS)
    {
        if (self->m_valueEdit)
        {
            self->commitValueEdit();
        }
        return 0;
    }

    return CallWindowProcA(self->m_prevValueEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK GV3EditorView::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<GV3EditorView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    case WM_TIMER:
        if (self && wParam == kTimerId)
        {
            self->render();
        }
        return 0;

    case WM_MOUSEMOVE:
        if (self)
        {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            self->updateHover(x, y);

            if (self->m_dragging)
            {
                self->updateDrag(y);
            }
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (self)
        {
            SetFocus(hwnd);
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            
            // Check peak hold click first (has priority)
            self->handleMeterPeakHoldClick(x, y);
            
            // Then check value text edit
            if (self->isOverValueText(x, y))
            {
                self->beginValueEdit();
                return 0;
            }
            
            // Finally check knob drag
            self->beginDrag(y);
        }
        return 0;

    case WM_LBUTTONUP:
        if (self)
        {
            self->endDrag();
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (self && self->m_controller && !self->m_valueEdit)
        {
            constexpr ParamID kGainParamId = 0;
            float v = static_cast<float>(self->m_controller->getParamNormalized(kGainParamId));
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            v += (delta > 0 ? 0.03f : -0.03f);
            self->m_controller->beginEdit(kGainParamId);
            self->applyNormalized(v);
            self->m_controller->endEdit(kGainParamId);
        }
        return 0;

    case WM_RBUTTONDOWN:
        if (self && self->m_controller && !self->m_valueEdit)
        {
            constexpr ParamID kGainParamId = 0;
            constexpr float kZeroDbNormalized = 60.0f / 84.0f;
            self->m_controller->beginEdit(kGainParamId);
            self->applyNormalized(kZeroDbNormalized);
            self->m_controller->endEdit(kGainParamId);
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (self)
        {
            self->render();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLOREDIT:
    {
        // Dark theme for value edit control
        if (self && self->m_valueEdit && reinterpret_cast<HWND>(lParam) == self->m_valueEdit)
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(240, 240, 255));       // Light text (matches gain readout)
            SetBkColor(hdc, RGB(28, 28, 44));            // Dark background (matches knob)
            static HBRUSH darkBrush = CreateSolidBrush(RGB(28, 28, 44));
            return reinterpret_cast<LRESULT>(darkBrush);
        }
        break;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void GV3EditorView::handleMeterPeakHoldClick(int mouseX, int mouseY)
{
    // Check if click is inside any meter's peak hold bounds
    auto& rt = gv3::plugins::gainRuntimeState();
    if (!rt.uiState)
    {
        return;
    }
    
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);
    
    // Check all four meters
    if (rt.uiState->meterInL.isInsidePeakHoldBounds(mx, my))
    {
        rt.uiState->meterInL.resetPeakHold();
    }
    else if (rt.uiState->meterInR.isInsidePeakHoldBounds(mx, my))
    {
        rt.uiState->meterInR.resetPeakHold();
    }
    else if (rt.uiState->meterOutL.isInsidePeakHoldBounds(mx, my))
    {
        rt.uiState->meterOutL.resetPeakHold();
    }
    else if (rt.uiState->meterOutR.isInsidePeakHoldBounds(mx, my))
    {
        rt.uiState->meterOutR.resetPeakHold();
    }
}

#else

bool GV3EditorView::createPlatformResources(void* /*parentWindow*/)
{
    return false;
}

void GV3EditorView::destroyPlatformResources()
{
}

void GV3EditorView::render()
{
}

#endif

} // namespace gv3::vst3
