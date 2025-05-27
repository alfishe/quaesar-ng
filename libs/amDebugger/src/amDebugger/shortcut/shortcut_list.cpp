#include "shortcut_list.h"
#include <qdIce/qdUI/shortcutMgr.h>


namespace qd {
namespace shortcut {


static ShortcutSetupFunc shortcuts_list[] = {
#define SHORTCUT(name, setup_func) setup_func,
    SHORTCUT_LIST(SHORTCUT)
#undef SHORTCUT
};  // ShortcutList



};  // namespace shortcut
};  // namespace qd
