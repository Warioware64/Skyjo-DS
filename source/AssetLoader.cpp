#include "AssetLoader.hpp"
#include "DebugPrint.hpp"
#include "ErrorHandler.hpp"
#include "Music.hpp"

#include <cstdio>
#include <cstdlib>

namespace
{
    // Safety net: if a worker somehow never finishes we bail out instead of
    // spinning forever. 10 seconds at 60 Hz.
    constexpr int kAssetLoadTimeoutFrames = 600;

    // How many times a batch will re-issue the jobs that failed, and how long
    // it pauses first. The pause is the point: a failure here is usually the
    // previous screen's buffers not having been released yet, so the retry has
    // to happen a moment later, not immediately. Three rounds of half a second
    // stays well inside the timeout above.
    constexpr int kAssetRetryRounds = 3;
    constexpr int kAssetRetryDelayFrames = 30;

    // grit keys transparency to magenta (0x7C1F) and writes it to palette
    // entry 0. On the DS that entry is the *backdrop*: it is displayed wherever
    // nothing else covers it, including while a background layer is still
    // hidden. So loading a background palette turns the whole screen magenta
    // until the layer is shown. Entry 0 is never referenced by the tiles of an
    // opaque background, so overwriting it costs nothing and a gap shows white.
    constexpr u16 kBackdrop = RGB15(31, 31, 31);
}

std::size_t HeapBytesFree()
{
    // getHeapEnd() is the current break; the allocator may still grow up to
    // getHeapLimit(). Free chunks below the break are not counted here, which
    // is exactly why HeapLargestBlock() exists alongside it.
    return static_cast<std::size_t>(getHeapLimit() - getHeapEnd());
}

std::size_t HeapLargestBlock()
{
    // Binary search on what malloc will actually hand over. Costs a few dozen
    // allocations, so this is for the failure path only.
    std::size_t lo = 0;
    std::size_t hi = HeapBytesFree() + (1024 * 1024); // may exceed the break

    while (lo < hi)
    {
        std::size_t mid = lo + (hi - lo + 1) / 2;
        void *p = std::malloc(mid);
        if (p != nullptr)
        {
            std::free(p);
            lo = mid;
        }
        else
        {
            hi = mid - 1;
        }
    }
    return lo;
}

// Appends the two heap figures to `out`, for a failure that is probably about
// memory. Written into a caller-owned buffer because the string this ends up in
// is built while the game is on its way down.
static void AppendHeapState(std::string &out)
{
    char buf[96];
    std::snprintf(buf, sizeof(buf), " (heap %uK free, largest block %uK)",
                  static_cast<unsigned>(HeapBytesFree() / 1024),
                  static_cast<unsigned>(HeapLargestBlock() / 1024));
    out.append(buf);
}

bool AsyncAssetBatch::Retryable(const Request &r)
{
    return r.kind != Request::Kind::FileWrite;
}

NEA_AsyncFile *AsyncAssetBatch::Issue(const Request &r)
{
    switch (r.kind)
    {
        case Request::Kind::TexGRF:
            return NEA_MaterialTexLoadGRFAsync(
                static_cast<NEA_Material *>(r.target),
                static_cast<NEA_Palette *>(r.palette),
                NEA_TEXGEN_TEXCOORD, r.path);

        case Request::Kind::BGGRF:
            return NEA_Hw2DBGLoadGRFFATAsync(
                static_cast<NEA_Hw2DBG *>(r.target), r.path, r.slot);

        case Request::Kind::OBJAsset:
            return NEA_Hw2DOBJAssetLoadGRFFATAsync(
                static_cast<NEA_Hw2DOBJAsset *>(r.target), r.path);

        case Request::Kind::Particle:
            return NEA_ParticleEmitterLoadFATAsync(
                static_cast<NEA_ParticleEmitter *>(r.target), r.path);

        case Request::Kind::RichTextMeta:
            return NEA_RichTextMetadataLoadFATAsync(
                static_cast<u32>(r.slot), r.path);

        case Request::Kind::FileWrite:
            return NEA_FATWriteDataAsync(r.path, r.data, r.size,
                                         NEA_ASYNC_WRITE_COPY);
    }
    return nullptr;
}

void AsyncAssetBatch::Track(const Request &request)
{
    NEA_AsyncFile *handle = Issue(request);
    if (handle == nullptr)
    {
        error.errorReason.assign("Couldn't queue an asset load: ");
        error.errorReason.append(request.path);
        AppendHeapState(error.errorReason);
        std::terminate();
    }

    this->jobs.emplace_back(Job{AsyncFilePtr(handle), request});
}

void AsyncAssetBatch::QueueTexGRF(NEA_Material *mat, NEA_Palette *pal,
                                  const char *path)
{
    this->Track({Request::Kind::TexGRF, path, mat, pal, 0, nullptr, 0});
}

void AsyncAssetBatch::QueueBGGRF(NEA_Hw2DBG *bg, const char *path,
                                 int paletteSlot)
{
    this->Track({Request::Kind::BGGRF, path, bg, nullptr, paletteSlot,
                 nullptr, 0});
}

void AsyncAssetBatch::QueueOBJAssetGRF(NEA_Hw2DOBJAsset *asset, const char *path)
{
    this->Track({Request::Kind::OBJAsset, path, asset, nullptr, 0, nullptr, 0});
}

void AsyncAssetBatch::QueueParticleEmitter(NEA_ParticleEmitter *emitter,
                                           const char *path)
{
    this->Track({Request::Kind::Particle, path, emitter, nullptr, 0, nullptr, 0});
}

