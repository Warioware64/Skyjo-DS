// Read-only filesystem over the asset archive linked into this binary.
//
// A Download Play guest has no cartridge and no NitroFS, but the game's asset
// code is all fopen()-based: NEA's loaders (NEAFAT.c) and libnds' grfLoadPath()
// both go through plain stdio. Registering a BlocksDS device_io_t therefore
// lets every one of those paths work untouched -- no memory-loading variants of
// AssetLoader, no changes to the paths in GamePartySharedAssets.
//
// The archive is built by pack_assets.py. Two details make the plumbing work:
//
//  - deviceIoAdd() makes this the current drive because nothing else is mounted
//    (device_io.c:46-48), and every asset path in the game is relative
//    ("cards/card_1_png.grf"), so they resolve here with no prefix at all. The
//    drive could not be called "nitro" in any case -- NitroFS claims that name
//    unconditionally.
//  - open() must return a descriptor with bits 31:28 and 1:0 clear, or libnds
//    calls libndsCrash("Bad device file descriptor"). A malloc'd pointer in main
//    RAM satisfies that.

#include "AssetDevice.hpp"

#include <nds.h>
#include <nds/arm9/device_io.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

// Emitted by bin2c from the blob pack_assets.py writes.
extern "C" {
extern const uint8_t assets_bin[];
extern const size_t assets_bin_size;
}

namespace
{
    constexpr uint32_t kMagic = 0x5241534B; // "KSAR"
    constexpr const char *kDriveName = "assets";

    struct Entry
    {
        uint32_t nameOffset;
        uint32_t dataOffset;
        uint32_t storedSize;
        uint32_t realSize;
    };

    struct Archive
    {
        uint32_t magic;
        uint32_t count;
        Entry entries[];
    };

    // One open file. `data` points either straight into the linked archive (for
    // a stored entry, so opening costs nothing) or at a buffer we decompressed
    // into and must free on close.
    struct OpenFile
    {
        const uint8_t *data;
        size_t size;
        size_t pos;
        bool owned;
    };

    const Archive *Root()
    {
        const Archive *a = reinterpret_cast<const Archive *>(assets_bin);
        return (a->magic == kMagic) ? a : nullptr;
    }

    const char *NameOf(const Archive *a, const Entry &e)
    {
        return reinterpret_cast<const char *>(assets_bin) + e.nameOffset;
    }

    // Paths arrive exactly as the caller wrote them, including any drive prefix,
    // so strip "assets:/" and any leading slashes before matching.
    const char *StripPrefix(const char *path)
    {
        const char *colon = std::strchr(path, ':');
        if (colon != nullptr)
            path = colon + 1;
        while (*path == '/')
            ++path;
        return path;
    }

    const Entry *Find(const char *path)
    {
        const Archive *a = Root();
        if (a == nullptr)
            return nullptr;

        const char *want = StripPrefix(path);
        for (uint32_t i = 0; i < a->count; ++i)
        {
            if (std::strcmp(NameOf(a, a->entries[i]), want) == 0)
                return &a->entries[i];
        }
        return nullptr;
    }

    bool AssetIsDrive(const char *name)
    {
        return std::strcmp(name, kDriveName) == 0;
    }

