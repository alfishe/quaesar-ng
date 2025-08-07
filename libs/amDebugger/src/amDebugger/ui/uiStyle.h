#pragma once
#include "qd/imGui/style/style.h"
#include <imgui/imgui.h>


namespace amD {

#define IT(name, color)
#define UiColorsList(IT)                                 \
    IT(DisasmWnd_PcCursor, qd::Color(0, 10, 160))        \
    IT(DisasmWnd_UserCursor, qd::Color(160, 160, 0))     \
    IT(DisasmWnd_OpCodeBytes, qd::Color(128, 128, 128))  \
    IT(DisasmWnd_Addr, qd::Color(192, 192, 192))         \
    IT(RegistersWnd_RegName, qd::Color(164, 164, 164))   \
    IT(RegistersWnd_RegValue, qd::Color(255, 255, 255))  \
    IT(CustomRegsWnd_RegName, qd::Color(165, 164, 164))  \
    IT(CustomRegsWnd_RegValue, qd::Color(255, 255, 255)) \
    /* UI COLOR LIST */

#undef IT


//////////////////////////////////////////////////////////////////////////
struct UiStyle : public qd::ImColorsTab {
public:
#define IT(name, color) name,
    enum EColor : uint32_t {
        UiColorsList(IT) COUNT
    }; // enum
#undef IT

    void UiStyle::applyColors()
    {
#define IT(name, color) this->setColorU(name, color);
        UiColorsList(IT);
#undef IT
    }

public:
    UiStyle::UiStyle() { applyColors(); }

    static UiStyle& get()
    {
        static UiStyle inst;
        return inst;
    }

}; // class UiStyle
//////////////////////////////////////////////////////////////////////////

static inline UiStyle& g_imColors = UiStyle::get();


//////////////////////////////////////////////////////////////////////////
[[nodiscard]] inline const qd::Color& uiGetColorU(UiStyle::EColor col)
{
    return g_imColors.getColorU(col);
};
[[nodiscard]] inline const ImVec4& uiGetColorF(UiStyle::EColor col)
{
    return g_imColors.getColorF(col);
};


}; // namespace amD
