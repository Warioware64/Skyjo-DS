#pragma once

#include <NEAGUI.h>
#include "Music.hpp"

// Drop-in replacement for `NEA_GUIObjectGetEvent(x) == NEA_Clicked`: returns true
// on the click (release) edge, and plays the UI click SFX on that same edge.
//
// NEA_GUIObjectGetEvent is a pure read (no side effects) and NEA_Clicked fires
// once per release for at most one object per frame, so this can't double-play.
inline bool GuiClicked(const NEA_GUIObj *o)
{
    if (NEA_GUIObjectGetEvent(o) == NEA_Clicked)
    {
        Music::SfxClick();
        return true;
    }
    return false;
}
