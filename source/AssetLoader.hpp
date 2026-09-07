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
// Overwrites the backdrop colour on both engines.
//
// grit keys transparency to magenta and puts it in palette entry 0, which the
// DS displays as the backdrop wherever nothing covers it -- so loading a
// background palette flashes the screen magenta until the layer appears. Call
// this after loading backgrounds. Entry 0 is never used by an opaque
// background's tiles, so nothing is lost.
void ClearBackdropToWhite();

// Bytes of heap left, and the largest single block still obtainable from it.
//
// Two numbers rather than one because they answer different questions: plenty
// of free memory with a small largest block means fragmentation, while both
// being small means the heap is simply used up. The probe allocates while it
// measures, so keep it to diagnostics.
std::size_t HeapBytesFree();
std::size_t HeapLargestBlock();

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

        // Everything needed to issue one load, kept so it can be issued again.
        //
        // The engine fails a job when it cannot open the file, cannot allocate
        // a buffer for it, cannot decode it, or could never start a worker
        // thread -- and three of those four are transient: they depend on what
        // else happened to be holding memory at that instant. Moving quickly
        // between menus is exactly when that is most likely, because one
        // screen's buffers are still being released while the next screen's
        // loads are being queued. Retrying costs a few frames; the alternative
        // was killing the game.
        //
        // Paths are always string literals, so borrowing the pointer is safe.
        struct Request
        {
            enum class Kind { TexGRF, BGGRF, OBJAsset, Particle, RichTextMeta,
                              FileWrite };

            Kind kind;
            const char *path;
            void *target;       // material / background / sprite asset / emitter
            void *palette;      // TexGRF only
            int slot;           // BGGRF palette slot, RichTextMeta font slot
            const void *data;   // FileWrite only
            std::size_t size;   // FileWrite only
        };

        // A queued job, and what it would take to ask for it again.
        struct Job
        {
            AsyncFilePtr handle;
            Request request;
        };

        // Issues one request, returning the engine's handle (NULL on refusal).
        static NEA_AsyncFile *Issue(const Request &request);

        // Whether a failed request may simply be issued again.
        //
        // Asset loads may: they only read a path. A file write may not. It is
        // queued with NEA_ASYNC_WRITE_COPY exactly so the caller can let its
        // buffer go the moment QueueFileWrite() returns, which callers do -- so
        // by the time a retry came round, `data` could point at nothing. A
        // failed save is also not fatal, which is why it is paired with
        // TryWait() rather than Wait() in the first place.
        static bool Retryable(const Request &request);

        // Re-issues every job of the batch that ended in NEA_ASYNC_ERROR.
        // Returns false if the engine refused to even queue one of them.
        bool RetryFailed();

        // Issues a request and records it, or dies if the engine refused it.
        void Track(const Request &request);

        // The first asset of the batch that did not finish, set by TryWait().
        const char *failedPath = nullptr;

        std::vector<Job> jobs;
};
