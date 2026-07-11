#pragma once
// Font Awesome 6 Solid icons — codepoints used by the debugger toolbar.
// The font is loaded and merged into ImGui's font atlas by imGuiContextManager.
//
// Usage:  ImGui::Button(ICON_FA_PLAY " Play")   ->  "▶ Play"

// Playback / control
#define ICON_FA_PLAY            "\xef\x81\x8b"   // U+F04B
#define ICON_FA_PAUSE           "\xef\x81\x8c"   // U+F04C
#define ICON_FA_STOP            "\xef\x81\x8d"   // U+F04D
#define ICON_FA_FORWARD_STEP    "\xef\x81\x91"   // U+F051  step-forward
#define ICON_FA_BACKWARD_STEP   "\xef\x81\x88"   // U+F048  step-backward
#define ICON_FA_FORWARD         "\xef\x81\x90"   // U+F050  forward-fast
#define ICON_FA_ARROW_RIGHT_FROM_BRACKET "\xef\x82\x8b" // U+F08B  arrow-right-from-bracket (step-out)

// System
#define ICON_FA_ROTATE_RIGHT    "\xef\x80\x9e"   // U+F01E  redo / reset
#define ICON_FA_BOLT            "\xef\x83\xa7"   // U+F0E7  turbo
#define ICON_FA_FLAG_CHECKERED  "\xef\x80\xa1"   // U+F11E  trace start

// Debug
#define ICON_FA_BUG             "\xef\x86\x88"   // U+F188
#define ICON_FA_CROSSHAIRS      "\xef\x81\x9b"   // U+F05B  breakpoint

// Visibility
#define ICON_FA_EYE             "\xef\x81\xae"   // U+F06E
#define ICON_FA_CLOCK           "\xef\x80\x97"   // U+F017  scanlines
