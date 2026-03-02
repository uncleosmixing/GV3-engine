#pragma once

#include "GlobalVST3Engine.h"
#include "public.sdk/source/common/pluginview.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

struct NVGcontext;

namespace gv3::vst3
{

class GV3ControllerBase;

class GV3EditorView : public Steinberg::CPluginView
{
public:
    explicit GV3EditorView(GV3ControllerBase* controller,
                           int width = 360, int height = 420);
    ~GV3EditorView() override;

    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API removed() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) SMTG_OVERRIDE;

private:
    bool createPlatformResources(void* parentWindow);
    void destroyPlatformResources();
    void render();

#ifdef _WIN32
    void beginDrag(int mouseY);
    void updateDrag(int mouseY);
    void endDrag();
    void applyNormalized(float normalizedValue);
    void updateHover(int mouseX, int mouseY);
    bool isOverValueText(int mouseX, int mouseY) const;
    bool isOverKnob(int mouseX, int mouseY) const;
    void beginValueEdit();
    void commitValueEdit();
    void cancelValueEdit();

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK valueEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    HWND m_hwnd { nullptr };
    HDC m_hdc { nullptr };
    HGLRC m_hglrc { nullptr };
    UINT_PTR m_timerId { 0 };
    bool m_dragging { false };
    int m_lastMouseY { 0 };
    float m_dragNorm { 0.0f };
    float m_hoverAmount { 0.0f };
    float m_hoverTarget { 0.0f };
    HWND m_valueEdit { nullptr };
    WNDPROC m_prevValueEditProc { nullptr };
    static constexpr int kValueEditId = 1001;
    static constexpr UINT_PTR kTimerId = 1;
    static constexpr UINT kTimerIntervalMs = 16;
#endif

    GV3ControllerBase* m_controller { nullptr };
    NVGcontext* m_nvg { nullptr };
    int m_width { 360 };
    int m_height { 420 };
};

} // namespace gv3::vst3
