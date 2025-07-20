#pragma once
#include "qd/stl/array.h"
#include <imgui/imgui.h>
#include "qd/base/color.h"

namespace qd {

//////////////////////////////////////////////////////////////////////////
struct ImColorsTab {
public:
    constexpr static size_t MAX_COLORS_COUNT = 255;
    struct ColorRec {
        qd::Color colorU32;
        ImVec4 colorF;
    };
    eastl::array<ColorRec, MAX_COLORS_COUNT> mColors = {};

public:

    [[nodiscard]] inline const qd::Color& getColorU(uint32_t col) const
    {
        const ColorRec& colorRef = mColors[col];
        return colorRef.colorU32;
    };
    [[nodiscard]] inline const ImVec4& getColorF(uint32_t col) const
    {
        const ColorRec& colorRef = mColors[col];
        return colorRef.colorF;
    };

    void setColorU(uint32_t col, const qd::Color& c)
    {
        ColorRec& rec = mColors[col];
        rec.colorU32 = c;
        rec.colorF.x = (float)c.r / 255.f;
        rec.colorF.y = (float)c.g / 255.f;
        rec.colorF.z = (float)c.b / 255.f;
        rec.colorF.w = (float)c.a / 255.f;
    }
}; // class ImColorsTab
//////////////////////////////////////////////////////////////////////////

void imGuiApplyStyleDark();

}; // namespace qd
