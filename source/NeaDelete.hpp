#pragma once

#include "globalHeader.hpp"

// Null-safe teardown for Nitro Engine Advanced handles.
//
// This exists because the engine's own guards are not compiled into the library
// we link. NEA_MaterialDelete() opens with NEA_AssertPointer(tex, "NULL
// pointer"), but libNEA.a is built without NEA_DEBUG, where that macro expands
// to `(void)(ptr)` -- nothing. What actually runs is:
//
//     __NEA_AsyncCancelTarget(tex);
//     if (tex->palette_autodelete) ...
//     free(tex->ram_data);
//
// So deleting a handle that was never created dereferences NULL, and deleting
// one twice reads freed memory and calls free() on whatever now sits in
// ram_data -- silent heap corruption that crashes somewhere else entirely,
// several screens later. NEA_PaletteDelete and the Hw2D deleters are the same
// shape. (NEA_GUIDeleteObject happens to be benign on a stale pointer, because
// it only frees what it finds in its own table, but it is included here so
// every teardown reads the same way.)
//
// Screens that create a handle conditionally, or that can be torn down twice,
// therefore cannot delete safely on their own. Clearing the caller's handle is
// the other half: a deleted pointer that stays non-NULL is exactly the
// dangling one the next teardown will pass back in.

inline void DeleteMaterial(NEA_Material *&mat)
{
    if (mat == nullptr) return;
    NEA_MaterialDelete(mat);
    mat = nullptr;
}

inline void DeletePalette(NEA_Palette *&pal)
{
    if (pal == nullptr) return;
    NEA_PaletteDelete(pal);
    pal = nullptr;
}

inline void DeleteGUI(NEA_GUIObj *&obj)
{
    if (obj == nullptr) return;
    NEA_GUIDeleteObject(obj);
    obj = nullptr;
}

inline void DeleteBG(NEA_Hw2DBG *&bg)
{
    if (bg == nullptr) return;
    NEA_Hw2DBGDelete(bg);
    bg = nullptr;
}

inline void DeleteOBJ(NEA_Hw2DOBJ *&obj)
{
    if (obj == nullptr) return;
    NEA_Hw2DOBJDelete(obj);
    obj = nullptr;
}

inline void DeleteOBJAsset(NEA_Hw2DOBJAsset *&asset)
{
    if (asset == nullptr) return;
    NEA_Hw2DOBJAssetDelete(asset);
    asset = nullptr;
}

inline void DeleteEmitter(NEA_ParticleEmitter *&emitter)
{
    if (emitter == nullptr) return;
    NEA_ParticleEmitterDelete(emitter);
    emitter = nullptr;
}

inline void DeleteCamera(NEA_Camera *&cam)
{
    if (cam == nullptr) return;
    NEA_CameraDelete(cam);
    cam = nullptr;
}