void AsyncAssetBatch::QueueRichTextMetadata(u32 slot, const char *path)
{
    this->Track({Request::Kind::RichTextMeta, path, nullptr, nullptr,
                 static_cast<int>(slot), nullptr, 0});
}

bool AsyncAssetBatch::QueueFileWrite(const char *path, const void *data,
                                     std::size_t size)
{
    const Request r{Request::Kind::FileWrite, path, nullptr, nullptr, 0,
                    data, size};

    NEA_AsyncFile *handle = Issue(r);
    if (handle == nullptr)
        return false;

    this->jobs.emplace_back(Job{AsyncFilePtr(handle), r});
    return true;
}

bool AsyncAssetBatch::RetryFailed()
{
    for (Job &job : this->jobs)
    {
        if (NEA_AsyncGetState(job.handle.get()) != NEA_ASYNC_ERROR)
            continue;
        if (!Retryable(job.request))
            continue;

        // Release first: the old handle owns whatever the failed attempt did
        // manage to allocate, and giving that back is half the reason the
        // second attempt has a better chance than the first.
        job.handle.reset();

        NEA_AsyncFile *handle = Issue(job.request);
        if (handle == nullptr)
            return false;

        job.handle.reset(handle);
    }
    return true;
}

void AsyncAssetBatch::Wait(const char *label)
{
    if (!this->TryWait(label))
    {
        error.errorReason.assign("Asset load failed: ");
        error.errorReason.append(this->failedPath != nullptr
                                     ? this->failedPath
                                     : (label != nullptr ? label : "(unnamed)"));
        AppendHeapState(error.errorReason);
        std::terminate();
    }
}

bool AsyncAssetBatch::TryWait(const char *label)
{
    if (this->jobs.empty())
        return true;

    const int total = static_cast<int>(this->jobs.size());

    // Completion is counted over this batch's own handles rather than
    // NEA_AsyncPendingCount(), which is global and would also see jobs queued
    // elsewhere (an intro background still landing, a save being written).
    int frames = 0;
    int retriesLeft = kAssetRetryRounds;
    int retryDelay = 0;

    while (true)
    {
        int done = 0;
        int failedCount = 0;
        for (const Job &job : this->jobs)
        {
            NEA_AsyncState state = NEA_AsyncGetState(job.handle.get());
            if (state == NEA_ASYNC_DONE || state == NEA_ASYNC_ERROR)
                done++;
            if (state == NEA_ASYNC_ERROR && Retryable(job.request))
                failedCount++;
        }
        // Checked before waiting, so a batch that has already landed (the
        // intro's background swap, finalized by the render loop long ago)
        // costs no frame at all.
        if (done == total)
        {
            // "Finished" counts failures too, so this is where a broken job is
            // caught. Give it another go rather than taking the game down for
            // what is usually a moment of memory pressure that has since
            // passed -- but wait first, since retrying in the same frame that
            // it failed would just fail the same way.
            if (failedCount > 0 && retriesLeft > 0)
            {
                if (retryDelay < kAssetRetryDelayFrames)
                {
                    retryDelay++;
                }
                else
                {
                    retriesLeft--;
                    retryDelay = 0;
                    if (!this->RetryFailed())
                        break; // the engine refused; report it below
                    frames = 0;
                    continue;
                }
            }
            else
            {
                break;
            }
        }

        // NEA_UPDATE_ASSETS is what advances the loads and runs their VRAM
        // uploads during the vertical blank.
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_ASSETS |
                                                     NEA_UPDATE_HW2D));

        // Keep the keypad's edge detector honest. libnds computes keysDown()
        // as "held now, not held at the previous scanKeys()", so a stretch of
        // frames without one leaves the baseline seconds stale, and the next
        // scan reports everything currently held as freshly pressed. A party
        // load spends hundreds of frames in here, which is exactly long enough
        // for a resting thumb to look like a deliberate press the moment the
        // game starts reading input again.
        scanKeys();

        // The console keeps running while we wait, so the music stream's
        // circular buffer still has to be refilled every frame — without this
        // the track stutters or loops for the whole duration of the load.
        Music::Pump();

        // Nothing is drawn here on purpose. Callers decide what is on screen
        // while they load -- in practice a white screen, which is also what
        // hides the backdrop going magenta as each palette lands.

        if (++frames > kAssetLoadTimeoutFrames)
        {
            error.errorReason.assign("Async asset load timed out: ");
            error.errorReason.append(label != nullptr ? label : "(unnamed)");
            AppendHeapState(error.errorReason);
            std::terminate();
        }
    }

    bool failed = false;
    for (const Job &job : this->jobs)
    {
        if (NEA_AsyncGetState(job.handle.get()) == NEA_ASYNC_DONE)
            continue;

        failed = true;

        // Name the first file that broke, not the batch. The engine fails a job
        // when it cannot open the file, cannot allocate a buffer for it, cannot
        // decode it, or could never start a worker thread for it -- so the file
        // plus the heap figures below are what separate those causes.
        if (this->failedPath == nullptr)
            this->failedPath = job.request.path;
    }

    // The handles are owned by `jobs`, so clearing it releases every one of
    // them, including the ones that failed. failedPath is deliberately left
    // set: Wait() reads it immediately after this returns.
    this->jobs.clear();

    DEBUG_PRINT("AsyncAssetBatch::TryWait() : batch complete");
    return !failed;
}

void ClearBackdropToWhite()
{
    BG_PALETTE[0] = kBackdrop;
    BG_PALETTE_SUB[0] = kBackdrop;
}
