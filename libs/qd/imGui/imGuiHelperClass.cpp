#include "imGuiHelperClass.h"

namespace qIm {

bool menuItem(const char* label, const char* shortcut /*= nullptr*/, bool selected /*= false*/, bool enabled /*= true*/)
{
    MenuItem item(label, shortcut, selected, enabled);
    return (bool)item;
}

}; // namespace qIm
