#pragma once

#include <nanovg.h>
#include "Rect.h"

namespace gv3::ui2
{

/**
 * @brief RAII guard for NanoVG scissor region.
 * 
 * Automatically resets scissor on destruction to prevent clip leaks.
 * 
 * Usage:
 * @code
 * {
 *     ScissorGuard guard(vg, myRect);
 *     // ... draw clipped content ...
 * } // scissor automatically reset here
 * @endcode
 */
class ScissorGuard
{
public:
    explicit ScissorGuard(NVGcontext* vg, const Rect& rect)
        : m_vg(vg)
    {
        if (m_vg && rect.isValid())
        {
            nvgScissor(m_vg, rect.x, rect.y, rect.w, rect.h);
            m_active = true;
        }
    }

    ~ScissorGuard()
    {
        if (m_active && m_vg)
        {
            nvgResetScissor(m_vg);
        }
    }

    // Non-copyable, non-movable (RAII semantics)
    ScissorGuard(const ScissorGuard&) = delete;
    ScissorGuard& operator=(const ScissorGuard&) = delete;

private:
    NVGcontext* m_vg = nullptr;
    bool m_active = false;
};

} // namespace gv3::ui2