    int AssetOpen(const char *path, int flags, mode_t mode)
    {
        (void)mode;

        // Read-only: anything asking to write is a bug rather than something to
        // fail quietly on.
        if ((flags & O_ACCMODE) != O_RDONLY)
        {
            errno = EROFS;
            return -1;
        }

        const Entry *e = Find(path);
        if (e == nullptr)
        {
            errno = ENOENT;
            return -1;
        }

        OpenFile *f = static_cast<OpenFile *>(std::malloc(sizeof(OpenFile)));
        if (f == nullptr)
        {
            errno = ENOMEM;
            return -1;
        }

        const uint8_t *stored = assets_bin + e->dataOffset;

        if (e->storedSize == e->realSize)
        {
            // Stored as-is: hand out a pointer into the binary, no copy.
            f->data = stored;
            f->owned = false;
        }
        else
        {
            // Compressed. The payload starts with the BIOS header word whose
            // bits 4-7 name the codec, the same convention grf.c uses.
            uint8_t *buffer = static_cast<uint8_t *>(std::malloc(e->realSize));
            if (buffer == nullptr)
            {
                std::free(f);
                errno = ENOMEM;
                return -1;
            }

            uint32_t header;
            std::memcpy(&header, stored, sizeof(header));

            switch (header & 0xF0)
            {
                case 0x10: decompress(stored, buffer, LZ77); break;
                case 0x20: decompress(stored, buffer, HUFF); break;
                case 0x30: decompress(stored, buffer, RLE);  break;
                default:
                    std::free(buffer);
                    std::free(f);
                    errno = EIO;
                    return -1;
            }

            f->data = buffer;
            f->owned = true;
        }

        f->size = e->realSize;
        f->pos = 0;

        return static_cast<int>(reinterpret_cast<uintptr_t>(f));
    }

    OpenFile *FileOf(int fd)
    {
        return reinterpret_cast<OpenFile *>(static_cast<uintptr_t>(fd));
    }

    int AssetClose(int fd)
    {
        OpenFile *f = FileOf(fd);
        if (f->owned)
            std::free(const_cast<uint8_t *>(f->data));
        std::free(f);
        return 0;
    }

    ssize_t AssetRead(int fd, void *ptr, size_t len)
    {
        OpenFile *f = FileOf(fd);
        size_t left = f->size - f->pos;
        if (len > left)
            len = left;

        std::memcpy(ptr, f->data + f->pos, len);
        f->pos += len;
        return static_cast<ssize_t>(len);
    }

    off_t AssetLseek(int fd, off_t pos, int whence)
    {
        OpenFile *f = FileOf(fd);

        off_t base;
        switch (whence)
        {
            case SEEK_SET: base = 0; break;
            case SEEK_CUR: base = static_cast<off_t>(f->pos); break;
            case SEEK_END: base = static_cast<off_t>(f->size); break;
            default:
                errno = EINVAL;
                return -1;
        }

        off_t target = base + pos;
        if (target < 0 || target > static_cast<off_t>(f->size))
        {
            errno = EINVAL;
            return -1;
        }

        f->pos = static_cast<size_t>(target);
        return target;
    }

    void FillStat(struct stat *st, size_t size)
    {
        std::memset(st, 0, sizeof(*st));
        st->st_size = static_cast<off_t>(size);
        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
        st->st_nlink = 1;
    }

    int AssetFstat(int fd, struct stat *st)
    {
        FillStat(st, FileOf(fd)->size);
        return 0;
    }

    int AssetStat(const char *file, struct stat *st)
    {
        const Entry *e = Find(file);
        if (e == nullptr)
        {
            errno = ENOENT;
            return -1;
        }
        FillStat(st, e->realSize);
        return 0;
    }

    // NEA's loaders call fseek(SEEK_END) + ftell to size a file, so lseek is as
    // load-bearing as read. Everything else is left NULL, which libnds turns
    // into ENOSYS rather than a crash.
    const device_io_t g_assetDevice = {
        .isdrive = AssetIsDrive,
        .open    = AssetOpen,
        .close   = AssetClose,
        .write   = nullptr,
        .read    = AssetRead,
        .lseek   = AssetLseek,
        .fstat   = AssetFstat,
        .stat    = AssetStat,
    };

    int g_deviceIndex = -1;
}

bool AssetDeviceMount()
{
    if (g_deviceIndex >= 0)
        return true;

    if (Root() == nullptr)
        return false; // archive missing or corrupt


    g_deviceIndex = deviceIoAdd(&g_assetDevice);
    return g_deviceIndex >= 0;
}

int AssetDeviceFileCount()
{
    const Archive *a = Root();
    return (a != nullptr) ? static_cast<int>(a->count) : 0;
}
