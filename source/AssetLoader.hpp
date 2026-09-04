#pragma once

#include "globalHeader.hpp"

#include <memory>

// One batch of asynchronous asset loads.
//
// Every load is only *queued* by the Queue* methods: a worker thread reads the
// files in the background while the main loop keeps running, and the VRAM
// uploads happen during the vertical blank from NEA_AsyncProcess(). Wait() then
// pumps the engine (and the music stream) until the whole batch has landed.
//
// Wait() blocks the calling function, not the console: callers use the assets
// as soon as they return, so the load still looks synchronous to them, but the
// music keeps streaming and a progress bar is drawn while it happens.
//
// Any load that fails is fatal, matching the rest of the project's asset code:
// the reason goes into `error.errorReason` and std::terminate() runs.
class AsyncAssetBatch
{
    public:
        AsyncAssetBatch() = default;
        ~AsyncAssetBatch() = default;

        AsyncAssetBatch(const AsyncAssetBatch&) = delete;
        AsyncAssetBatch& operator=(const AsyncAssetBatch&) = delete;

        // 3D material + palette from a GRF (ptexconv output).
        void QueueTexGRF(NEA_Material *mat, NEA_Palette *pal, const char *path);

        // Hardware 2D background from a GRF. `paletteSlot` only has an effect on
        // 4bpp backgrounds; an 8bpp one always owns its engine's whole palette.
        void QueueBGGRF(NEA_Hw2DBG *bg, const char *path, int paletteSlot);

        // Shared hardware sprite asset (gfx + auto-allocated palette slot).
        void QueueOBJAssetGRF(NEA_Hw2DOBJAsset *asset, const char *path);

        // Particle emitter description (.npe).
        void QueueParticleEmitter(NEA_ParticleEmitter *emitter, const char *path);

        // Rich-text font metadata (.fnt). The matching bitmap has no async
        // loader in the engine, so it stays a plain NEA_RichTextMaterialLoadGRF.
        void QueueRichTextMetadata(u32 slot, const char *path);

        // Writes a buffer to a file in the background. The engine writes to a
        // temporary and renames, so an interrupted save can't destroy the old
        // one. The data is copied, so `data` may go away as soon as this returns.
        //
        // Unlike the asset loaders above this one is not fatal on failure: a
        // save can legitimately fail (no card, read-only medium) and losing a
        // save is not a reason to kill the game. Returns false if the write
        // could not even be queued; pair it with TryWait().
        bool QueueFileWrite(const char *path, const void *data, std::size_t size);

        // Pumps the engine until every queued job has finished, keeping the
        // music stream fed and drawing `label` with a progress bar. Releases
        // every handle before returning, so the batch can be reused.
        void Wait(const char *label);

        // Same, but returns false instead of terminating when a job failed.
        // For work whose failure the caller can live with, such as a save.
        bool TryWait(const char *label);

        // Number of jobs queued since the last Wait().
        std::size_t Pending() const { return this->jobs.size(); }

    private:
        // RAII owner for an async load handle, so every early return still
        // releases it. NEA_AsyncFile is an opaque (incomplete) type, which is
        // fine here only because the deleter calls a function instead of
        // `delete` on it.
        struct AsyncFileRelease
        {
            void operator()(NEA_AsyncFile *handle) const
            {
                NEA_AsyncRelease(handle);
            }
        };

        using AsyncFilePtr = std::unique_ptr<NEA_AsyncFile, AsyncFileRelease>;

        // Records a queued handle, or dies if the engine refused the job.
        void Track(NEA_AsyncFile *handle, const char *path);

        std::vector<AsyncFilePtr> jobs;
};
