#pragma once

#include <NEAGUI.h>
#include "Music.hpp"

// Drop-in replacement for `NEA_GUIObjectGetEvent(x) == NEA_Clicked`: returns true
// on the click (release) edge, and plays the UI click SFX on that same edge.
//
// NEA_GUIObjectGetEvent is a pure read (no side effects) and NEA_Clicked fires
// once per release for at most one object per frame, so this can't double-play.
//
// A null object is not a click. Screens that remove a button while they are still
// running -- the Download Play menu drops Start and Back once the room closes --
// leave a null handle behind, and NEA_GUIObjectGetEvent asserts on NULL. Checking
// here covers every call site at once.
inline bool GuiClicked(const NEA_GUIObj *o)
{
    if (o == nullptr)
        return false;

    if (NEA_GUIObjectGetEvent(o) == NEA_Clicked)
    {
        Music::SfxClick();
        return true;
    }
    return false;
}
