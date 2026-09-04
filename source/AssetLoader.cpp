#include "AssetLoader.hpp"
#include "DebugPrint.hpp"
#include "ErrorHandler.hpp"
#include "Music.hpp"

namespace
{
    // Safety net: if a worker somehow never finishes we bail out instead of
    // spinning forever. 10 seconds at 60 Hz.
    constexpr int kAssetLoadTimeoutFrames = 600;

    // What the loading overlay draws this frame. NEA_Process() takes a plain
    // function pointer, so the draw callback can't capture — it reads these.
    const char *g_loadLabel = nullptr;
    int g_loadDone = 0;
    int g_loadTotal = 0;

    // Loading overlay, drawn on the main engine (the bottom screen, because
    // Process::ProcessInit calls NEA_MainScreenSetOnBottom). Deliberately made
    // of flat quads plus one line of rich text so it needs no assets of its
    // own — it has to work while the assets are precisely what's missing.
    void DrawLoadingOverlay()
    {
        NEA_2DViewInit();
        NEA_ClearColorSet(NEA_White, 0, 63);

        constexpr s16 panelX1 = 24, panelY1 = 76;
        constexpr s16 panelX2 = 232, panelY2 = 116;
        constexpr s16 barX1 = 36, barY1 = 98;
        constexpr s16 barX2 = 220, barY2 = 108;

        NEA_2DDrawQuad(panelX1, panelY1, panelX2, panelY2, 1, RGB15(4, 4, 8));
        NEA_2DDrawQuad(barX1, barY1, barX2, barY2, 2, RGB15(10, 10, 14));

        if (g_loadTotal > 0 && g_loadDone > 0)
        {
            const int span = barX2 - barX1;
            s16 fill = static_cast<s16>(barX1 + (span * g_loadDone) / g_loadTotal);
            if (fill > barX1)
                NEA_2DDrawQuad(barX1, barY1, fill, barY2, 3, RGB15(31, 28, 12));
        }

        // NEA_2DDrawQuad leaves its colour in the GPU's vertex-colour register
        // and the font is drawn untinted, so reset it before the text.
        NEA_PolyColor(NEA_White);
        if (g_loadLabel != nullptr)
            NEA_RichTextRender3D(0, g_loadLabel, barX1, panelY1 + 4);
    }
}

void AsyncAssetBatch::Track(NEA_AsyncFile *handle, const char *path)
{
    if (handle == nullptr)
    {
        error.errorReason.assign("Couldn't queue an async load: ");
        error.errorReason.append(path);
        std::terminate();
    }

    this->jobs.emplace_back(handle);
}

void AsyncAssetBatch::QueueTexGRF(NEA_Material *mat, NEA_Palette *pal,
                                  const char *path)
{
    this->Track(NEA_MaterialTexLoadGRFAsync(mat, pal, NEA_TEXGEN_TEXCOORD, path),
                path);
}

void AsyncAssetBatch::QueueBGGRF(NEA_Hw2DBG *bg, const char *path,
                                 int paletteSlot)
{
    this->Track(NEA_Hw2DBGLoadGRFFATAsync(bg, path, paletteSlot), path);
}

void AsyncAssetBatch::QueueOBJAssetGRF(NEA_Hw2DOBJAsset *asset, const char *path)
{
    this->Track(NEA_Hw2DOBJAssetLoadGRFFATAsync(asset, path), path);
}

void AsyncAssetBatch::QueueParticleEmitter(NEA_ParticleEmitter *emitter,
                                           const char *path)
{
    this->Track(NEA_ParticleEmitterLoadFATAsync(emitter, path), path);
}

void AsyncAssetBatch::QueueRichTextMetadata(u32 slot, const char *path)
{
    this->Track(NEA_RichTextMetadataLoadFATAsync(slot, path), path);
}

bool AsyncAssetBatch::QueueFileWrite(const char *path, const void *data,
                                     std::size_t size)
{
    NEA_AsyncFile *handle = NEA_FATWriteDataAsync(path, data, size,
                                                  NEA_ASYNC_WRITE_COPY);
    if (handle == nullptr)
        return false;

    this->jobs.emplace_back(handle);
    return true;
}

void AsyncAssetBatch::Wait(const char *label)
{
    if (!this->TryWait(label))
    {
        error.errorReason.assign("An async asset load failed: ");
        error.errorReason.append(label != nullptr ? label : "(unnamed)");
        std::terminate();
    }
}

bool AsyncAssetBatch::TryWait(const char *label)
{
    if (this->jobs.empty())
        return true;

    g_loadLabel = label;
    g_loadTotal = static_cast<int>(this->jobs.size());
    g_loadDone = 0;

    // Progress is counted over this batch's own handles rather than
    // NEA_AsyncPendingCount(), which is global and would also see jobs queued
    // elsewhere (an intro background still landing, a save being written).
    int frames = 0;
    while (true)
    {
        int done = 0;
        for (const AsyncFilePtr &handle : this->jobs)
        {
            NEA_AsyncState state = NEA_AsyncGetState(handle.get());
            if (state == NEA_ASYNC_DONE || state == NEA_ASYNC_ERROR)
                done++;
        }
        g_loadDone = done;

        // Checked before waiting, so a batch that has already landed (the
        // intro's background swap, finalized by the render loop long ago)
        // costs no frame and flashes no overlay.
        if (done == g_loadTotal)
            break;

        // NEA_UPDATE_ASSETS is what advances the loads and runs their VRAM
        // uploads during the vertical blank.
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_ASSETS |
                                                     NEA_UPDATE_HW2D));

        // The console keeps running while we wait, so the music stream's
        // circular buffer still has to be refilled every frame — without this
        // the track stutters or loops for the whole duration of the load.
        Music::Pump();

        NEA_Process(DrawLoadingOverlay);

        if (++frames > kAssetLoadTimeoutFrames)
        {
            error.errorReason.assign("Async asset load timed out: ");
            error.errorReason.append(label != nullptr ? label : "(unnamed)");
            std::terminate();
        }
    }

    bool failed = false;
    for (const AsyncFilePtr &handle : this->jobs)
    {
        if (NEA_AsyncGetState(handle.get()) != NEA_ASYNC_DONE)
            failed = true;
    }

    // The handles are owned by `jobs`, so clearing it releases every one of
    // them, including the ones that failed.
    this->jobs.clear();
    g_loadLabel = nullptr;
    g_loadTotal = 0;
    g_loadDone = 0;

    DEBUG_PRINT("AsyncAssetBatch::TryWait() : batch complete");
    return !failed;
}
